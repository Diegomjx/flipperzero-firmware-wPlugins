#include "nfc_supported_card_plugin.h"
#include <flipper_application.h>
#include <nfc/protocols/mf_classic/mf_classic_poller_sync.h>
#include <bit_lib.h>
#include <string.h>
#include <furi.h>

#define TAG "SmartRider"

static const uint8_t STANDARD_KEY_1[6] = {0x20, 0x31, 0xD1, 0xE5, 0x7A, 0x3B};
static const uint8_t STANDARD_KEY_2[6] = {0x4C, 0xA6, 0x02, 0x9F, 0x94, 0x73};
static const uint8_t STANDARD_KEY_3[6] = {0x19, 0x19, 0x53, 0x98, 0xE3, 0x2F};

typedef struct {
    uint32_t timestamp;
    uint8_t tap_on;
    char route[5];
    uint32_t cost;
    uint16_t transaction_number;
    uint16_t journey_number;
} TripData;

typedef struct {
    uint32_t balance;
    uint8_t token;
    uint16_t issued_days;
    uint16_t expiry_days;
    char card_serial_number[11];
    uint16_t purchase_cost;
    uint16_t auto_load_threshold;
    uint16_t auto_load_value;
    TripData last_trips[2];
} SmartRiderData;

static int offset_to_block(int offset) {
    return offset / 16;
}

static int offset_in_block(int offset) {
    return offset % 16;
}

static const char* get_concession_type(uint8_t token) {
    switch(token) {
    case 0x00:
        return "Pre-issue";
    case 0x01:
        return "Standard Fare";
    case 0x02:
        return "Student";
    case 0x04:
        return "Tertiary";
    case 0x06:
        return "Seniors";
    case 0x07:
        return "Health Care";
    case 0x0e:
        return "PTA Staff";
    case 0x0f:
        return "Pensioner";
    case 0x10:
        return "Free Travel";
    default:
        return "Unknown";
    }
}

static bool smartrider_verify(Nfc* nfc) {
    furi_assert(nfc);

    MfClassicKey key = {};
    MfClassicError error;
    MfClassicBlock block_data;

    // Check Sector 0 Key A (STANDARD_KEY_1)
    memcpy(key.data, STANDARD_KEY_1, sizeof(STANDARD_KEY_1));
    error = mf_classic_poller_sync_auth(nfc, 0, &key, MfClassicKeyTypeA, NULL);
    if(error != MfClassicErrorNone) {
        FURI_LOG_D(TAG, "Sector 0 Key A authentication failed");
        return false;
    }

    // Read the key from the card and compare
    error = mf_classic_poller_sync_read_block(nfc, 0, &key, MfClassicKeyTypeA, &block_data);
    if(error != MfClassicErrorNone || memcmp(block_data.data, STANDARD_KEY_1, 6) != 0) {
        FURI_LOG_D(TAG, "Sector 0 Key A mismatch");
        return false;
    }

    // Check Sector 6 Key A (STANDARD_KEY_2)
    uint8_t sector_6_first_block = mf_classic_get_first_block_num_of_sector(6);
    memcpy(key.data, STANDARD_KEY_2, sizeof(STANDARD_KEY_2));
    error = mf_classic_poller_sync_auth(nfc, sector_6_first_block, &key, MfClassicKeyTypeA, NULL);
    if(error != MfClassicErrorNone) {
        FURI_LOG_D(TAG, "Sector 6 Key A authentication failed");
        return false;
    }

    // Read the key from the card and compare
    error = mf_classic_poller_sync_read_block(
        nfc, sector_6_first_block, &key, MfClassicKeyTypeA, &block_data);
    if(error != MfClassicErrorNone || memcmp(block_data.data, STANDARD_KEY_2, 6) != 0) {
        FURI_LOG_D(TAG, "Sector 6 Key A mismatch");
        return false;
    }

    // Check Sector 6 Key B (STANDARD_KEY_3)
    memcpy(key.data, STANDARD_KEY_3, sizeof(STANDARD_KEY_3));
    error = mf_classic_poller_sync_auth(nfc, sector_6_first_block, &key, MfClassicKeyTypeB, NULL);
    if(error != MfClassicErrorNone) {
        FURI_LOG_D(TAG, "Sector 6 Key B authentication failed");
        return false;
    }

    // Read the key from the card and compare
    error = mf_classic_poller_sync_read_block(
        nfc, sector_6_first_block + 3, &key, MfClassicKeyTypeB, &block_data);
    if(error != MfClassicErrorNone || memcmp(&block_data.data[10], STANDARD_KEY_3, 6) != 0) {
        FURI_LOG_D(TAG, "Sector 6 Key B mismatch");
        return false;
    }

    // If all checks pass, it's likely a SmartRider card
    FURI_LOG_I(TAG, "SmartRider card verified");
    return true;
}

static bool smartrider_read(Nfc* nfc, NfcDevice* device) {
    furi_assert(nfc);
    furi_assert(device);

    MfClassicData* data = mf_classic_alloc();
    nfc_device_copy_data(device, NfcProtocolMfClassic, data);

    MfClassicType type = MfClassicTypeMini;
    MfClassicError error = mf_classic_poller_sync_detect_type(nfc, &type);
    if(error != MfClassicErrorNone) {
        mf_classic_free(data);
        return false;
    }

    data->type = type;
    if(type != MfClassicType1k) {
        mf_classic_free(data);
        return false;
    }

    MfClassicDeviceKeys keys = {.key_a_mask = 0, .key_b_mask = 0};
    for(size_t i = 0; i < mf_classic_get_total_sectors_num(data->type); i++) {
        memcpy(keys.key_a[i].data, STANDARD_KEY_1, sizeof(STANDARD_KEY_1));
        FURI_BIT_SET(keys.key_a_mask, i);
    }

    error = mf_classic_poller_sync_read(nfc, &keys, data);
    if(error == MfClassicErrorNotPresent) {
        mf_classic_free(data);
        return false;
    }

    nfc_device_set_data(device, NfcProtocolMfClassic, data);
    bool is_read = (error == MfClassicErrorNone);
    mf_classic_free(data);

    return is_read;
}

static bool parse_trip_data(const MfClassicBlock* block_data, TripData* trip, int trip_offset) {
    trip->timestamp = bit_lib_bytes_to_num_le(block_data->data + trip_offset + 3, 4);
    trip->tap_on = (block_data->data[trip_offset + 7] & 0x10) == 0x10;
    memcpy(trip->route, block_data->data + trip_offset + 8, 4);
    trip->route[4] = '\0';
    trip->cost = bit_lib_bytes_to_num_le(block_data->data + trip_offset + 13, 2);
    trip->transaction_number = bit_lib_bytes_to_num_le(block_data->data + trip_offset, 2);
    trip->journey_number = bit_lib_bytes_to_num_le(block_data->data + trip_offset + 2, 2);
    return true;
}

static bool smartrider_parse(const NfcDevice* device, FuriString* parsed_data) {
    furi_assert(device);
    const MfClassicData* data = nfc_device_get_data(device, NfcProtocolMfClassic);
    bool parsed = false;

    do {
        // Verify keys
        const uint8_t verify_sectors[] = {0, 6};
        for(size_t i = 0; i < COUNT_OF(verify_sectors); i++) {
            const MfClassicSectorTrailer* sec_tr =
                mf_classic_get_sector_trailer_by_sector(data, verify_sectors[i]);
            uint64_t key_a =
                bit_lib_bytes_to_num_be(sec_tr->key_a.data, COUNT_OF(sec_tr->key_a.data));
            uint64_t key_b =
                bit_lib_bytes_to_num_be(sec_tr->key_b.data, COUNT_OF(sec_tr->key_b.data));

            if(verify_sectors[i] == 0 && key_a != 0x203141E57A3B) break; // STANDARD_KEY_1
            if(verify_sectors[i] == 6 && (key_a != 0x4CA6029F9473 || key_b != 0x1919539853F))
                break; // STANDARD_KEY_2 and STANDARD_KEY_3
        }

        // Pre-check block readability for required blocks
        int required_blocks[] = {
            offset_to_block(0xe0),
            offset_to_block(0x40),
            offset_to_block(0x50),
            1,
            offset_to_block(0x340),
            offset_to_block(0x320)};
        for(size_t i = 0; i < COUNT_OF(required_blocks); i++) {
            if(!mf_classic_is_block_read(data, required_blocks[i])) break;
        }

        SmartRiderData sr_data = {0};
        const MfClassicBlock* block_data;

        // Parse balance
        block_data = &data->block[offset_to_block(0xe0)];
        sr_data.balance = bit_lib_bytes_to_num_le(block_data->data + offset_in_block(0xe0) + 7, 2);

        // Parse configuration data (Sector 1)
        block_data = &data->block[offset_to_block(0x40)];
        sr_data.issued_days = bit_lib_bytes_to_num_le(block_data->data + 16, 2);
        sr_data.expiry_days = bit_lib_bytes_to_num_le(block_data->data + 18, 2);
        sr_data.auto_load_threshold = bit_lib_bytes_to_num_le(block_data->data + 20, 2);
        sr_data.auto_load_value = bit_lib_bytes_to_num_le(block_data->data + 22, 2);

        // Parse current token (in the next block of Sector 1)
        block_data = &data->block[offset_to_block(0x50)];
        sr_data.token = block_data->data[8]; // Current token at offset 24 in Sector 1

        // Parse card serial number
        block_data = &data->block[1];
        snprintf(
            sr_data.card_serial_number,
            sizeof(sr_data.card_serial_number),
            "%02X%02X%02X%02X%02X",
            block_data->data[6],
            block_data->data[7],
            block_data->data[8],
            block_data->data[9],
            block_data->data[10]);

        // Parse purchase cost
        block_data = &data->block[offset_to_block(0x06)];
        sr_data.purchase_cost =
            bit_lib_bytes_to_num_le(block_data->data + offset_in_block(0x06) + 8, 2);

        // Parse last two trips
        int trip_offsets[] = {0x340, 0x320}; // Last two trip offsets
        for(int i = 0; i < 2; i++) {
            block_data = &data->block[offset_to_block(trip_offsets[i])];
            parse_trip_data(block_data, &sr_data.last_trips[i], offset_in_block(trip_offsets[i]));
        }

        furi_string_printf(
            parsed_data,
            "\e#SmartRider\n"
            "Balance: $%lu.%02lu\n"
            "Concession: %s\n"
            "Serial: %s%s\n"
            "Total Cost: $%u.%02u\n"
            "Auto-Load: $%u.%02u/$%u.%02u\n"
            "Last Trip: %s $%lu.%02lu %s\n"
            "Prev Trip: %s $%lu.%02lu %s",
            sr_data.balance / 100,
            sr_data.balance % 100,
            get_concession_type(sr_data.token),
            strncmp(sr_data.card_serial_number, "00", 2) == 0 ? "SR0" : "",
            strncmp(sr_data.card_serial_number, "00", 2) == 0 ? sr_data.card_serial_number + 2 :
                                                                sr_data.card_serial_number,
            sr_data.purchase_cost / 100,
            sr_data.purchase_cost % 100,
            sr_data.auto_load_threshold / 100,
            sr_data.auto_load_threshold % 100,
            sr_data.auto_load_value / 100,
            sr_data.auto_load_value % 100,
            sr_data.last_trips[0].tap_on ? "Tag on" : "Tag off",
            (unsigned long)(sr_data.last_trips[0].cost / 100),
            (unsigned long)(sr_data.last_trips[0].cost % 100),
            sr_data.last_trips[0].route,
            sr_data.last_trips[1].tap_on ? "Tag on" : "Tag off",
            (unsigned long)(sr_data.last_trips[1].cost / 100),
            (unsigned long)(sr_data.last_trips[1].cost % 100),
            sr_data.last_trips[1].route);

        parsed = true;
    } while(false);

    return parsed;
}

static const NfcSupportedCardsPlugin smartrider_plugin = {
    .protocol = NfcProtocolMfClassic,
    .verify = smartrider_verify,
    .read = smartrider_read,
    .parse = smartrider_parse,
};

static const FlipperAppPluginDescriptor smartrider_plugin_descriptor = {
    .appid = NFC_SUPPORTED_CARD_PLUGIN_APP_ID,
    .ep_api_version = NFC_SUPPORTED_CARD_PLUGIN_API_VERSION,
    .entry_point = &smartrider_plugin,
};

const FlipperAppPluginDescriptor* smartrider_plugin_ep(void) {
    return &smartrider_plugin_descriptor;
}
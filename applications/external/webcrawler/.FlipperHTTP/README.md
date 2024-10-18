# FlipperHTTP
HTTP library for Flipper Zero. Compatible with Wifi Dev Board for Flipper Zero (ESP32S2 Dev Module). View the extended documentation here: https://www.jblanked.com/api/FlipperHTTP/

## Installation
1. Download the `flipper_http_bootloader.bin`, `flipper_http_firmware_a.bin`, and `flipper_http_partitions.bin` files.
2. Unplug your Wifi Dev Board and connect your Flipper Zero to your computer.
3. Open up qFlipper.
4. Click on the `File manager`.
5. Naviate to `SD Card/apps_data/esp_flasher/`
6. Drag all three bin files (or the entire folder) into the directory.
7. Disconnect your Flipper from your computer then turn off your Flipper.
8. Plug your Wi-Fi Devboard into the Flipper then turn on your Flipper.
9. Open the ESP Flasher app on your Flipper, it should be located under `Apps->GPIO` from the main menu. If not, download it from the Flipper App Store.
10. In the ESP Flasher app, select the following options:
    - "Reset Board": wait a few seconds, then go back.
    - "Enter Bootloader": wait until the 'waiting for download' message appears, then go back.
11. Click on Manual Flash.
12. Click on Bootloader and select the `flipper_http_bootloader.bin` that you downloaded earlier.
13. Click on Part Table and select the `flipper_http_partitions.bin` that you downloaded earlier.
14. Click on FirmwareA and select the `flipper_http_firmware_a.bin` that you downloaded earlier.
15. Click on FLASH - slow. If successful, you will see three green LED blinks on the Dev board.
16. On the Dev Board, press the RESET button once.

You are all set. Here's the initial guide: [https://www.youtube.com/watch?v=Y2lUVTMTABE](https://www.youtube.com/watch?v=AZfbrLKJMpM)

Star the repository (https://github.com/jblanked/WebCrawler-FlipperZero) and follow me for updates and upcoming Flipper apps.


## Usage in `C` (flipper_http.h)

| **Function Name**                                | **Return Value** | **Parameters**                                                                                                | **Description**                                                                                   |
|--------------------------------------------------|------------------|----------------------------------------------------------------------------------------------------------------|---------------------------------------------------------------------------------------------------|
| `flipper_http_init`                              | `bool`           | `FlipperHTTP_Callback callback`, `void *context`                                                               | Initializes the HTTP module with a callback and context.                                           |
| `flipper_http_deinit`                            | `void`           | None                                                                                                           | Deinitializes the HTTP module.                                                                    |
| `flipper_http_connect_wifi`                      | `bool`           | None                                                                                                           | Connects to WiFi using previously saved credentials.                                               |
| `flipper_http_disconnect_wifi`                   | `bool`           | None                                                                                                           | Disconnects from the current WiFi network.                                                         |
| `flipper_http_ping`                              | `bool`           | None                                                                                                           | Sends a ping request to test connectivity.                                                         |
| `flipper_http_scan_wifi`                         | `bool`           | `const char *ssid`, `const char *password`                                                                     | Scans for nearby WiFi networks.                                                                    |
| `flipper_http_save_wifi`                         | `bool`           | `const char *ssid`, `const char *password`                                                                     | Saves WiFi credentials for future connections.                                                     |
| `flipper_http_send_data`                         | `bool`           | `const char *data`                                                                                             | Sends the specified data to the server.                                                            |
| `flipper_http_rx_callback`                       | `void`           | `const char *line`, `void *context`                                                                            | Callback function for handling received data.                                                      |
| `flipper_http_get_request`                       | `bool`           | `const char *url`                                                                                              | Sends a GET request to the specified URL.                                                          |
| `flipper_http_get_request_with_headers`          | `bool`           | `const char *url`, `const char *headers`                                                                       | Sends a GET request with custom headers to the specified URL.                                      |
| `flipper_http_post_request_with_headers`         | `bool`           | `const char *url`, `const char *headers`, `const char *payload`                                                 | Sends a POST request with custom headers and a payload to the specified URL.                       |
| `flipper_http_put_request_with_headers`          | `bool`           | `const char *url`, `const char *headers`, `const char *payload`                                                 | Sends a PUT request with custom headers and a payload to the specified URL.                        |
| `flipper_http_delete_request_with_headers`          | `bool`           | `const char *url`, `const char *headers`, `const char *payload`                                                 | Sends a DELETE request with custom headers and a payload to the specified URL.                        |
| `flipper_http_save_received_data`                | `bool`           | `size_t bytes_received`, `const char line_buffer[]`                                                            | Saves the received data to the SD card, with the specified size and buffer.                        |

`In C, fhttp.received_data holds the received data from HTTP requests. In JavaScript and mPython, the response is returned directly from the function.`

## Usage in `JavaScript` (flipper_http.js):
| **Function Name**                      | **Return Value** | **Parameters**                                       | **Description**                                                                                      |
|----------------------------------------|------------------|-----------------------------------------------------|------------------------------------------------------------------------------------------------------|
| `fhttp.init`                           | `void`           | None                                                | Initializes the serial connection with the correct settings.                                          |
| `fhttp.deinit`                         | `void`           | None                                                | Deinitializes and ends the serial connection.                                                         |
| `fhttp.connect_wifi`                   | `bool`           | None                                                | Sends a command to connect to WiFi and returns whether the connection was successful.                 |
| `fhttp.disconnect_wifi`                | `bool`           | None                                                | Sends a command to disconnect from WiFi and returns whether the disconnection was successful.          |
| `fhttp.ping`                           | `bool`           | None                                                | Sends a ping request to test connectivity and returns whether a response was received.                |
| `fhttp.scan_wifi`                      | `str`           | None                                                 | Scans for nearby WiFi access points and returns a string with each access point separated by a comma               |
| `fhttp.save_wifi`                      | `bool`           | `ssid: string`, `password: string`                  | Saves WiFi credentials and returns whether the save operation was successful.                         |
| `fhttp.send_data`                      | `void`           | `data: string`                                      | Sends the specified data to the serial port.                                                          |
| `fhttp.read_data`                      | `string`         | `delay_ms: number`                                  | Reads data from the serial port with a specified delay and returns the response received.      |
| `fhttp.get_request`                    | `string`         | `url: string`                                       | Sends a GET request to the specified URL and returns the response of the response.             |
| `fhttp.get_request_with_headers`       | `string`         | `url: string`, `headers: string`                    | Sends a GET request with headers and returns the response of the response.                    |
| `fhttp.post_request_with_headers`      | `string`         | `url: string`, `headers: string`, `payload: string` | Sends a POST request with headers and payload, and returns the response of the response.       |
| `fhttp.put_request_with_headers`       | `string`         | `url: string`, `headers: string`, `payload: string` | Sends a PUT request with headers and payload, and returns the response of the response.        |
| `fhttp.delete_request_with_headers`       | `string`         | `url: string`, `headers: string`, `payload: string` | Sends a PUT request with headers and payload, and returns the response of the response.        |

## Usage in `Python` (flipper_http.py):
| **Function Name**                      | **Return Value** | **Parameters**                                       | **Description**                                                                                      |
|----------------------------------------|------------------|-----------------------------------------------------|------------------------------------------------------------------------------------------------------|
| `flipper_http_connect_wifi`                   | `bool`           | None                                                | Sends a command to connect to WiFi and returns whether the connection was successful.                 |
| `flipper_http_disconnect_wifi`                | `bool`           | None                                                | Sends a command to disconnect from WiFi and returns whether the disconnection was successful.          |
| `flipper_http_ping`                           | `bool`           | None                                                | Sends a ping request to test connectivity and returns whether a response was received.                |
| `flipper_http_save_wifi`                      | `bool`           | `ssid: str`, `password: str`                  | Saves WiFi credentials and returns whether the save operation was successful.                         |
| `flipper_http_scan_wifi`                      | `str`            | None                                          | Scans for nearby WiFi access points and returns a string with each access point separated by a comma.  |
| `flipper_http_send_data`                      | `void`           | `data: str`                                      | Sends the specified data to the serial port.                                                          |
| `flipper_http_read_data`                      | `str`         | `sleep_ms: int`                                  | Reads data from the serial port with a specified delay and returns the response received.      |
| `flipper_http_get_request`                    | `str`         | `url: str`                                       | Sends a GET request to the specified URL and returns the response of the response.             |
| `flipper_http_get_request_with_headers`       | `str`         | `url: str`, `headers: str`                    | Sends a GET request with headers and returns the response of the response.                    |
| `flipper_http_post_request_with_headers`      | `str`         | `url: str`, `headers: str`, `data: str` | Sends a POST request with headers and data, and returns the response of the response.       |
| `flipper_http_put_request_with_headers`       | `str`         | `url: str`, `headers: str`, `data: str` | Sends a PUT request with headers and data, and returns the response of the response.        |
| `flipper_http_delete_request_with_headers`       | `str`         | `url: str`, `headers: str`, `data: str` | Sends a PUT request with headers and data, and returns the response of the response.        |

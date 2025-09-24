# ESP32-Audiopad

![ESP32 Audiopad](images/esp32_audiopad_photo.jpg)

A WiFi-enabled, web-configurable audio playback device using an ESP32. This project turns an ESP32 into a simple soundboard that plays specific MP3 files when physical buttons are pressed. All configuration, file management, and testing can be done through a web interface hosted on the device itself.

## Features

*   **6-Button Audio Playback:** Connect up to 6 physical buttons to trigger MP3 playback.
*   **Web-Based Management:** No need to re-flash to change sounds. Connect to the ESP32's web server to:
    *   Upload MP3 files for each button.
    *   Delete assigned audio files.
    *   View currently assigned files.
    *   Monitor battery voltage.
    *   Remotely test button sounds.
    *   Stop any currently playing audio.
*   **Over-The-Air (OTA) Updates:** Update the firmware and filesystem (SPIFFS) over WiFi using the Arduino IDE.
*   **Deep Sleep:** Automatically enters deep sleep after a period of inactivity to conserve battery, and wakes up on a button press.
*   **I2S Audio Output:** Uses an I2S amplifier for clear digital audio playback.
*   **Battery Monitoring:** Reads voltage from an ADC pin to provide a battery level estimate on the web UI.

## Hardware Requirements

*   **ESP32 Development Board:** Any standard ESP32 board should work.
*   **I2S Audio Amplifier:** MAX98357A has been used.
*   **Speaker:** 3W speaker.
*   **Push Buttons (x6):** Cherry MX Red buttons.
*   **Resistors:** External pull-up resistors are needed for deep sleep functionality to save battery.
*   **Battery:** 1650 Lithium Ion battery with BMS
*   **Wires and a breadboard/protoboard.**

### Pinout

The following pin connections are defined in the code.

| Component         | ESP32 Pin | Description                               |
| ----------------- | --------- | ----------------------------------------- |
| **I2S Amplifier** |           |                                           |
| `BCLK`            | 4         | Bit Clock                                 |
| `LRC` / `WS`      | 2         | Left/Right Clock (Word Select)            |
| `DIN`             | 15        | Data In                                   |
| **Buttons**       |           |10k External Pull-up Resistors used |
| Button 1          | 25        |                                           |
| Button 2          | 26        |                                           |
| Button 3          | 27        |                                           |
| Button 4          | 13        |                                           |
| Button 5          | 14        |                                           |
| Button 6          | 32        |                                           |
| **Battery**       |           | For voltage monitoring         |
| `BATTERY_PIN`     | 35        | Connect to the positive terminal          |

> **Note:** The code is currently configured for buttons with external pull-up resistors (`pinMode(PIN, INPUT)`). The current debounce logic works for a button press pulling the pin LOW.

## Software Setup

1.  **Arduino IDE:** Make sure you have the Arduino IDE installed with the ESP32 board support package.

2.  **Libraries:** Install the following libraries through the Arduino Library Manager:
    *   `ESP8266Audio` by Earle F. Philhower, III (works for ESP32 as well)
    *   `WebServer` (part of the ESP32 core)
    *   `SPIFFS` (part of the ESP32 core)

3.  **WiFi Credentials:**
    *   Create a file named `secrets.h` in the same directory as your `.ino` file.
    *   Add your WiFi network credentials to this file like so:

    ```cpp
    // secrets.h
    #pragma once

    const char* ssid = "YOUR_WIFI_SSID";
    const char* password = "YOUR_WIFI_PASSWORD";
    ```
    *   The `secrets.h` file is included in `.gitignore` to prevent you from accidentally committing your credentials.

4.  **Upload Filesystem:**
    *   Before the first flash, you need to upload the filesystem image. In the Arduino IDE, go to `Tools` > `Partition Scheme` and select a scheme with SPIFFS, like "Default 4MB with spiffs (1.2MB APP/1.5MB SPIFFS)".
    *   If you have a `data` directory with files to pre-load, you can use the "ESP32 Sketch Data Upload" tool. For this project, it's not necessary as the web interface creates the `/audio` directory.

5.  **Compile and Flash:**
    *   Select your ESP32 board and COM port from the `Tools` menu.
    *   Click the "Upload" button to flash the firmware to your ESP32.

## How to Use



1.  **Power On:** Power up your ESP32. It will automatically connect to the WiFi network you specified in `secrets.h`.

2.  **Find IP Address:** Open the Arduino IDE's Serial Monitor (baud rate 115200). The ESP32 will print its IP address once connected to WiFi.

3.  **Open Web Interface:** Open a web browser on a device connected to the same network and navigate to the ESP32's IP address (e.g., `http://192.168.1.123`).

4.  **Upload Audio:**
    *   The web page will show an upload section for each of the 6 buttons.
    *   Click "Choose File" for a button, select an MP3 file from your computer, and click "Upload".
    *   The file will be named `buttonX.mp3` (where X is the button number) and stored in the ESP32's filesystem.
    *   **Note:** There is a file size limit of 500KB per file. For best results, use MP3s with a lower bitrate (e.g., 64kbps mono).

5.  **Play Sounds:**
    *   Press a physical button connected to the ESP32 to play its assigned sound.
    *   You can also use the "Test" button on the web interface to trigger playback remotely.

6.  **Manage Files:**
    *   The "Audio Files" section shows which buttons have a sound assigned.
    *   You can delete any sound file by clicking the "Delete" button next to it.

## Future Improvements
*   Add a visual indicator (e.g., an LED) to show when the device is going to sleep.
*   Implement a "Settings" page in the web UI to configure sleep timeout and other parameters.

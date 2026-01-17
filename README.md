# Wemos D1 Mini - Ultimate Dimmer Firmware

This project is a custom firmware for the Wemos D1 Mini (ESP8266) designed to control LED lighting in furniture (e.g., cabinets, wardrobes). It supports both **Online** (WiFi connected) and **Offline** (Autonomous) modes, making it versatile for any environment.

## 🌟 Features

*   **Unique Device ID**: Each board generates a unique name based on its Chip ID (e.g., `Wemos-Dimmer-A1B2C3`). No conflicts when using multiple devices!
*   **Dual Modes**:
    *   **⚡ Sensor Mode (Auto)**: Light turns ON when the sensor is triggered (door opened).
    *   **🌐 Manual Mode (Web)**: Control brightness manually via a web interface slider.
*   **Easy Setup**: Creates a `Setup-Wemos-Dimmer-XXXXXX` hotspot if no WiFi is found.
*   **OTA & Web Updates**:
    *   **OTA**: Update wirelessly via PlatformIO/Arduino IDE.
    *   **Web Update**: Upload `.bin` firmware files directly through the browser.
*   **Eco Mode**: Automatically disables WiFi radio after 15 minutes of inactivity in AP mode to save power.

## 🔌 Wiring

| Wemos Pin | Component | Description |
| :--- | :--- | :--- |
| **D1 (GPIO 5)** | **Sensor** | Connect to an IR Proximity Sensor (e.g., FC-51). |
| **D4 (GPIO 2)** | **LED / Gate** | Built-in LED or external MOSFET gate (Active LOW by default). |
| **5V / G** | **Power** | 5V Power Supply. |

> **Note:** The current logic assumes the sensor output is **LOW** when an obstacle (closed door) is detected. This can be inverted in the Web Settings.

## 🚀 Getting Started

1.  **Flash the Firmware**: Upload the `firmware.bin` to your Wemos D1 Mini.
2.  **Connect to WiFi**:
    *   Power on the device.
    *   Search for a WiFi network named `Setup-Wemos-Dimmer-XXXXXX` on your phone/PC.
    *   Connect to it. A configuration portal should open automatically (or go to `192.168.4.1`).
    *   Select your home WiFi and enter the password.
3.  **Control**:
    *   Find the device IP in your router or access it via `http://Wemos-Dimmer-XXXXXX.local`.
    *   Use the web interface to switch modes, adjust brightness, or update firmware.

## 🔄 Updating Firmware

You can update the device without connecting it to a computer:

1.  Go to the main web interface.
2.  Click the **"🔄 Update Firmware"** button at the bottom.
3.  Select the new `.bin` file and click **Update**.

## 🛠️ Offline Mode

If the device cannot connect to your home WiFi (or if you don't have one), it will create its own Access Point named `Wemos-Dimmer-XXXXXX` (password: `12345678`). You can connect to it directly to control the light.

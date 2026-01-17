# Developer Notes & Context

This file contains technical context, architectural decisions, and troubleshooting history for the Wemos Dimmer project. It serves as a memory aid for AI agents and developers.

## 🏗 Architecture Decisions

### 1. Custom Web Update Implementation
*   **Problem**: The standard `ESP8266HTTPUpdateServer` library was consistently failing with `Update error: ERROR[13]: No data supplied` when accessing the `/update` endpoint via a browser.
*   **Solution**: We replaced the library with a custom implementation using the core `Update` class directly.
*   **Details**: The custom handler (`server.on("/update", HTTP_POST, ...)`) manages the upload stream manually, which proved to be robust and error-free.

### 2. Unique Device IDs
*   **Logic**: Device name is generated at runtime: `Wemos-Dimmer-` + `ESP.getChipId()` (HEX).
*   **Purpose**: Prevents mDNS and AP name conflicts when multiple devices are powered on simultaneously.
*   **Integration**: This ID is used for:
    *   WiFi Hostname.
    *   Access Point SSID (`Wemos-Dimmer-XXXXXX`).
    *   mDNS address (`http://Wemos-Dimmer-XXXXXX.local`).
    *   Web Interface Display.

### 3. Offline / Hybrid Mode
*   **Behavior**: The device attempts to connect to WiFi on boot.
*   **Fallback**: If connection fails (or no credentials), it launches an AP (`Wemos-Dimmer-XXXXXX`).
*   **Control**: The Web Interface is fully functional in both AP and STA modes.
*   **Reset**: A "Forget WiFi" button allows the user to clear credentials and force the device back into AP mode (Offline Mode).

### 4. Multilingual Support
*   **Implementation**: Strings are stored in a JSON-like structure in JavaScript (`const texts = { en: {...}, ru: {...} }`).
*   **Storage**: Language preference (`config.lang`) is stored in EEPROM (Index 0 struct).
*   **Switching**: Dynamic DOM updates via JS, no page reload required.

## 🐛 Troubleshooting History (Windows Environment)

### 1. COM Port Locking (`PermissionError`)
*   **Issue**: PlatformIO/esptool often fails to open the COM port on Windows with `Access is denied`.
*   **Cause**: The driver (CH340) or Windows kernel holds onto the handle even after the device is disconnected, or other software (Cura, Slicers) grabs the port.
*   **Solutions**:
    *   **Best**: Use Web Update (`/update`) whenever possible.
    *   **Workaround**: Change the COM port number in Windows Device Manager (e.g., COM6 -> COM10) to force a handle reset.
    *   **Last Resort**: Reboot PC.

### 2. OTA (ArduinoOTA) Issues
*   **Issue**: OTA uploads via PlatformIO often fail with `No Answer` on Windows.
*   **Cause**: Likely Windows Firewall blocking incoming UDP packets from the ESP8266 or network discovery lag.
*   **Workaround**: Rely on the **Web Update** feature implemented in the firmware.

## 📝 Future Tasks / Backlog
*   [ ] Add smooth transition (gamma correction) for LED brightness.
*   [ ] Add support for external button (push button) on a GPIO pin.
*   [ ] Integration with MQTT / Home Assistant.

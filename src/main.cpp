#include <Arduino.h>
/*
 * Wemos D1 Mini: Ultimate Dimmer (Unique ID Version)
 * - Modes: Online (WiFi) / Offline (AP)
 * - Unique device name (Wemos-Dimmer-XXXXXX)
 * - Web Updates + OTA
 * - Multilingual Support (EN/RU)
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>
#include <WiFiManager.h> 
#include <ArduinoOTA.h>

#define LED_PIN 2       // D4 (Built-in LED, LOW=ON)
#define SENSOR_PIN 5    // D1 (Sensor)

ESP8266WebServer server(80);
WiFiManager wifiManager;

// Settings structure
struct Settings {
  int mode;              // 0 = Sensor, 1 = Web
  bool invertLogic;      // Sensor inversion
  int sensorMaxBrightness; // Brightness for sensor mode (0-1023)
  int lang;              // 0 = EN, 1 = RU
} config;

// Brightness variables
int currentBrightness = 0; 
int targetBrightness = 0;
String deviceName = "Wemos-Dimmer"; // Will be updated in setup()

// Timers
unsigned long lastActivity = 0;
const unsigned long WIFI_TIMEOUT = 15 * 60 * 1000; // 15 minutes Eco Mode

// ================= HTML PAGE =================
const char html_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Wemos Dimmer</title>
  <style>
    body { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif; background: #1a1a1a; color: #e0e0e0; text-align: center; padding: 10px; margin: 0; }
    .card { background: #2d2d2d; padding: 20px; border-radius: 16px; margin: 15px auto; max-width: 400px; box-shadow: 0 4px 6px rgba(0,0,0,0.3); }
    h2 { color: #ffd700; margin: 0 0 5px 0; font-weight: 300; }
    .dev-name { color: #666; font-size: 12px; margin-bottom: 20px; }
    
    select, button, input[type='range'], input[type='file'] { width: 100%; margin: 8px 0; box-sizing: border-box; }
    select, input[type='file'] { background: #3d3d3d; color: white; padding: 12px; border: 1px solid #444; border-radius: 8px; font-size: 16px; }
    
    .btn { padding: 14px; border: none; border-radius: 8px; font-size: 16px; font-weight: 600; cursor: pointer; transition: opacity 0.2s; }
    .btn:active { opacity: 0.8; }
    .btn-save { background: #ffd700; color: #1a1a1a; }
    .btn-wifi { background: #4a4a4a; color: #fff; margin-top: 20px; font-size: 14px; }
    
    .slider-container { position: relative; padding: 10px 0; }
    .slider { -webkit-appearance: none; height: 6px; border-radius: 3px; background: #444; outline: none; }
    .slider::-webkit-slider-thumb { -webkit-appearance: none; width: 28px; height: 28px; border-radius: 50%; background: #ffd700; cursor: pointer; box-shadow: 0 2px 4px rgba(0,0,0,0.4); }
    
    .val-text { font-size: 2.5em; font-weight: 200; color: #ffd700; display: block; margin: 10px 0; }
    .checkbox-row { display: flex; align-items: center; justify-content: space-between; background: #3d3d3d; padding: 12px; border-radius: 8px; margin: 10px 0; }
    .checkbox-row input { width: 24px; height: 24px; margin: 0; }
    
    hr { border: 0; border-top: 1px solid #444; margin: 25px 0; }
    .status { font-size: 12px; color: #666; margin-top: 10px; }
  </style>
  <script>
    const texts = {
      en: {
        title: "Light Control",
        config: "Configuration",
        mode_sensor: "⚡ Sensor Mode (Auto)",
        mode_manual: "🌐 Manual Mode (Web)",
        invert: "Invert Sensor Logic",
        max_bri: "Max Brightness:",
        save: "Save Settings",
        wifi: "📡 Configure WiFi",
        update: "🔄 Update Firmware",
        forget: "⚠️ Forget WiFi & Go Offline",
        confirm_forget: "Disconnect from Home WiFi and switch to Offline Mode?"
      },
      ru: {
        title: "Управление Светом",
        config: "Настройки",
        mode_sensor: "⚡ Автоматический (Датчик)",
        mode_manual: "🌐 Ручной (Web)",
        invert: "Инверсия Датчика",
        max_bri: "Макс. Яркость:",
        save: "Сохранить",
        wifi: "📡 Настроить WiFi",
        update: "🔄 Обновить Прошивку",
        forget: "⚠️ Забыть WiFi (Оффлайн)",
        confirm_forget: "Отключиться от домашней сети и перейти в автономный режим?"
      }
    };

    function setLang(lang) {
      document.getElementById('txt-title').innerText = texts[lang].title;
      // ... (rest of logic handles updates dynamically if elements exist)
      if(document.getElementById('txt-config')) {
          document.getElementById('txt-config').innerText = texts[lang].config;
          document.getElementById('opt-sensor').innerText = texts[lang].mode_sensor;
          document.getElementById('opt-manual').innerText = texts[lang].mode_manual;
          document.getElementById('txt-invert').innerText = texts[lang].invert;
          document.getElementById('txt-maxbri').innerText = texts[lang].max_bri;
          document.getElementById('btn-save').innerText = texts[lang].save;
          document.getElementById('btn-wifi').innerText = texts[lang].wifi;
          document.getElementById('btn-update').innerText = texts[lang].update;
          document.getElementById('btn-forget').innerText = texts[lang].forget;
          document.getElementById('frm-forget').setAttribute('onsubmit', "return confirm('" + texts[lang].confirm_forget + "');");
      }
    }

    function updateVal(val) {
      document.getElementById('bri-disp').innerText = Math.round(val/10.23) + '%';
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/set?val=" + val, true);
      xhr.send();
    }
    
    function toggleMode() {
        var val = document.getElementById('m').value;
        document.getElementById('web-ctrl').style.display = (val == '1') ? 'block' : 'none';
        document.getElementById('sensor-ctrl').style.display = (val == '0') ? 'block' : 'none';
    }

    function init() {
      if(document.getElementById('m')) toggleMode();
      var lang = document.documentElement.lang;
      // Simple check, real implementation handles config
    }
  </script>
</head>
<body onload="init()">
  <!-- MAIN PAGE -->
  <div class="card" id="main-page">
    <h2 id="txt-title">Light Control</h2>
    <div class="dev-name">%DEV_NAME%</div>
    
    <div id="web-ctrl" style="display:none">
      <span id="bri-disp" class="val-text">%CUR_PCT%%</span>
      <div class="slider-container">
        <input type="range" min="0" max="1023" value="%CUR_VAL%" class="slider" 
               oninput="document.getElementById('bri-disp').innerText = Math.round(this.value/10.23) + '%';" 
               onchange="updateVal(this.value)">
      </div>
    </div>

    <form action="/save" method="POST">
      <h3 id="txt-config">Configuration</h3>
      
      <div style="text-align:left;color:#aaa;font-size:12px;margin-bottom:5px">Language / Язык:</div>
      <select name="lang" id="l" onchange="setLang(this.value == '1' ? 'ru' : 'en')">
        <option value="0" %SEL_EN%>🇺🇸 English</option>
        <option value="1" %SEL_RU%>🇷🇺 Русский</option>
      </select>

      <select name="mode" id="m" onchange="toggleMode()">
        <option value="0" id="opt-sensor" %SEL_0%>⚡ Sensor Mode (Auto)</option>
        <option value="1" id="opt-manual" %SEL_1%>🌐 Manual Mode (Web)</option>
      </select>
      
      <div id="sensor-ctrl">
        <div class="checkbox-row">
           <span id="txt-invert">Invert Sensor Logic</span>
           <input type="checkbox" name="inv" %CHK%>
        </div>
        <div id="txt-maxbri" style="text-align:left;color:#aaa;margin-top:10px;font-size:12px">Max Brightness:</div>
        <input type="range" name="sb" min="10" max="1023" value="%S_BRI%" class="slider">
      </div>

      <button type="submit" id="btn-save" class="btn btn-save">Save Settings</button>
    </form>

    <hr>
    <form action="/wifi_setup" method="POST">
      <button id="btn-wifi" class="btn btn-wifi">📡 Configure WiFi</button>
    </form>
    <button id="btn-update" class="btn btn-wifi" style="background:#555;margin-top:10px" onclick="window.location.href='/update'">🔄 Update Firmware</button>
    
    <form id="frm-forget" action="/reset_wifi" method="POST" onsubmit="return confirm('Disconnect from Home WiFi and switch to Offline Mode?');">
      <button id="btn-forget" class="btn" style="background:#c62828; color:white; margin-top:10px">⚠️ Forget WiFi & Go Offline</button>
    </form>
    
    <div class="status">IP: %IP_ADDR%</div>
  </div>
</body>
</html>
)rawliteral";

const char update_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Update Firmware</title>
  <style>
    body { font-family: sans-serif; background: #1a1a1a; color: #e0e0e0; text-align: center; padding: 20px; }
    .card { background: #2d2d2d; padding: 20px; border-radius: 16px; margin: 0 auto; max-width: 400px; }
    h2 { color: #ffd700; }
    input[type='file'] { margin: 20px 0; background: #333; color: white; padding: 10px; width: 100%; border-radius: 8px; }
    .btn { background: #ffd700; color: #1a1a1a; padding: 12px; border: none; border-radius: 8px; font-size: 16px; font-weight: bold; cursor: pointer; width: 100%; }
    .back { display: block; margin-top: 15px; color: #aaa; text-decoration: none; }
  </style>
</head>
<body>
  <div class="card">
    <h2>Update Firmware</h2>
    <form method='POST' action='/update' enctype='multipart/form-data'>
      <input type='file' name='update'>
      <button class='btn'>Upload & Update</button>
    </form>
    <a href="/" class="back">← Back to Control</a>
  </div>
</body>
</html>
)rawliteral";

// ================= LOGIC =================

void handleRoot() {
  String html = html_page;
  
  if(config.mode == 0) { html.replace("%SEL_0%", "selected"); html.replace("%SEL_1%", ""); }
  else { html.replace("%SEL_0%", ""); html.replace("%SEL_1%", "selected"); }
  
  if(config.lang == 1) { html.replace("%SEL_EN%", ""); html.replace("%SEL_RU%", "selected"); }
  else { html.replace("%SEL_EN%", "selected"); html.replace("%SEL_RU%", ""); }

  html.replace("%CHK%", config.invertLogic ? "checked" : "");
  html.replace("%S_BRI%", String(config.sensorMaxBrightness));
  html.replace("%CUR_VAL%", String(targetBrightness));
  html.replace("%CUR_PCT%", String(targetBrightness / 10));
  html.replace("%IP_ADDR%", WiFi.localIP().toString());
  html.replace("%DEV_NAME%", deviceName);

  server.send(200, "text/html", html);
  lastActivity = millis();
}

void handleSave() {
  if (server.hasArg("mode")) config.mode = server.arg("mode").toInt();
  if (server.hasArg("lang")) config.lang = server.arg("lang").toInt();
  config.invertLogic = server.hasArg("inv");
  if (server.hasArg("sb")) config.sensorMaxBrightness = server.arg("sb").toInt();

  EEPROM.put(0, config);
  EEPROM.commit();
  server.sendHeader("Location", "/");
  server.send(303);
  lastActivity = millis();
}

void handleSetBrightness() {
  if (server.hasArg("val")) {
    int val = server.arg("val").toInt();
    if (config.mode == 1) targetBrightness = val;
    server.send(200, "text/plain", "OK");
    lastActivity = millis();
  } else {
    server.send(400, "text/plain", "Bad Request");
  }
}

// Start WiFiManager ON DEMAND
void handleWiFiSetup() {
  String apName = "Setup-" + deviceName;
  server.send(200, "text/html", "<h1>Starting WiFi Setup...</h1><p>Connect to '" + apName + "' if connection is lost.</p><script>setTimeout(function(){window.location.href='/';}, 10000);</script>");
  delay(500);
  
  server.stop(); 
  
  WiFiManager wm;
  wm.setTimeout(180); 
  
  if (!wm.startConfigPortal(apName.c_str())) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    ESP.restart(); 
  }
  
  Serial.println("connected...yeey :)");
  ESP.restart();
}

void handleResetWiFi() {
  server.send(200, "text/html", "<h1>WiFi settings cleared!</h1><p>Restarting into Offline Mode...</p>");
  delay(1000);
  wifiManager.resetSettings();
  ESP.restart();
}

void updateFading() {
  if (currentBrightness < targetBrightness) {
    currentBrightness += 20; 
    if (currentBrightness > targetBrightness) currentBrightness = targetBrightness;
  } 
  else if (currentBrightness > targetBrightness) {
    currentBrightness -= 20; 
    if (currentBrightness < targetBrightness) currentBrightness = targetBrightness;
  }
  analogWrite(LED_PIN, 1023 - currentBrightness);
}

void setup() {
  Serial.begin(115200);
  EEPROM.begin(sizeof(config));
  
  pinMode(LED_PIN, OUTPUT);
  pinMode(SENSOR_PIN, INPUT);

  EEPROM.get(0, config);
  // Basic validation to prevent EEPROM garbage issues after struct update
  if (config.mode < 0 || config.mode > 1) {
    config.mode = 0;
    config.invertLogic = false;
    config.sensorMaxBrightness = 1023;
    config.lang = 0; // Default to English
  }
  if (config.lang < 0 || config.lang > 1) config.lang = 0;

  // --- UNIQUE ID GENERATION ---
  String chipId = String(ESP.getChipId(), HEX);
  chipId.toUpperCase();
  deviceName = "Wemos-Dimmer-" + chipId;
  
  Serial.println("Device Name: " + deviceName);

  // --- WiFi Setup ---
  WiFi.hostname(deviceName);
  WiFi.mode(WIFI_STA); 
  WiFi.begin();        

  Serial.print("Connecting to WiFi");
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 20) {
    delay(500);
    Serial.print(".");
    retries++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected! IP: " + WiFi.localIP().toString());
  } else {
    Serial.println("\nFailed to connect. Starting Offline AP.");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(deviceName.c_str(), "12345678"); 
    Serial.println("AP IP: " + WiFi.softAPIP().toString());
  }

  if (MDNS.begin(deviceName.c_str())) {
    Serial.println("mDNS responder started: " + deviceName + ".local");
  }

  // Setup update server (Custom Implementation)
  server.on("/update", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", update_page);
  });
  
  server.on("/update", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      WiFiUDP::stopAll();
      Serial.printf("Update: %s\n", upload.filename.c_str());
      uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
      if (!Update.begin(maxSketchSpace)) { 
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { 
        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
    }
  });
  
  // Setup OTA (via IDE)
  ArduinoOTA.setHostname(deviceName.c_str());
  ArduinoOTA.begin();

  server.on("/", handleRoot);
  server.on("/save", handleSave);
  server.on("/set", handleSetBrightness);
  server.on("/wifi_setup", handleWiFiSetup);
  server.on("/reset_wifi", handleResetWiFi);
  
  server.begin();
  Serial.println("HTTP server started");
  lastActivity = millis();
}

void loop() {
  server.handleClient();
  ArduinoOTA.handle();
  MDNS.update();

  if (WiFi.getMode() == WIFI_AP) {
     if (WiFi.softAPgetStationNum() == 0 && (millis() - lastActivity > WIFI_TIMEOUT)) {
        // WiFi.mode(WIFI_OFF); 
     }
     if (WiFi.softAPgetStationNum() > 0) lastActivity = millis();
  }

  if (config.mode == 0) { 
    bool sensorSignal = digitalRead(SENSOR_PIN); 
    bool doorOpen = (sensorSignal == HIGH); 
    if (config.invertLogic) doorOpen = !doorOpen;

    if (doorOpen) targetBrightness = config.sensorMaxBrightness;
    else targetBrightness = 0;
  }
  
  updateFading();
  delay(10);
}

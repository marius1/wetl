#include <ESP8266mDNS.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <FS.h>
#include <EEPROM.h>

#define LIGHT_TOGGLE_PIN 2
#define LIGHT_RED LOW
#define LIGHT_GREEN HIGH

bool lightState = LIGHT_RED;
bool apMode = false;
bool tryReconnect = false;
ESP8266WebServer server(80);

void setLight(bool state) {
    lightState = state;
    digitalWrite(LIGHT_TOGGLE_PIN, state);
}

void handleGetLightState() {
    String state = (lightState == LIGHT_GREEN) ? "green" : "red";
    server.send(200, "text/json", "{\"state\":\"" + state + "\"}");
}

void handlePostLightState() {
    if(server.args() == 0) {
        return server.send(500, "text/plain", "BAD ARGS");
    }

    String state = server.arg(0);

    if (state == "green") {
        setLight(LIGHT_GREEN);
    } else {
        setLight(LIGHT_RED);
    }

    handleGetLightState();
}

void handleGetWifiApList() {
    String response = "[";
    uint8 n = WiFi.scanNetworks();

    Serial.printf("Found %d networks\n", n);

    for(uint8 i = 0; i < n; i++) {
        response += "{\"ssid\":\"" + WiFi.SSID(i) + "\", \"rssi\": \"" + WiFi.RSSI(i) + "\"}";
        if (i != n-1) {
            response += ",";
        }        
    }

    response += "]";

    server.send(200, "text/json", response);
}

void handlePostWifiSettings() {
    if(server.args() < 2) {
        return server.send(500, "text/plain", "BAD ARGS");
    }

    String ssid = server.arg("ssid");
    String password = server.arg("password");

    if(ssid.length() > 31 || password.length() > 63) {
        return server.send(500, "text/plain", "BAD ARGS");
    }

    Serial.print("Save called with ssid: '");
    Serial.print(ssid);
    Serial.print("' and password: '");
    Serial.print(password);
    Serial.println("'");

    EEPROM.begin(96);

    uint8_t position = 0, i = 0;    
    char ssidChars[sizeof(ssid)];
    ssid.toCharArray(ssidChars, sizeof(ssid));
    while(i < sizeof(ssid)) {
        EEPROM.write(position++, ssidChars[i++]);
    }
    EEPROM.write(position, '\0');

    position = 32, i =0;
    char passwordChars[sizeof(password)];
    password.toCharArray(passwordChars, sizeof(password));
    while(i < sizeof(ssid)) {
        EEPROM.write(position++, password[i++]);
    }
    EEPROM.write(position, '\0');

    EEPROM.commit();
    EEPROM.end();

    server.send(200, "text/plain", "Settings saved. Trying to connect, if successfull you will lose connection.");

    tryReconnect = true;
}

void setupWebServer() {
    server.on("/", []() {
        server.sendHeader("Location", String("/index.html"), true);
        server.send( 302, "text/plain", "");
    });

    server.on("/light", HTTP_GET, handleGetLightState);  
    server.on("/light", HTTP_POST, handlePostLightState);
    
    server.on("/wifi/list", HTTP_GET, handleGetWifiApList);
    server.on("/wifi/save", HTTP_POST, handlePostWifiSettings);

    server.serveStatic("/", SPIFFS, "/", "max-age=86400");
    
    server.begin();
    Serial.println("HTTP server started");
}

void setupMdns() {
    MDNS.begin("wetl");
    MDNS.addService("http", "tcp", 80);
}

void setupApMode() {
    WiFi.disconnect();
    WiFi.mode(WIFI_AP);

    char tmp[11];
    sprintf(tmp, "wetl-%06x", ESP.getChipId());

    WiFi.softAP(tmp);
    apMode = true;

    setLight(LIGHT_RED);
}

bool connect() {
    uint8 timeout = 0;

    EEPROM.begin(96);

    String ssid;
    for (uint8 i = 0; i < 32; i++) {
        char r = char(EEPROM.read(i));
        if (r == '\0') break;
        ssid += r;
    }

    String password;
    for (uint8 i = 32; i < 96; i++) {
        char r = char(EEPROM.read(i));
        if (r == '\0') break;
        password += r;
    }
    EEPROM.end();

    Serial.print("SSID: "); Serial.println(ssid);
    Serial.print("Password: "); Serial.println(password);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        timeout++;

        if (timeout == 60) {
            return false;
        }
    }

    Serial.println("");
    Serial.print("Connected to: ");  
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    apMode = false;

    return true;
}

void setup() {
    pinMode(LIGHT_TOGGLE_PIN, OUTPUT);
    setLight(LIGHT_RED);

    WiFi.disconnect();

    Serial.begin(115200);    
    SPIFFS.begin();

    setupWebServer();
    setupMdns();

    delay(10);
    Serial.println("");
}

void loop() {
    if ((WiFi.status() != WL_CONNECTED && apMode == false) || tryReconnect) {
        tryReconnect = false;
        if (connect() == false) {
            Serial.println("\nFailed to connect, settup up AP mode");
            setupApMode();
        }
    }

    server.handleClient();
}

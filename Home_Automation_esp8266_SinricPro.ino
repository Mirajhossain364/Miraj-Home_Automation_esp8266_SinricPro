// Uncomment the following line to enable serial debug output
#define ENABLE_DEBUG

#ifdef ENABLE_DEBUG
    #define DEBUG_ESP_PORT Serial
    #define NODEBUG_WEBSOCKETS
    #define NDEBUG
#endif 

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "SinricPro.h"
#include "SinricProSwitch.h"

#include <map>

// Updated Wi-Fi credentials and API keys
#define WIFI_SSID         "Soulmates"    
#define WIFI_PASS         "1234512345"
#define APP_KEY           "1fb9c9b2-d941-4a37-aad0-efac9913dd53"
#define APP_SECRET        "5322f984-26f4-42f6-b5bd-46988b867584-b4067011-f69b-4a81-a3f9-0e359be3c3b4"

// New device IDs
#define DEVICE_1   "SWITCH_ID_NEW_1"
#define DEVICE_2   "SWITCH_ID_NEW_2"
#define DEVICE_3   "SWITCH_ID_NEW_3"
#define DEVICE_4   "SWITCH_ID_NEW_4"

// New GPIO Pin Assignments
#define RELAY_PIN_1 13   // D7
#define RELAY_PIN_2 2    // D4
#define RELAY_PIN_3 15   // D8
#define RELAY_PIN_4 14   // D5

#define SWITCH_PIN_1 12  // D6
#define SWITCH_PIN_2 0   // D3
#define SWITCH_PIN_3 4   // D2
#define SWITCH_PIN_4 5   // D1

#define WIFI_LED_PIN 16  // D0

#define BAUD_RATE 9600
#define DEBOUNCE_TIME 300

// Struct for managing relay and switch pin configuration
typedef struct {
    int relayPin;
    int switchPin;
} deviceConfig;

std::map<String, deviceConfig> devices = {
    {DEVICE_1, {RELAY_PIN_1, SWITCH_PIN_1}},
    {DEVICE_2, {RELAY_PIN_2, SWITCH_PIN_2}},
    {DEVICE_3, {RELAY_PIN_3, SWITCH_PIN_3}},
    {DEVICE_4, {RELAY_PIN_4, SWITCH_PIN_4}}
};

// Struct to manage flip switch state, debounce, and device ID
typedef struct {
    String deviceId;
    bool lastSwitchState;
    unsigned long lastSwitchChangeTime;
} flipSwitchConfig;

std::map<int, flipSwitchConfig> flipSwitches;

// Initialize relay pins
void configureRelays() {
    for (auto &device : devices) {
        pinMode(device.second.relayPin, OUTPUT);
        digitalWrite(device.second.relayPin, HIGH);
    }
}

// Initialize flip switch pins
void configureFlipSwitches() {
    for (auto &device : devices) {
        flipSwitchConfig switchConfig;
        switchConfig.deviceId = device.first;
        switchConfig.lastSwitchChangeTime = 0;
        switchConfig.lastSwitchState = true;

        pinMode(device.second.switchPin, INPUT_PULLUP);
        flipSwitches[device.second.switchPin] = switchConfig;
    }
}

// Power state change handler for SinricPro
bool onPowerChange(String deviceId, bool &state) {
    int relayPin = devices[deviceId].relayPin;
    digitalWrite(relayPin, !state);
    return true;
}

// Handle flip switch debounce and state changes
void manageFlipSwitches() {
    unsigned long currentMillis = millis();
    
    for (auto &flipSwitch : flipSwitches) {
        unsigned long lastChange = flipSwitch.second.lastSwitchChangeTime;

        if (currentMillis - lastChange > DEBOUNCE_TIME) {
            int switchPin = flipSwitch.first;
            bool currentSwitchState = digitalRead(switchPin);
            bool lastSwitchState = flipSwitch.second.lastSwitchState;

            if (currentSwitchState != lastSwitchState) {
                flipSwitch.second.lastSwitchChangeTime = currentMillis;
                String deviceId = flipSwitch.second.deviceId;
                int relayPin = devices[deviceId].relayPin;
                bool newRelayState = !digitalRead(relayPin);
                digitalWrite(relayPin, newRelayState);

                SinricProSwitch &switchDevice = SinricPro[deviceId];
                switchDevice.sendPowerStateEvent(!newRelayState);

                flipSwitch.second.lastSwitchState = currentSwitchState;
            }
        }
    }
}

// Wi-Fi connection setup
void connectToWiFi() {
    Serial.println("\r\n[WiFi]: Connecting...");
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(250);
    }
    digitalWrite(WIFI_LED_PIN, LOW);
    Serial.printf("Connected to Wi-Fi! IP Address: %s\r\n", WiFi.localIP().toString().c_str());
}

// SinricPro setup to manage devices
void setupSinricPro() {
    for (auto &device : devices) {
        const char *deviceId = device.first.c_str();
        SinricProSwitch &switchDevice = SinricPro[deviceId];
        switchDevice.onPowerState(onPowerChange);
    }

    SinricPro.begin(APP_KEY, APP_SECRET);
    SinricPro.restoreDeviceStates(true);
}

void setup() {
    Serial.begin(BAUD_RATE);
    pinMode(WIFI_LED_PIN, OUTPUT);
    digitalWrite(WIFI_LED_PIN, HIGH);

    configureRelays();
    configureFlipSwitches();
    connectToWiFi();
    setupSinricPro();
}

void loop() {
    SinricPro.handle();
    manageFlipSwitches();
}

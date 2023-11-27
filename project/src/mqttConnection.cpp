#include <M5Stack.h>
#include "M5_ENV.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <vector>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include "secrets.h"

#define MQTT_TOPIC "IOT"

uint32_t now;

WiFiClient wifiClient; 
PubSubClient client(wifiClient);
TFT_eSprite tftSprite = TFT_eSprite(&M5.Lcd);
IPAddress server; // IP address of the MQTT server

const char* ssid = SECRET_SSID; // name of the wifi network
const char* password = SECRET_PASS; // password of the wifi network
bool addr = server.fromString(MQTT_SERVER);
const int port = MQTT_PORT; // port of the MQTT server
String mqttGlobalTopic = MQTT_TOPIC;

long lastReconnectAttempt = 0;
String macAddress = "";
const int ledPin = 26;
bool state = false;
bool draw = false;

void drawScreen() {
    tftSprite.fillScreen(TFT_BLACK);
    tftSprite.setTextColor(TFT_WHITE);
    tftSprite.setFont(&FreeMonoBold9pt7b);
    tftSprite.setTextDatum(MC_DATUM);
    tftSprite.drawString(String("MAC: ") + macAddress.c_str(), 0, 10);
    tftSprite.drawString(String("Wifi Connection: ") + (WiFi.isConnected() ? "OK" : "ERROR"), 0, 30);
    tftSprite.drawString(String("MQTT Connection: ") + (client.connected() ? "OK" : "ERROR"), 0, 50);
    tftSprite.drawString(String("Output state: ") + (state? "ON" : "OFF"), 0, 70);
    tftSprite.pushSprite(0,0);
}

void callback(char* topic, byte* payload, unsigned int length) {
    if(String(topic) == mqttGlobalTopic+"/"+macAddress+"/action") {
        String action = String(payload, length);
        Serial.println("Get action: " + action);
        digitalWrite(ledPin, digitalRead(ledPin) == HIGH? LOW : HIGH);
        state = state? false : true;
        drawScreen();
    }
}

boolean reconnect(){
    Serial.println("Connection to MQTT server at " + server.toString() + ":" + String(port));
    if(client.connect(WiFi.macAddress().c_str())) {
        String subscribeTopic = String(mqttGlobalTopic)+"/"+macAddress+"/#";
        Serial.println("M5Stack subscribed to the topic " + subscribeTopic);
        client.subscribe(subscribeTopic.c_str());
    }
    return client.connected();
}

void setup() {
	M5.begin();
	M5.Power.begin();
    now = millis();

    tftSprite.setColorDepth(8);
    tftSprite.createSprite(320, 240);
    tftSprite.fillScreen(TFT_BLACK);
    tftSprite.setTextColor(TFT_WHITE);
    tftSprite.setFont(&FreeMonoBold9pt7b);

    WiFi.begin(ssid, password);
    WiFi.setAutoReconnect(true);
    int i = 0;
    while (WiFi.status()!= WL_CONNECTED) {
        tftSprite.fillScreen(TFT_BLACK);
        tftSprite.drawString(String("Connection to WiFi: ") + ssid, 0, 10);
        i++;
        Serial.print(".");
        tftSprite.drawString(std::string(i%3+1, '.').c_str(), 0, 30);
        tftSprite.pushSprite(0,0);

        delay(500);
    }

    Serial.println("Wifi connected to address: " + WiFi.localIP().toString());
    macAddress = WiFi.macAddress();
    
    client.setServer(server, port);
    client.setCallback(callback);
    
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LOW);
    state = false;
    draw = false;

    drawScreen();
    Serial.println("End setup"); 
}

void loop() {
    if(!WiFi.isConnected()) {
        now = millis();
        if (now - lastReconnectAttempt > 5000) {
            Serial.println("WiFi lost. Reconnect...");
            lastReconnectAttempt = now;
            WiFi.reconnect();
            drawScreen();
        }
    }
    else if (!client.connected()) {
        now = millis();
        if (now - lastReconnectAttempt > 5000) {
            lastReconnectAttempt = now;
            // Attempt to reconnect
            if (reconnect()) {
                lastReconnectAttempt = 0;
            }
            drawScreen();
            draw = true;
        }
    } else {
        if(draw) {
            draw = false;
            drawScreen();
        }
        // Client connected
        client.loop();
        String mqttTopic = mqttGlobalTopic + String("/") + WiFi.macAddress();
        // client.publish(mqttTopic.c_str(), "value");
    }
}
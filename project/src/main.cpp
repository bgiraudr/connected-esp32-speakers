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

int scanTime = 5; // durée du scan en secondes
BLEScan *pBLEScan;

#define MQTT_TOPIC "IOT"

uint32_t now;

WiFiClient wifiClient;
PubSubClient client(wifiClient);
TFT_eSprite tftSprite = TFT_eSprite(&M5.Lcd);
IPAddress server; // IP address of the MQTT server

const char *ssid = SECRET_SSID;     // name of the wifi network
const char *password = SECRET_PASS; // password of the wifi network
bool addr = server.fromString(MQTT_SERVER);
const int port = MQTT_PORT; // port of the MQTT server
String mqttGlobalTopic = MQTT_TOPIC;

long lastReconnectAttempt = 0;
String macAddress = "";
const int ledPin = 26;
bool state = false;
bool draw = false;

bool deviceFound = false;
bool instructionReceived = false;
int rssiValue;

bool bleConnected;

#define M5NAME "M1";

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
    void onResult(BLEAdvertisedDevice advertisedDevice)
    {
        if (advertisedDevice.getName() == "M5Stack")
        {
            deviceFound = true;
            rssiValue = advertisedDevice.getRSSI();
        }
    }
};

void drawScreen()
{
    tftSprite.fillScreen(TFT_BLACK);
    tftSprite.setTextColor(TFT_WHITE);
    tftSprite.setFont(&FreeMonoBold9pt7b);
    tftSprite.setTextDatum(MC_DATUM);
    tftSprite.drawString(String("MAC: ") + macAddress.c_str(), 0, 10);
    tftSprite.drawString(String("Wifi Connection: ") + (WiFi.isConnected() ? "OK" : "ERROR"), 0, 30);
    tftSprite.drawString(String("MQTT Connection: ") + (client.connected() ? "OK" : "ERROR"), 0, 50);
    tftSprite.drawString(String("Output state: ") + (state ? "ON" : "OFF"), 0, 70);
    tftSprite.pushSprite(0, 0);
}

boolean reconnect()
{
    Serial.println("Connection to MQTT server at " + server.toString() + ":" + String(port));
    if (client.connect(WiFi.macAddress().c_str()))
    {
        String subscribeTopic = String(mqttGlobalTopic) + "/" + macAddress + "/#";
        Serial.println("M5Stack subscribed to the topic " + subscribeTopic);
        client.subscribe(subscribeTopic.c_str());
    }
    return client.connected();
}

void callback(char *topic, byte *payload, unsigned int length)
{
    if (String(topic) == mqttGlobalTopic + "/BEST")
    {
        String action = String(payload, length);
        Serial.println("Get action: " + action);
        if(action.equals("M1")) {
            state = true;
        } else {
            state = false;
        }

        // digitalWrite(ledPin, digitalRead(ledPin) == HIGH ? LOW : HIGH);
        // state = state ? false : true;
        // drawScreen();
        instructionReceived = true;
    }
}

void setup()
{
    Serial.begin(115200);
    M5.begin();
    M5.Power.begin();

    tftSprite.setColorDepth(8);
    tftSprite.createSprite(320, 240);
    tftSprite.fillScreen(TFT_BLACK);
    tftSprite.setTextColor(TFT_WHITE);
    tftSprite.setFont(&FreeMonoBold9pt7b);

    BLEDevice::init("");
    pBLEScan = BLEDevice::getScan(); // créer un nouveau scan
    pBLEScan->setActiveScan(true);   // active scan utilise plus de puissance, mais obtient des résultats plus rapidement
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99); // moins d'intervalles minimise la probabilité de collision
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());

    client.setServer(server, port);
    client.setCallback(callback);

    bleConnected = true;

    drawScreen();
}

void loop()
{
    // bluetooth scan until the device is found
    if(!deviceFound) {
        // lancer un nouveau scan
        BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
        Serial.print("Devices found: ");
        Serial.println(foundDevices.getCount());
        Serial.println("Scan done!");
    } else {
        // the device has been found, we need to send signal to mqtt{
        // deconnect the bluetooth
        BLEDevice::deinit();

        // connect to wifi
        WiFi.begin(ssid, password);
        WiFi.setAutoReconnect(true);
        while (WiFi.status() != WL_CONNECTED)
        {
            tftSprite.fillScreen(TFT_BLACK);
            tftSprite.drawString(String("Connection to WiFi: ") + ssid, 0, 10);
            tftSprite.pushSprite(0, 0);
            delay(500);
        }

        Serial.println("Wifi connected to address: " + WiFi.localIP().toString());
        macAddress = WiFi.macAddress();
        
        Serial.println("Connection to MQTT server at " + server.toString() + ":" + String(port));
        if (client.connect(WiFi.macAddress().c_str()))
        {
            String subscribeTopic = String(mqttGlobalTopic) + "/" + macAddress + "/#";
            Serial.println("M5Stack subscribed to the topic " + subscribeTopic);
            client.subscribe(subscribeTopic.c_str());
        }

        String mqttTopic = mqttGlobalTopic + String("/") + "M1";
        // send to the mqtt client the power value
        client.publish(mqttTopic.c_str(), std::to_string(rssiValue*-1).c_str());
        Serial.println(String(rssiValue) + " envoyé au serveur ! " + mqttTopic);

        // wait for instruction callback
        client.setServer(server, port);
        client.setCallback(callback);
        int m = 0;
        while(!instructionReceived) {
            Serial.println("waiting for instruction " + String(m));
            m+=1;
            client.loop();
            delay(500);
            if(m>=20) break;
        }
        Serial.println("Instruction Received !");
        client.disconnect();
        WiFi.disconnect(true, false);
        delay(200);

        // reconnect the bluetooth
        BLEDevice::init("");
        deviceFound = false;
        pBLEScan = BLEDevice::getScan(); // créer un nouveau scan
        pBLEScan->setActiveScan(true);   // active scan utilise plus de puissance, mais obtient des résultats plus rapidement
        pBLEScan->setInterval(100);
        pBLEScan->setWindow(99); // moins d'intervalles minimise la probabilité de collision
        pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
        instructionReceived = false;
    }
}
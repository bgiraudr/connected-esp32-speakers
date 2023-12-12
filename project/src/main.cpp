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

int scanTime = 2; // durée du scan en secondes
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
const int speakerPin = 25;
bool state = false;
bool draw = false;

bool deviceFound = false;
bool instructionReceived = false;
int rssiValue;

bool bleConnected;

#define M5NAME "M1";

int melody[] = {
  261, 293, 329, 349, 391, 440, 493,
  523, 493, 440, 391, 349, 329, 293,
  261, 261, 293, 293, 329, 293, 329, 349, 391, 440, 493,
  523, 493, 440, 391, 349, 329, 293
};

// Durée de chaque note
int noteDurations[] = {
  4, 4, 4, 4, 4, 4, 4,
  4, 4, 4, 4, 4, 4, 4,
  4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
  4, 4, 4, 4, 4, 4, 4
};

const int volumeFactor = 4;

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
    void onResult(BLEAdvertisedDevice advertisedDevice)
    {
        if (advertisedDevice.getName() == "M5Stack")
        {
            Serial.println("M5 Stack found");
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
    tftSprite.drawString(String("M5 Number: M1"), 0, 90);
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

void play_music() {
    for (int thisNote = 0; thisNote < sizeof(melody)-1; thisNote++) {
      // Calculer la durée de la note
      int noteDuration = 1000 / (noteDurations[thisNote] * volumeFactor);
      M5.Speaker.tone(melody[thisNote], noteDuration);

      // Pause entre les notes
      int pauseBetweenNotes = noteDuration * 1.30;
      delay(pauseBetweenNotes);
      //noTone(speakerPin); // Arrêter la note
    }

    M5.Speaker.mute();
}

void callback(char *topic, byte *payload, unsigned int length)
{
    if (String(topic) == mqttGlobalTopic + "/BEST")
    {
        String action = String(payload, length);
        Serial.println("Get action: " + action);
        if(action.equals("IOT/M1")) {
            state = true;
            play_music();
        } else {
            state = false;
        }

        Serial.println(state);
        // digitalWrite(ledPin, state ? HIGH : LOW);
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

    //set pin 26 to output
    pinMode(ledPin, OUTPUT);
    pinMode(speakerPin, OUTPUT); //setup du buzzer

    bleConnected = true;

    drawScreen();
}

int cpt = 0;

void loop()
{
    cpt++;
    Serial.println(cpt);
    // bluetooth scan until the device is found
    if(!deviceFound) {
        // lancer un nouveau scan
        // pBLEScan->stop();
        BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
        Serial.print("Devices found: ");
        Serial.println(foundDevices.getCount());
        Serial.println("Scan done!");
    } else {
        // the device has been found, we need to send signal to mqtt{
        // deconnect the bluetooth
        Serial.println("I'm HERE !");
        BLEDevice::deinit();

        delay(200);

        // connect to wifi
        WiFi.begin(ssid, password);
        int val = 0;
        while (WiFi.status() != WL_CONNECTED)
        {
            tftSprite.fillScreen(TFT_BLACK);
            tftSprite.drawString(String("Connection to WiFi: ") + ssid, 0, 10);
            tftSprite.pushSprite(0, 0);
            Serial.println("Trying to connect to Wifi");
            val++;
            delay(500);
            if(val > 15) {
                Serial.println("Force reload wifi");
                WiFi.begin(ssid, password);
                val = 0;
            }
        }

        Serial.println("Wifi connected to address: " + WiFi.localIP().toString());
        macAddress = WiFi.macAddress();
        
        Serial.println("Connection to MQTT server at " + server.toString() + ":" + String(port));
        if (client.connect(WiFi.macAddress().c_str()))
        {
            String subscribeTopic = String(mqttGlobalTopic) + "/" + "BEST" + "/#";
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
        if(m<20) Serial.println("Instruction Received !");
        client.disconnect();
        WiFi.disconnect(true, false);
        delay(200);

        // reconnect the bluetooth
        BLEDevice::init("");
        deviceFound = false;
        pBLEScan = BLEDevice::getScan(); // créer un nouveau scan
        // pBLEScan->stop();
        pBLEScan->setActiveScan(true);   // active scan utilise plus de puissance, mais obtient des résultats plus rapidement
        pBLEScan->setInterval(100);
        pBLEScan->setWindow(99); // moins d'intervalles minimise la probabilité de collision
        pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
        instructionReceived = false;
    }
}
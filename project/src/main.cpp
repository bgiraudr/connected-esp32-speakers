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
#include "pitches.h"

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

String macAddress = "";

const int speakerPin = 25;

bool deviceFound = false;
bool instructionReceived = false;
int rssiValue;

#define M5NAME "M1";


std::vector<std::vector<int>> melodyVector = {
{ // pacman
NOTE_B4, NOTE_B5, NOTE_FS5, NOTE_DS5,
NOTE_B5, NOTE_FS5, NOTE_DS5, NOTE_C5,
NOTE_C6, NOTE_G6, NOTE_E6, NOTE_C6, NOTE_G6, NOTE_E6,

NOTE_B4, NOTE_B5, NOTE_FS5, NOTE_DS5, NOTE_B5,
NOTE_FS5, NOTE_DS5, NOTE_DS5, NOTE_E5, NOTE_F5,
NOTE_F5, NOTE_FS5, NOTE_G5, NOTE_G5, NOTE_GS5, NOTE_A5, NOTE_B5
},
{ // its a small world
NOTE_E4, NOTE_F4, NOTE_G4, NOTE_E5, NOTE_C5, NOTE_D5, NOTE_C5, NOTE_C5, NOTE_B4, NOTE_B4, NOTE_D4,
  NOTE_E4, NOTE_F4, NOTE_D5, NOTE_B4, NOTE_C5, NOTE_B4, NOTE_A4, NOTE_G4, NOTE_G4, NOTE_E4, NOTE_F4, 
  NOTE_G4, NOTE_C5, NOTE_D5, NOTE_E5, NOTE_D5, NOTE_C5, NOTE_A4, NOTE_D5, NOTE_E5, NOTE_F5, NOTE_E5, 
  NOTE_D5, NOTE_G4, NOTE_F5, NOTE_E5, NOTE_D5, NOTE_C5, NOTE_C5, REST, NOTE_C5, REST, NOTE_C5, NOTE_E5, 
  NOTE_C5, NOTE_D5, REST, NOTE_D5, NOTE_D5, NOTE_D5, REST, NOTE_D5, NOTE_F5, NOTE_D5, NOTE_E5, REST, NOTE_E5, 
  NOTE_E5, NOTE_E5, REST, NOTE_E5, NOTE_G5, NOTE_E5, NOTE_F5, REST, NOTE_F5, NOTE_F5, NOTE_E5, NOTE_D5, NOTE_G4, 
  NOTE_B4, NOTE_C5, NOTE_C5, REST
},
{ //xmas
    NOTE_E5, NOTE_E5, NOTE_E5,
  NOTE_E5, NOTE_E5, NOTE_E5,
  NOTE_E5, NOTE_G5, NOTE_C5, NOTE_D5,
  NOTE_E5,
  NOTE_F5, NOTE_F5, NOTE_F5, NOTE_F5,
  NOTE_F5, NOTE_E5, NOTE_E5, NOTE_E5, NOTE_E5,
  NOTE_E5, NOTE_D5, NOTE_D5, NOTE_E5,
  NOTE_D5, NOTE_G5
},
{//la panthere rose
REST, REST, REST, NOTE_DS4, 
  NOTE_E4, REST, NOTE_FS4, NOTE_G4, REST, NOTE_DS4,
  NOTE_E4, NOTE_FS4,  NOTE_G4, NOTE_C5, NOTE_B4, NOTE_E4, NOTE_G4, NOTE_B4,   
  NOTE_AS4, NOTE_A4, NOTE_G4, NOTE_E4, NOTE_D4, 
  NOTE_E4, REST, REST, NOTE_DS4,

  REST, NOTE_E5, NOTE_D5, NOTE_B4, NOTE_A4, NOTE_G4, NOTE_E4,
  NOTE_AS4, NOTE_A4, NOTE_AS4, NOTE_A4, NOTE_AS4, NOTE_A4, NOTE_AS4, NOTE_A4,   
  NOTE_G4, NOTE_E4, NOTE_D4, NOTE_E4, NOTE_E4, NOTE_E4
}
};

// Tableau noteDurations en vecteur
std::vector<std::vector<int>> noteDurationsVector = {
{ //pacman
16, 16, 16, 16,
32, 16, 8, 16,
16, 16, 16, 32, 16, 8,

16, 16, 16, 16, 32,
16, 8, 32, 32, 32,
32, 32, 32, 32, 32, 16, 8
}, 
{ // its a small world
8, 8, 4, 4, 4, 8, 8, 4, 4, 4, 8, 8, 4, 4, 4, 8, 8, 4, 4, 4, 8, 8, 4, 8, 8, 4, 8, 8, 4, 
  8, 8, 4, 8, 8, 4, 4, 4, 4, 2, 4, 4, 4, 8, 8, 4, 4, 4, 8, 8, 2, 4, 8, 8, 4, 4, 4, 8, 8, 
  2, 4, 8, 8, 4, 4, 4, 8, 8, 4, 8, 8, 2, 2, 2, 4, 4
},
{ //xmas
  8, 8, 4,
  8, 8, 4,
  8, 8, 8, 8,
  2,
  8, 8, 8, 8,
  8, 8, 8, 16, 16,
  8, 8, 8, 8,
  4, 4
},
{ // la panthere rose
      2, 4, 8, 8, 
  4, 8, 8, 4, 8, 8,
  8, 8,  8, 8, 8, 8, 8, 8,   
  2, 16, 16, 16, 16, 
  2, 4, 8, 4,

  4, 8, 8, 8, 8, 8, 8,
  16, 8, 16, 8, 16, 8, 16, 8,   
  16, 16, 16, 16, 16, 2
}
};

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

void play_music() {
    randomSeed(millis());
    int music = random(0,melodyVector.size());
    
    for (int thisNote = 0; thisNote < melodyVector[music].size(); thisNote++) {
      // Calculer la durée de la note
    //   if((noteDurationsVector[thisNote] * volumeFactor) != 0) {
        int noteDuration = 1000 / (noteDurationsVector[music][thisNote]);
        M5.Speaker.tone(melodyVector[music][thisNote], noteDuration);

        // Pause entre les notes
        int pauseBetweenNotes = noteDuration * 1.3;
        delay(pauseBetweenNotes);
    //   }
      //noTone(speakerPin); // Arrêter la note
    }

    M5.Speaker.mute();
}

void callback(char *topic, byte *payload, unsigned int length) {
    if (String(topic) == mqttGlobalTopic + "/BEST") {
        String action = String(payload, length);
        Serial.println("Get action: " + action);

        tftSprite.fillScreen(TFT_BLACK);
        tftSprite.drawString(String("Received:"), 0, 10);
        tftSprite.drawString(action, 0, 30);
        tftSprite.pushSprite(0, 0);
        
        if(action.equals("IOT/M5/"+macAddress)) {
            tftSprite.setTextColor(TFT_GREEN);
            tftSprite.drawString(String("DISCO TIME"), 0, 50);
            tftSprite.pushSprite(0, 0);
            play_music();
        } else {
            tftSprite.setTextColor(TFT_RED);
            tftSprite.drawString(String("NOT THE CHOSEN ONE"), 0, 50);
            tftSprite.pushSprite(0, 0);
        }

        instructionReceived = true;
        tftSprite.setTextColor(TFT_WHITE);
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

    pinMode(speakerPin, OUTPUT); //setup du buzzer
}

void loop() {
    // bluetooth scan until the device is found
    if(!deviceFound) {
        // lancer un nouveau scan
        tftSprite.fillScreen(TFT_BLACK);
        tftSprite.drawString(String("Start bluetooth scan"), 0, 10);

        BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
        Serial.print("Devices found: ");
        Serial.println(foundDevices.getCount());
        Serial.println("Scan done!");
        tftSprite.drawString(String("Done : ") + foundDevices.getCount() + " devices", 0, 30);
        if(!macAddress.isEmpty()) tftSprite.drawString(String("M5 MAC : ") + macAddress, 0, 50);
        tftSprite.pushSprite(0, 0);
    } else {
        // the device has been found, we need to send signal to mqtt
        // deconnect the bluetooth
        Serial.println("I'm HERE !");
        BLEDevice::deinit();

        // connect to wifi
        WiFi.begin(ssid, password);
        int val = 0;
        while (WiFi.status() != WL_CONNECTED)
        {
            tftSprite.fillScreen(TFT_BLACK);
            tftSprite.drawString(String("Connection to WiFi: ") + ssid, 0, 10);
            tftSprite.drawString(String("Try num : ") + val, 0, 30);
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

        tftSprite.fillScreen(TFT_BLACK);
        tftSprite.drawString(String("Wifi connected !"), 0, 10);

        Serial.println("Wifi connected to address: " + WiFi.localIP().toString());
        macAddress = WiFi.macAddress();
        
        Serial.println("Connection to MQTT server at " + server.toString() + ":" + String(port));
        if (client.connect(WiFi.macAddress().c_str()))
        {
            String subscribeTopic = String(mqttGlobalTopic) + "/" + "BEST" + "/#";
            Serial.println("M5Stack subscribed to the topic " + subscribeTopic);
            tftSprite.drawString(String("Sub to : ") + subscribeTopic, 0, 30);
            client.subscribe(subscribeTopic.c_str());
        }

        String mqttTopic = mqttGlobalTopic + String("/M5/") + macAddress;
        // send to the mqtt client the power value
        int v = 0;
        while(!client.publish(mqttTopic.c_str(), std::to_string(rssiValue*-1).c_str())) {
            v++;
            delay(10);
            if(v > 100) break;
        }
        if(v <= 100) {

            tftSprite.drawString(String(rssiValue*-1) + "->" + mqttTopic, 0, 50);
            tftSprite.pushSprite(0, 0);

            Serial.println(String(rssiValue) + " envoyé au serveur ! " + mqttTopic);

            // wait for instruction callback
            client.setServer(server, port);
            client.setCallback(callback);
            int m = 0;
            while(!instructionReceived) {
                Serial.println("waiting for instruction " + String(m));
                tftSprite.fillRect(0,70,tftSprite.width(), 40, TFT_BLACK);
                tftSprite.drawString(String("wait for instruction : ") + m, 0, 70);
                tftSprite.pushSprite(0, 0);
                
                m+=1;
                client.loop();
                delay(500);
                if(m>=20) break;
            }
            if(m<20) Serial.println("Instruction Received !");
        } else {
            tftSprite.fillScreen(TFT_BLACK);
            tftSprite.setTextColor(TFT_RED);
            tftSprite.drawString(String("CAN'T SEND TO MQTT"), 0, 10);
            tftSprite.pushSprite(0, 0);
        }

        client.disconnect();
        WiFi.disconnect(true, false);
        delay(200);

        // reconnect the bluetooth
        BLEDevice::init("");
        tftSprite.setTextColor(TFT_WHITE);
        deviceFound = false;
        pBLEScan = BLEDevice::getScan(); // créer un nouveau scan
        pBLEScan->setActiveScan(true);   // active scan utilise plus de puissance, mais obtient des résultats plus rapidement
        pBLEScan->setInterval(100);
        pBLEScan->setWindow(99); // moins d'intervalles minimise la probabilité de collision
        pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
        instructionReceived = false;
    }
}
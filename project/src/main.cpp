#include <M5Stack.h>
#include "EnvSensor.hpp"
#include "M5_ENV.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <vector>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>


WiFiClient wifiClient; 
PubSubClient client(wifiClient); 

const char * ssid = "WIFI_TP_IOT";
const char * password = "TP_IOT_2022";
int scanTime = 30; //In seconds

uint32_t now;
IPAddress server(192, 168, 31, 134);

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
      Serial.printf("Advertised Device: %s \n", advertisedDevice.toString().c_str());
      int rssi = advertisedDevice.getRSSI();
      Serial.print("Rssi: ");
      Serial.println(rssi);
    }
}; 

void callback(char * topic, byte * payload, unsigned int length){
    DynamicJsonDocument jsonDynamic(1024); 

    Serial.print("Message recu : "); 
    Serial.println(topic); 
/*     deserializeJson(jsonDynamic, (char*)(payload));
    bool find = false;
    for (int i = 0; i < sensors.size(); i++) {
        if(sensors[i].getMac() == (jsonDynamic["mac"])) {
            sensors[i].setHum(jsonDynamic["humidity"]);
            sensors[i].setPressure(jsonDynamic["pressure"]);
            sensors[i].setTemp(jsonDynamic["temperature"]);
            find = true;
            break;
        }
    }
    if(!find) sensors.push_back(EnvSensor(jsonDynamic["mac"], jsonDynamic["temperature"], jsonDynamic["humidity"], jsonDynamic["pressure"])); */ 
    Serial.println(); 
}

void reconnect(){
    while(!client.connected()){
        if(!client.connect(WiFi.macAddress().c_str())){
            Serial.println("Erreur : tentative de connexion"); 
            delay(5000);
        } else {
            client.subscribe("Music/#"); 
        }
    }
}


void setup() {
    M5.begin();
    M5.Power.begin(); 
    WiFi.begin(ssid, password); 
    while(WiFi.status() != WL_CONNECTED){
        delay(500); 
    }
    BLEDevice::init("");
    BLEScan* pBLEScan = BLEDevice::getScan(); //create new scan
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
    BLEScanResults foundDevices = pBLEScan->start(scanTime);
    Serial.print("Devices found: ");
    Serial.println(foundDevices.getCount());
    Serial.println("Scan done!");
    client.setServer(server, 1883);
    client.setCallback(callback); 
    now = millis();
    Serial.println("Fin du Setup");
}



void loop() {
    if(millis() - now > 1000){
        Serial.println("Loop");
        now = millis();
    }
}









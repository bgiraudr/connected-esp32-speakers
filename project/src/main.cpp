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


WiFiClient wifiClient; 
PubSubClient client(wifiClient); 

#define TARGET_DEVICE "E0:6D:17:50:9F:15"

const char * ssid = "WIFI_TP_IOT";
const char * password = "TP_IOT_2022";
int scanTime = 5; //In seconds

uint32_t now;
IPAddress server(192, 168, 31, 134);

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) { 
        Serial.println("TRYING TO GET RSSI VALUE"); 
        int rssi = advertisedDevice.getRSSI(); 
        if(rssi > (- 67)){
            //We have one result but we don't know what it is 
            Serial.println("Device name : "); 
            Serial.println(advertisedDevice.getAddress().toString().c_str()); 
            Serial.print("Rssi: ");
            Serial.println(rssi); 
        }
    }
}; 

/*    Serial.printf("Advertised Device: %s \n", advertisedDevice.toString().c_str());
        if(advertisedDevice.getAddress().equals(BLEAddress(TARGET_DEVICE))){
            Serial.print("RSSI Value : "); 
            //Serial.println(advertisedDevice.getRSSI()); 
        }
*/

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

/* void reconnect(){
    while(!client.connected()){
        if(!client.connect(WiFi.macAddress().c_str())){
            Serial.println("Erreur : tentative de connexion"); 
            delay(5000);
        } else {
            client.subscribe("Music/#"); 
        }
    }
} */


void setup() {
    M5.begin();
    M5.Power.begin(); 
/*    WiFi.begin(ssid, password); 
     while(WiFi.status() != WL_CONNECTED){
        delay(500); 
    } */
    BLEDevice::init("");
    BLEScan* pBLEScan = BLEDevice::getScan(); //create new scan
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(false); //active scan uses more power, but get results faster
    BLEScanResults foundDevices = pBLEScan->start(scanTime);
/*     Serial.print("Devices found: ");
    Serial.println(foundDevices.getCount());
    Serial.println("Scan done!"); 
    client.setServer(server, 1883);
    client.setCallback(callback); */
    now = millis();
    Serial.println("Fin du Setup");
}



void loop() {
    if(millis() - now > 1000){
        now = millis();
    }
}









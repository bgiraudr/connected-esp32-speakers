#include <BLEDevice.h>
#include <M5Stack.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <FastLED.h>

// UUID du service BLE
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define NUM_LEDS 25
#define DATA_PIN 27

CRGB leds[NUM_LEDS];

void setup() {
  Serial.begin(115200);

  // Créer le dispositif BLE
  BLEDevice::init("M5Stack");

  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);

  // Créer le serveur BLE
  BLEServer *pServer = BLEDevice::createServer();

  // Créer le service BLE
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Démarrer le service
  pService->start();

  // Démarrer l'annonce
  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->start();

  Serial.println("BLE device is now advertising");
  
  // Afficher l'adresse Bluetooth du M5Stack
  Serial.print("Bluetooth Address: ");
  Serial.println(BLEDevice::getAddress().toString().c_str());

  for(int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Blue;
  }
  FastLED.show();
}

void loop() {
  // Mettre votre code principal ici, pour s'exécuter de manière répétée
}

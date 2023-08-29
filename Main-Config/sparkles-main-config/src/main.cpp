#include <Arduino.h>
#include "ESP8266WiFi.h"
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <WiFiManager.h>
#include <espnow.h>
#include "ESP8266TimerInterrupt.h"
#define TIMER_INTERVAL_MS       1000
#define USING_TIM_DIV1 true
#define SEND_ADDRESS 1
#define CALIBRATE 2
#define RETRIEVE_TIMES 3
volatile uint32_t lastMillis = 0;
ESP8266Timer ITimer;
uint32_t timerCounter = 0;
int mode = 0;
uint8_t clientAddresses[300][6];
int clientAddressCounter = 0;
int clientAddressTimerCounter = 0;
int retrieverCounter = 0;

uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
typedef struct struct_message {
  uint8_t a[6];
  int b;
  float c;
  String d;
  bool e;
} struct_message;

void IRAM_ATTR TimerHandler()
{
  myData.b = timerCounter;
  if (mode == SEND_ADDRESS) {
    if (clientAddressTimerCounter !=0 && timerCounter > clientAddressTimerCounter+ 2) {
      mode = CALIBRATE;
    }
    myData.d = "SEND_ADDRESS";
    esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
  }
  else if (mode == CALIBRATE ) {
    myData.d = "TIMER";
    esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
  }
  else if (mode == RETRIEVE_TIMES) {
    //figure out a good algorithm to retrieve the runtimes from the individual clients
    //keep in mind that each client needs to be polled individually and we need a different kind of banddwith than "per second" for this
    //esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));

  }

  timerCounter++;


}


void OnDataRecv(uint8_t * mac, uint8_t  *incomingData, uint8_t len) {
  if (len == sizeof(myData)) {
    memcpy(&myData, incomingData, len);
  }
  if (mode == SEND_ADDRESS) {
    memcpy(&clientAddresses[clientAddressCounter], myData.a, 6);
    clientAddressTimerCounter = timerCounter;
    clientAddressCounter++;
  }

}

void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.print("Last Packet Send Status: ");
  if (sendStatus == 0){
    Serial.println("Delivery success");
  }
  else{
    Serial.println("Delivery fail");
  }
}

struct_message myData;



void setup() {
  Serial.begin(115200);
  if (ITimer.attachInterruptInterval(TIMER_INTERVAL_MS * 1000, TimerHandler))
  {
    lastMillis = millis();
    Serial.print(F("Starting  ITimer OK, millis() = ")); 
    Serial.println(lastMillis);
  }
  else 
    Serial.println(F("Can't set ITimer correctly. Select another freq. or interval")); 
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  WiFi.macAddress(myData.a); 
  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_register_send_cb(OnDataSent);
  esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
  mode = SEND_ADDRESS;
}

void loop() {
// analog read on analog read

}


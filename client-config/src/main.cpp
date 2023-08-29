#include <Arduino.h>
#include "ESP8266WiFi.h"
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <WiFiManager.h>
#include <espnow.h>
#define MODE_CONFIG 1
#define MODE_RTRV_VALUES 2
#define MODE_RUN 3
int mode = 0;
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
typedef struct struct_message {
    uint8_t a[6];
    int b;
    float c;
    String d;
    bool e;
} struct_message;

struct_message myData;
int timerCounter = 0;
uint8_t mainAddress[6];
float times[100];

void sendTimes(int index) {
  struct_message returnData;
  returnData.d = "RETURN";
  returnData.b = index;
  returnData.c = times[index];
  esp_now_send(mainAddress, (uint8_t *) &returnData, sizeof(returnData));
}

void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  memcpy(&myData, incomingData, sizeof(myData));
  if (myData.d == "Timer") {
    if (mode != MODE_CONFIG) {
      mode = MODE_CONFIG;
    }
    if (timerCounter != myData.b){
    timerCounter = myData.b;
    }
    memcpy (&mainAddress, myData.a, sizeof(myData.a));
  }
  if (myData.d == "END_TIMER") {
    mode = 0;
  }
  if (myData.d == "SEND_TIMES") {
    if (mode != MODE_RTRV_VALUES) {
      mode = MODE_RTRV_VALUES;
    }
    sendTimes(myData.b);
  }
  if (myData.d == "MAIN_ADDRESS") { 

  }

}

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb(OnDataRecv);  
}

void loop() {
  if (mode == MODE_CONFIG) {
    //analogRead
  }
}
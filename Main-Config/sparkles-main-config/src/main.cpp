#include <Arduino.h>
#include "ESP8266WiFi.h"
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <WiFiManager.h>
#include <espnow.h>
#include "ESP8266TimerInterrupt.h"
#define TIMER_INTERVAL_MS       1000
#define USING_TIM_DIV1 true
volatile uint32_t lastMillis = 0;
ESP8266Timer ITimer;
int timerCounter = 0;
int mode = 0;

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
  myData.d = "Timer";
  myData.b = timerCounter;
  timerCounter++;
  esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));

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
}

void loop() {
// analog read on analog read

}


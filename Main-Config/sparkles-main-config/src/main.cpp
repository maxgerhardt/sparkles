#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include "ESP32TimerInterrupt.h"
#define TIMER_INTERVAL_MS       1000
#define USING_TIM_DIV1 true
#define TIMER_CALIBRATION 1
int mode = TIMER_CALIBRATION;


/*
TODO: 
- safeguard that a reboot of master will somehow cause the clients to restart their timer
- 
*/

//----------
//wifi stuff
//----------
//uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
//MAIN ESP32 
//uint8_t broadcastAddress[] = {0xB4,0xE6,0x2D,0xE9,0x3C,0x21};
//xiao address
//uint8_t broadcastAddress[] = {0x34,0x85,0x18,0x5,0x88,0xb0};
//CLIENT ESP32
uint8_t broadcastAddress[] = {0x7c,0x87,0xce,0x2d,0xcf,0x98};

esp_now_peer_info_t peerInfo;
//-------
//message types
//--------
struct timer_message {
  uint8_t messageType;
  uint16_t counter;
  uint16_t sendTime;
  uint16_t lastDelay;
} ;
typedef struct mode_change {
  uint8_t messageType;
  uint8_t mode;
} mode_change;

struct set_address {
  uint8_t messageType;
  uint8_t address[6];
};


timer_message timerMessage;
mode_change modeChange;
set_address setAddress;

//-----------
//Timer config variables
//-----------
ESP32Timer ITimer(0);
int msgSendTime;
int msgArriveTime;
int msgReceiveTime;
bool newMsgSent = false;
bool newMsgReceived = false;
int timesince;
int newMsgTime;
int lastDelay = 0;
uint32_t timerCounter = 0;



bool IRAM_ATTR TimerHandler(void * timerNo)
{ 
    timerCounter++;
    if (mode == TIMER_CALIBRATION and timerCounter < 100) {
      msgSendTime = micros();
      timerMessage.messageType = 0;
      timerMessage.sendTime = msgSendTime;
      timerMessage.counter = timerCounter;
      timerMessage.lastDelay = lastDelay;
      esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &timerMessage, sizeof(timerMessage));
      newMsgSent = true;
    }

  return true;
}


void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int lenn) {
  msgReceiveTime = micros();
  newMsgReceived = true;

}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t sendStatus) {
  if (sendStatus == ESP_NOW_SEND_SUCCESS) {
    msgArriveTime = micros();
    lastDelay = msgArriveTime-msgSendTime;
  }
  else {
    msgArriveTime = 0;
  }
}


void setup() {
  Serial.begin(115200);
  
  if (ITimer.attachInterruptInterval(TIMER_INTERVAL_MS * 1000, TimerHandler))
  {
    Serial.print(F("Starting  ITimer OK, millis() = ")); 
    Serial.println(millis());
  }
  else 
    Serial.println(F("Can't set ITimer correctly. Select another freq. or interval")); 
  
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  //esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
    // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);  
  //esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
  

}

void loop() {
  if (newMsgSent == true) { 
  Serial.print("Message Sent. Timer: ");
  Serial.print(timerMessage.counter);
  Serial.print(" SendTime: ");
  Serial.print(timerMessage.sendTime);
  Serial.print(" LastDelay: ");
  Serial.println(timerMessage.lastDelay);
  newMsgSent = false;
  }

// analog read on analog read

}


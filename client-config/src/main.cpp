#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include "ESP32TimerInterrupt.h"
#define TIMER_INTERVAL_MS       1000
#define USING_TIM_DIV1 true

#define SET_TIME_OFFSET 1
#define PRINT_MODE -1
#define WAIT_FOR_INSTRUCTIONS 0
#define CALIBRATE 2

//Network
//uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t broadcastAddress[] = {0xB4,0xE6,0x2D,0xE9,0x3C,0x21};
esp_now_peer_info_t peerInfo;


//Variables for Time Offset
struct timer_message {
  uint8_t messageType;
  uint16_t counter;
  uint16_t sendTime;
  uint16_t lastDelay;
} ;

//3 ints 1 float 1 address 1 string 1 bool?

struct mode_change {
  uint8_t messageType;
  uint8_t mode;
} ;


int messageArriveTime;
int lastTime = 0;

//message_struct testmsg;
int timeOffset;

//general setup
int mode = SET_TIME_OFFSET;

timer_message timerMessage; 
mode_change modeChange;
timer_message oldMessage;

//calibration stuff
int sensorValue;
int microphonePin = A0;
int clapCounter = 0;
int lastClap;
//make sure 
struct clap {
  int timerCounter;
  int timeStamp;
};
clap claps[100];

//timer stuff
ESP32Timer ITimer(0);
uint32_t timerCounter = 0;


bool IRAM_ATTR TimerHandler(void * timerNo)
{ 
    timerCounter++;
  return true;
}


void receiveTimer(int messageArriveTime) {
  if (abs(messageArriveTime - lastTime-1000000) < 300 and abs(timerMessage.lastDelay) <1500) {
    timeOffset = lastTime-oldMessage.sendTime;
    mode = PRINT_MODE;
  }
  else {
    lastTime = messageArriveTime;
    oldMessage = timerMessage;
  }
}


void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  switch (incomingData[0]) {
    case SET_TIME_OFFSET: 
      messageArriveTime = micros();
      memcpy(&timerMessage,incomingData,sizeof(timerMessage));
      receiveTimer(messageArriveTime);
      break;
    case CALIBRATE: 
      mode = incomingData[1];
      break;
      Serial.println("Data type not recognized");
  }
  
  if (mode == SET_TIME_OFFSET) {

  }
  if (mode == 0 ){

  }
  
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t sendStatus) {
  messageArriveTime = ESP_NOW_SEND_SUCCESS ? micros() : 0;
}



void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  // start timer
  if (ITimer.attachInterruptInterval(TIMER_INTERVAL_MS * 1000, TimerHandler))
  {
    Serial.print(F("Starting  ITimer OK, millis() = ")); 
    Serial.println(millis());
  }
  else 
    Serial.println(F("Can't set ITimer correctly. Select another freq. or interval")); 
  
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);



  // Init ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
    // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);  
  esp_now_register_send_cb(OnDataSent);

}

void loop() {
  if (mode == -1 ){
    mode = 0;
    Serial.print("Time: ");
    Serial.println(timerMessage.sendTime);
    Serial.print("Counter: ");
    Serial.println(timerMessage.counter);
    Serial.print("Difference ");
    Serial.println(timeOffset);
  }
  if (mode == CALIBRATE) {
    sensorValue = analogRead(microphonePin);
    if (sensorValue < 50 and millis() > lastClap+1000) {
      claps[clapCounter].timerCounter = timerCounter;
      claps[clapCounter].timeStamp = micros();
      lastClap = millis();
      clapCounter++;
  }

}
}
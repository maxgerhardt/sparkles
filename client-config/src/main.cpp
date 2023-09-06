#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include "ESP32TimerInterrupt.h"
#define TIMER_INTERVAL_MS       1000
#define USING_TIM_DIV1 true

#define WAIT_FOR_TIMER 1
#define TIMER_CALIBRATION 2
#define PRINT_MODE -1
#define HELLO 0
#define WAIT_FOR_CALIBRATE 3
#define CALIBRATE 4
#define SEND_CLAP_TIME 5
#define CLAP 6

//Network
//uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t broadcastAddress[] = {0xB4,0xE6,0x2D,0xE9,0x3C,0x21};
uint8_t myAddress[6];
esp_now_peer_info_t peerInfo;


//Variables for Time Offset
struct timer_message {
  uint8_t messageType;
  uint16_t counter;
  uint16_t sendTime;
  uint16_t lastDelay;
} timerMessage, oldMessage;

//3 ints 1 float 1 address 1 string 1 bool?

struct mode_change {
  uint8_t messageType;
  uint8_t mode;
} modeChange;

int messageArriveTime;
int lastTime = 0;

//message_struct testmsg;
int timeOffset;

//general setup
int mode = HELLO;


//calibration stuff
int sensorValue;
int microphonePin = A0;
int clapCounter = 0;
int lastClap;
//make sure 
struct clap_time {
  uint8_t messageType = CLAP;
  int clapCounter;
  int timerCounter;
  int timeStamp;
} claps[100];

bool clapSent = false;

struct send_clap {
  uint8_t messageType = SEND_CLAP_TIME;
  uint8_t address[6];
  uint8_t clapIndex;
} sendClap ;

//address sending
int addressReceived = false;
int addressSending = 0;
struct address_message {
  uint8_t messageType = HELLO;
  uint8_t address[6];
} addressMessage;

//timer stuff
ESP32Timer ITimer(0);
uint32_t timerCounter = 0;
struct timer_received_message {
  uint8_t messageType = WAIT_FOR_TIMER;
  uint8_t address[6];
  uint8_t timerOffset;
} timerReceivedMessage;

bool IRAM_ATTR TimerHandler(void * timerNo)
{ 
  timerCounter++;
  return true;
}


void sendAddress() {
  esp_now_send(broadcastAddress, (uint8_t *) &addressMessage, sizeof(addressMessage));
}

void sendClapTime(int clapIndex) {
  esp_now_send(broadcastAddress, (uint8_t *) &claps[clapIndex], sizeof(claps[clapIndex]));
}

void receiveTimer(int messageArriveTime) {
  if (abs(messageArriveTime - lastTime-1000000) < 300 and abs(timerMessage.lastDelay) <1500) {
    timeOffset = lastTime-oldMessage.sendTime;
    WiFi.macAddress(timerReceivedMessage.address);
    timerReceivedMessage.timerOffset = timeOffset;
    esp_now_send(broadcastAddress,(uint8_t *) &timerReceivedMessage, sizeof(timerReceivedMessage) );
  }
  else {
    lastTime = messageArriveTime;
    oldMessage = timerMessage;
  }
}


void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  switch (incomingData[0]) {
    case TIMER_CALIBRATION:  
      messageArriveTime = micros();
      memcpy(&timerMessage,incomingData,sizeof(timerMessage));
      receiveTimer(messageArriveTime);
      break;
    case CALIBRATE: 
      mode = incomingData[1];
      break;
    case SEND_CLAP_TIME: 
      mode = SEND_CLAP_TIME;
      memcpy(&sendClap,incomingData,sizeof(sendClap));
      sendClapTime(sendClap.clapIndex);
      break;

    default: 
      Serial.println("Data type not recognized");
  }
  
  if (mode == 0 ){

  }
  
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t sendStatus) {
  if (mode == HELLO) {
    if (ESP_NOW_SEND_SUCCESS == true) {
      mode = WAIT_FOR_TIMER;
    }
  }
  else if (mode == TIMER_CALIBRATION) {
    if (ESP_NOW_SEND_SUCCESS == true) {
      mode = WAIT_FOR_CALIBRATE;
    }
  else if (mode == SEND_CLAP_TIME) {
    if (ESP_NOW_SEND_SUCCESS == false) {
      clapSent = false;
    }
    else {
      clapSent = true;
    }
  }
    else {
      esp_now_send(broadcastAddress,(uint8_t *) &timerReceivedMessage, sizeof(timerReceivedMessage) );
    }
  }

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
  WiFi.macAddress(addressMessage.address);


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
  if (mode == HELLO) { 
    sendAddress();
    delay(50);
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
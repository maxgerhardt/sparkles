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
#define ASK_CLAP_TIME 5
#define SEND_CLAP_TIME 6 
#define NOCLAPFOUND -1
int mode;


//Network
//uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t broadcastAddress[] = {0xB4,0xE6,0x2D,0xE9,0x3C,0x21};
uint8_t myAddress[6];
esp_now_peer_info_t peerInfo;


//Variables for Time Offset
struct message_timer {
  uint8_t messageType;
  uint16_t counter;
  uint16_t sendTime;
  uint16_t lastDelay;
} timerMessage, oldMessage;

struct message_mode {
  uint8_t messageType;
  uint8_t mode;
} modeChange;

struct message_timer_received {
  uint8_t messageType = WAIT_FOR_CALIBRATE;
  uint8_t address[6];
  uint8_t timerOffset;
} timerReceivedMessage;

struct message_clap_time {
  uint8_t messageType = SEND_CLAP_TIME;
  int clapCounter;
  int timerCounter;
  int timeStamp;
} claps[100];

struct message_send_clap {
  uint8_t messageType = ASK_CLAP_TIME;
  uint8_t address[6];
  uint8_t clapIndex;
} sendClap ;

struct message_address {
  uint8_t messageType = HELLO;
  uint8_t address[6];
} addressMessage;

struct no_clap_found {
  uint8_t messageType = NOCLAPFOUND;
  uint8_t address[6];
  uint8_t clapIndex;
} noClapFound;




//timer stuff
int timeOffset;
int messageArriveTime;
int lastTime = 0;


//general setup



//calibration stuff
int sensorValue;
int microphonePin = A0;
int clapCounter = 0;
int lastClap;
bool clapSent = false;

//address sending
int addressReceived = false;
int addressSending = 0;


//timer stuff
ESP32Timer ITimer(0);
uint32_t timerCounter = 0;


bool IRAM_ATTR TimerHandler(void * timerNo)
{ 
  timerCounter++;
  return true;
}


void sendAddress() {
  esp_now_send(broadcastAddress, (uint8_t *) &addressMessage, sizeof(addressMessage));
}

void sendClapTime(int clapIndex) {
  for (int i = 0; i <= clapCounter; i++) {
    if (claps[i].timerCounter < clapCounter+2 or claps[i].timerCounter > clapCounter-2) { 
      esp_now_send(broadcastAddress, (uint8_t *) &claps[i], sizeof(claps[i]));
      break;
    }
  }
  esp_now_send(broadcastAddress, (uint8_t *) &noClapFound, sizeof(noClapFound));

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
    case ASK_CLAP_TIME: 
      mode = ASK_CLAP_TIME;
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
  else if (mode == ASK_CLAP_TIME) {
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
  mode = HELLO;
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
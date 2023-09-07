#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include "ESP32TimerInterrupt.h"
#define TIMER_INTERVAL_MS       1000
#define USING_TIM_DIV1 true

#define HELLO 0
#define WAIT_FOR_TIMER 1
#define TIMER_CALIBRATION 2
#define WAIT_FOR_CALIBRATE 3
#define CALIBRATE 4
#define ASK_CLAP_TIME 5
#define SEND_CLAP_TIME 6
#define ANIMATE 7
int mode = HELLO;


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
uint8_t emptyAddress[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
esp_now_peer_info_t peerInfo;

//-------
//message types
//--------
struct message_timer {
  uint8_t messageType;
  uint16_t counter;
  uint16_t sendTime;
  uint16_t lastDelay;
} timerMessage;
typedef struct message_mode_change {
  uint8_t messageType;
  uint8_t mode;
} modeChange;

struct message_timer_received {
  uint8_t messageType = WAIT_FOR_TIMER;
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
struct message_address{
  uint8_t messageType = HELLO;
  uint8_t address[6];
} addressMessage;



struct client_address {
  uint8_t address[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  int id;
  float xLoc;
  float yLoc;
  float zLoc;
} clientAddresses[200];


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

//calibration
int sensorValue;
int microphonePin = A0;
int clapCounter = 0;
int lastClap = 0;


//receive addresses
int addressCounter = 0;

bool IRAM_ATTR TimerHandler(void * timerNo)
{ 
    timerCounter++;
    //wait for timer vs wait for calibrate
    if (mode == TIMER_CALIBRATION and timerCounter < 100) {
      msgSendTime = micros();
      timerMessage.messageType = TIMER_CALIBRATION;
      timerMessage.sendTime = msgSendTime;
      timerMessage.counter = timerCounter;
      timerMessage.lastDelay = lastDelay;
      esp_err_t result = esp_now_send(clientAddresses[addressCounter-1].address, (uint8_t *) &timerMessage, sizeof(timerMessage));
      newMsgSent = true;
    }

  return true;
}


void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int lenn) {
  if (mode == HELLO) {
    if (incomingData[0] == HELLO) { 
      memcpy(&addressMessage,incomingData,sizeof(addressMessage));
      for (int i = 0; i < sizeof(clientAddresses); i++) {
        if (clientAddresses[i].address == emptyAddress) {
          memcpy(&clientAddresses[i].address, addressMessage.address, sizeof(addressMessage.address));
          addressCounter++;
          break;
        }
        else if (clientAddresses[i].address == addressMessage.address) {
          break;
        }
      }
    }
    else if (incomingData[0] == WAIT_FOR_TIMER) {
      //check if incomingdata[1] can be in accordance with current Timer
      mode = TIMER_CALIBRATION;
    }
    else if (incomingData[0] == WAIT_FOR_CALIBRATE) {
      mode = HELLO;
    }
    //how to actually store the clap data?
    

  }

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
  if (mode == CALIBRATE) {
    sensorValue = analogRead(microphonePin);
    if (sensorValue < 50 and millis() > lastClap+1000) {
      claps[clapCounter].timerCounter = timerCounter;
      claps[clapCounter].timeStamp = micros();
      lastClap = millis();
      clapCounter++;
    }
  }
  else if (mode == ASK_CLAP_TIME) {
    //make sure claps actually correspond, in case one got lost or so?
    for (int i =0; i <= addressCounter; i++) {
      if (clientAddresses[i].address == emptyAddress) {
        mode = ANIMATE;
        break;
      }
      else { 
        for (int j = 0; j < clapCounter; j++) {;
          sendClap.clapIndex = j;
          memcpy(&sendClap.address, clientAddresses[i].address, sizeof(clientAddresses[i].address));
          esp_now_send(clientAddresses[i].address, (uint8_t *) &sendClap, sizeof(sendClap) );
        }
      }
    }
  }

}


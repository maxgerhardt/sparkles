#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
//#include "ESP32TimerInterrupt.h"
//#include "driver/gptimer.h"
#define TIMER_INTERVAL_MS       1000
#define USING_TIM_DIV1 true

#define HELLO 0
#define ANNOUNCE 1
#define WAIT_FOR_TIMER 1
#define TIMER_CALIBRATION 2
#define GOT_TIMER 3
#define CALIBRATE 4
#define ASK_CLAP_TIME 5
#define SEND_CLAP_TIME 6
#define ANIMATE 7
#define NOCLAPFOUND -1
int mode = HELLO;
#define NUM_DEVICES 20;
hw_timer_t * timer = NULL;
int interruptCounter;  //for counting interrupt
int totalInterruptCounter;   	//total interrupt counting

//portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
 

//----------
//wifi stuff
//----------
//uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
//MAIN ESP32 
//uint8_t broadcastAddress[] = {0xB4,0xE6,0x2D,0xE9,0x3C,0x21};
//xiao address
//uint8_t broadcastAddress[] = {0x34,0x85,0x18,0x5,0x88,0xb0};
//CLIENT ESP32
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t emptyAddress[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
esp_now_peer_info_t peerInfo;
uint8_t timerReceiver[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

//-------
//message types
//--------
struct message_timer {
  uint8_t messageType;
  uint16_t counter;
  uint16_t sendTime;
  uint16_t lastDelay;
} timerMessage;

struct message_announce {
  uint8_t messageType = ANNOUNCE;
  uint16_t sendTime;
  uint8_t address[6];
} announceMessage;

struct message_timer_received {
  uint8_t messageType = WAIT_FOR_TIMER;
  uint8_t address[6];
  uint8_t timerOffset;
} timerReceivedMessage;

struct message_address{
  uint8_t messageType = HELLO;
  uint8_t address[6];
} addressMessage;


//-----------
//Timer config variables
//-----------
int msgSendTime;
int msgArriveTime;
int msgReceiveTime;
bool newMsgSent = false;
bool newMsgReceived = false;
int timesince;
int newMsgTime;
int lastDelay = 0;
int timerCounter = 0;

//calibration
int sensorValue;
int microphonePin = A0;
int clapCounter = 0;
int lastClap = 0;


//receive addresses
int addressCounter = 0;



struct client_address {
  uint8_t address[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  int id;
  float xLoc;
  float yLoc;
  float zLoc;
} clientAddresses[20];

void printAddress(const uint8_t * mac_addr){
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.println(macStr);
}


void IRAM_ATTR onTimer()
{   
    msgSendTime = micros();
    Serial.print("Counter in Timer");
    Serial.print(timerCounter);
    timerCounter++;
    if (timerCounter > 10000) {
      Serial.println("resetting to 0");
      timerCounter = 0;
    }
    //wait for timer vs wait for calibrate
    if (mode == TIMER_CALIBRATION and timerCounter < 100) {
      timerMessage.messageType = TIMER_CALIBRATION;
      timerMessage.sendTime = msgSendTime;
      timerMessage.counter = timerCounter;
      timerMessage.lastDelay = lastDelay;
      Serial.println("SENDING TIMER TO CLIENT");

      esp_err_t result = esp_now_send(timerReceiver, (uint8_t *) &timerMessage, sizeof(timerMessage));
      newMsgSent = true;
    }
    else {
      announceMessage.sendTime = msgSendTime;
      esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &announceMessage, sizeof(announceMessage));
    }

  //return true;
}


 
void addPeer(uint8_t * address) {
  Serial.print("Adding peer");
  printAddress(address);
  memcpy(&peerInfo.peer_addr, address, 6);
  if (esp_now_get_peer(peerInfo.peer_addr, &peerInfo) == ESP_OK) {
    Serial.println("Found Peer");
    return;
  }
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
    // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
  else {
    Serial.println("Added Peer");
  }
}

void removePeer(uint8_t address[6]) {
  if (esp_now_del_peer(address) != ESP_OK) {
    Serial.println("coudln't delete peer");
    return;
  }
}

void  OnDataRecv(const esp_now_recv_info * mac, const uint8_t *incomingData, int len) {
  if (mode == HELLO) {
    if (incomingData[0] == HELLO) { 
      Serial.println("received hello");
      memcpy(&addressMessage,incomingData,sizeof(addressMessage));
      for (int i = 0; i < sizeof(clientAddresses); i++) {
        if (memcmp(&clientAddresses[i].address, &emptyAddress, 6) == true) {
          Serial.println("Adding Address");
          memcpy(&clientAddresses[i].address, addressMessage.address, sizeof(addressMessage.address));
          memcpy(timerReceiver, addressMessage.address, sizeof(addressMessage.address));
          addPeer(timerReceiver);
          addressCounter++;
          break;
        }
        else if (clientAddresses[i].address == addressMessage.address) {
          break;
        }
        mode = TIMER_CALIBRATION;

      }
    }
    else if (incomingData[0] == WAIT_FOR_TIMER) {
      //check if incomingdata[1] can be in accordance with current Timer
      Serial.println("WAIT FOR TIMER");
      mode = TIMER_CALIBRATION;
      memcpy(&timerReceiver, addressMessage.address, sizeof(addressMessage.address));
      addPeer(timerReceiver);
    }
    else if (incomingData[0] == GOT_TIMER) {
      removePeer(timerReceiver);
      mode = HELLO;
    }
    }
    //how to actually store the clap data?


  }




void  OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t sendStatus) {
  //das geht natürlich nur wenn der richtige message status 
  Serial.println("-------");
  Serial.println("message sent");
  printAddress(mac_addr);
  
  if (sendStatus == ESP_NOW_SEND_SUCCESS) {
    Serial.println("Success");
    msgArriveTime = micros();
    lastDelay = msgArriveTime-msgSendTime;
  }
  else {
    msgArriveTime = 0;
  }
  Serial.println("-------");
}


void setup() {
  Serial.begin(115200);

  timer = timerBegin(1000000);           	// timer 0, prescalar: 80, UP counting
  timerAttachInterrupt(timer, &onTimer); 	// Attach interrupt
  timerWrite(timer, 0);  		// Match value= 1000000 for 1 sec. delay.
  timerStart(timer);   
  timerAlarm(timer, 5000000, true, 0);

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
  WiFi.macAddress(announceMessage.address);
  timerCounter = 0;
}

void loop() {
//esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &announceMessage, sizeof(announceMessage));
Serial.println(".");
printAddress(timerReceiver);
Serial.print("Mode: ");
Serial.println(mode);
Serial.print("TimerCounter ");
Serial.println(timerCounter);
delay(10000);

 /* else if (mode == ASK_CLAP_TIME) {
    //make sure claps actually correspond, in case one got lost or so?
    for (int i =0; i <= addressCounter; i++) {
      if (clientAddresses[i].address == emptyAddress) {
        mode = ANIMATE;
        break;
      }
      else { 
        for (int j = 0; j < clapCounter; j++) {;
          sendClap.clapIndex = claps[j].timerCounter;
          memcpy(&sendClap.address, clientAddresses[i].address, sizeof(clientAddresses[i].address));
          esp_now_send(clientAddresses[i].address, (uint8_t *) &sendClap, sizeof(sendClap) );
        }
      }
    }
  }*/

}


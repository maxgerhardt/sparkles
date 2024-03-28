#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
//#include "ESP32TimerInterrupt.h"
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

int mode;
boolean from_head = true;
int foo = 0;

int timerDifference = 0;

//Network
//uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
uint8_t hostAddress[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint8_t hostdAddress[] = {0x7c, 0x87, 0xce, 0x2d, 0xcf, 0x98 };
uint8_t myAddress[6];
esp_now_peer_info_t peerInfo;

esp_now_peer_num_t peerNum;
int timerflag = 0;


//Variables for Time Offset
struct message_timer {
  uint8_t messageType;
  uint16_t counter;
  uint32_t sendTime;
  uint16_t lastDelay;
} timerMessage, oldMessage;

struct message_announce {
  uint8_t messageType = ANNOUNCE;
  uint32_t sendTime;
  uint8_t address[6];
} announceMessage;

struct message_mode {
  uint8_t messageType;
  uint8_t mode;
} modeChange;

struct message_timer_received {
  uint8_t messageType = GOT_TIMER;
  uint8_t address[6];
  uint32_t timerOffset;
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
uint32_t messageArriveTime;
uint32_t lastTime = 0;


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
//ESP32Timer ITimer(0);
uint32_t timerCounter = 0;

void printAddress(const uint8_t * mac_addr){
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print(macStr);
}

void addPeer(uint8_t * address) {
  memcpy(&peerInfo.peer_addr, address, 6);
  if (esp_now_get_peer(peerInfo.peer_addr, &peerInfo) == ESP_OK) {
    return;
  }
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
    // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
}

void sendAddress() {
  esp_now_send(broadcastAddress, (uint8_t *) &addressMessage, sizeof(addressMessage));
}


void receiveTimer(int messageArriveTime) {
  timerflag++;
  //wenn die letzte message maximal 300 mikrosekunden abweicht und der letzte delay auch nicht mehr als 1500ms her war, dann muss die msg korrekt sein
  if (messageArriveTime < lastTime) {
    timerDifference = (4294967295-lastTime)+messageArriveTime;
  }
  else {
    timerDifference = messageArriveTime-lastTime;
  }
  if (abs(timerDifference-1000000) < 300 and abs(timerMessage.lastDelay) <1500) {
    timerflag = timerflag+10;
    //damit hab ich den zeitoffset.. 
    // if zeit - timeoffset % 1000 = 0: blink
    timeOffset = lastTime-oldMessage.sendTime;
    WiFi.macAddress(timerReceivedMessage.address);
    timerReceivedMessage.timerOffset = timeOffset;
    esp_now_send(hostAddress,(uint8_t *) &timerReceivedMessage, sizeof(timerReceivedMessage) );
  }
  else {
    lastTime = messageArriveTime;
    oldMessage = timerMessage;
  }
}






void OnDataRecv(const esp_now_recv_info * mac, const uint8_t *incomingData, int len) {
  memcpy(&hostAddress, mac->src_addr, sizeof(hostAddress));
  switch (incomingData[0]) {
    case ANNOUNCE: 
    {
      addPeer(hostAddress);
      memcpy(&announceMessage, incomingData, sizeof(announceMessage));
      esp_now_send(hostAddress, (uint8_t*) &addressMessage, sizeof(addressMessage));
      mode=TIMER_CALIBRATION;
      
    }
      break;
    case TIMER_CALIBRATION:  
    {
      messageArriveTime = micros();
      memcpy(&timerMessage,incomingData,sizeof(timerMessage));
      receiveTimer(messageArriveTime);
    }
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
      mode = CALIBRATE;
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
      //esp_now_send(broadcastAddress,(uint8_t *) &timerReceivedMessage, sizeof(timerReceivedMessage) );
    }
  }

}



void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
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
   WiFi.macAddress(addressMessage.address);
   esp_now_get_peer_num(&peerNum);

     Serial.print("Number of Peers start: ");
  Serial.println(peerNum.total_num);
  delay(2000);
  
}

void loop() {
  //printAddress(addressMessage.address);
  Serial.println("");
  Serial.print("Last Delay: ");
  Serial.println(timerMessage.lastDelay);
  Serial.print ("Timer Flag");
  Serial.println(timerflag);

  delay(5000);

} 
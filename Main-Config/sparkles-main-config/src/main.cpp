#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <PeakDetection.h> 

//#include "ESP32TimerInterrupt.h"
//#include "driver/gptimer.h"
#define TIMER_INTERVAL_MS       1000
#define USING_TIM_DIV1 true

#define MSG_HELLO 0
#define MSG_ANNOUNCE 1
#define MSG_TIMER_CALIBRATION 2
#define MSG_GOT_TIMER 3
#define MSG_ASK_CLAP_TIME 5
#define MSG_SEND_CLAP_TIME 6
#define MSG_ANIMATION 7
#define MSG_NOCLAPFOUND -1

#define MODE_SEND_ANNOUNCE 0
#define MODE_SENDING_TIMER 1
#define MODE_CALIBRATE 4
#define MODE_ANIMATE 7

#define ANIMATION_SYNC 1
int mode;
#define NUM_DEVICES 20

#define V1 1
#define V2 2
#define D1 3

#define DEVICE V2
hw_timer_t * timer = NULL;
PeakDetection peakDetection; 

int interruptCounter;  //for counting interrupt
int totalInterruptCounter;   	//total interrupt counting
bool start = true;
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t emptyAddress[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t timerReceiver[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
esp_now_peer_info_t peerInfo;

int audioPin = 5;
int oldClapCounter = 0;

void printMode(int mode) { 
  Serial.print("Mode: ");
  switch (mode) {
    case MODE_SEND_ANNOUNCE:
    Serial.println("MODE_SEND_ANNOUNCE");
    break;
    case MODE_SENDING_TIMER:
    Serial.println("MODE_SENDING_TIMER");
    break;
    case MODE_CALIBRATE: 
    Serial.println("MODE_CALIBRATE");
    break;
    case MODE_ANIMATE:
    Serial.println("MODE_ANIMATE");
    break;
  }
}


void printMessage(int message) { 
  Serial.print("Message: ");
  switch (message) {
    case MSG_HELLO:
    Serial.println("MSG_HELLO");
    break;
    case MSG_ANNOUNCE:
    Serial.println("MSG_ANNOUNCE");
    break;
    case MSG_GOT_TIMER : 
    Serial.println("MSG_GOT_TIMER ");
    break;
    case MSG_SEND_CLAP_TIME:
    Serial.println("MSG_SEND_CLAP_TIME");
    default: 
    Serial.println("Didn't recognize Message");
  }
}

//-------
//message types
//--------
struct message_timer {
  uint8_t messageType;
  uint16_t counter;
  uint32_t sendTime;
  uint16_t lastDelay;
} timerMessage;


struct message_got_timer {
  uint8_t messageType = MSG_GOT_TIMER;
  uint16_t delayAvg;
} gotTimerMessage;
struct message_announce {
  uint8_t messageType = MSG_ANNOUNCE;
  uint32_t sendTime;
  uint8_t address[6];
} announceMessage;
struct message_address{
  uint8_t messageType = MSG_HELLO;
  uint8_t address[6];
} addressMessage;

struct message_clap_time {
  uint8_t messageType = MSG_SEND_CLAP_TIME;
  int clapCounter;
  uint32_t timeStamp; //offsetted.
} clapTime;


struct animate {
  uint8_t messageType = MSG_ANIMATION; 
  uint8_t animationType;
  uint16_t speed;
  uint16_t delay;
  uint16_t reps;
  uint8_t rgb1[3];
  uint8_t rgb2[3];
  uint32_t startTime;
} animationMessage;




//-----------
//Timer config variables
//-----------
uint32_t msgSendTime;
uint32_t msgArriveTime;
uint32_t msgReceiveTime;
int timerCounter = 0;
int lastDelay = 0;
int oldTimerCounter = 0;
int delayAvg = 0;


//calibration
int sensorValue;
int clapCounter = 0;
int lastClap = 0;
uint32_t lastClapTime;


//receive addresses
int addressCounter = 0;



struct client_address {
  uint8_t address[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  int id;
  float xLoc;
  float yLoc;
  float zLoc;
} ;
client_address clientAddresses[NUM_DEVICES];



void printAddress(const uint8_t * mac_addr){
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.println(macStr);
}

void modeSwitch(int switchMode) {
  Serial.print("Switched mode to ");
  printMode(switchMode);
  mode = switchMode;
}



void IRAM_ATTR onTimer()
{   
  msgSendTime = micros();
    timerCounter++;
    //wait for timer vs wait for calibrate
    if (mode == MODE_SENDING_TIMER) {
      timerMessage.messageType = MSG_TIMER_CALIBRATION;
      
      timerMessage.sendTime = msgSendTime;
      timerMessage.counter = timerCounter;
      timerMessage.lastDelay = lastDelay;
      esp_err_t result = esp_now_send(timerReceiver, (uint8_t *) &timerMessage, sizeof(timerMessage));
    }
    else {
      //Serial.println("broadcasting");
      announceMessage.sendTime = msgSendTime;
      esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &announceMessage, sizeof(announceMessage));
    }

  //return true;
}


 
int addPeer(uint8_t * address) {
  memcpy(&peerInfo.peer_addr, address, 6);
  if (esp_now_get_peer(peerInfo.peer_addr, &peerInfo) == ESP_OK) {
    Serial.println("Found Peer");
    return 0;
  }
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
    // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return -1;
  }
  else {
    Serial.println("Added Peer");
    return 1;
  }
}

void removePeer(uint8_t address[6]) {
  if (esp_now_del_peer(address) != ESP_OK) {
    Serial.println("coudln't delete peer");
    return;
  }
}

void  OnDataRecv(const esp_now_recv_info * mac, const uint8_t *incomingData, int len) {
  Serial.print("received ");
  printMessage(incomingData[0]);
  

  switch (incomingData[0]) {
    case MSG_HELLO: 
    Serial.println("received hallo");
    memcpy(&addressMessage,incomingData,sizeof(addressMessage));
    for (int i = 0; i < NUM_DEVICES; i++) {
      if (memcmp(clientAddresses[i].address, emptyAddress, 6) == 0) {
        Serial.println("need to add peer");
          memcpy(clientAddresses[i].address, addressMessage.address, 6);
          memcpy(&timerReceiver, mac->src_addr, 6);
          addPeer(timerReceiver);
          addressCounter++;
          modeSwitch(MODE_SENDING_TIMER);
          break;
        }
        else if (memcmp(&clientAddresses[i].address, &addressMessage.address, 6) == true) {
          Serial.print("found: ");
          printAddress(addressMessage.address);
          modeSwitch(MODE_SENDING_TIMER);
          break;
        }
      }
    break;
    case MSG_GOT_TIMER: 
      Serial.println("GOT TIMER");
      removePeer(timerReceiver);
      timerCounter = 0;
      lastDelay = 0;
      modeSwitch(MODE_ANIMATE);
      break;
    case MSG_SEND_CLAP_TIME:
      Serial.println("GOT CLAP");
      memcpy(&clapTime,incomingData,sizeof(clapTime));
      Serial.println("--");
      break;

    default: 
      Serial.println("MSG NOT RECOGNIZED");
      Serial.println(incomingData[0]);
    } 

  }




void  OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t sendStatus) {
  if (mode == MODE_SENDING_TIMER) {
    Serial.print("Sent Timer to  ");
    printAddress(mac_addr);
    if (sendStatus == ESP_NOW_SEND_SUCCESS) {
      msgArriveTime = micros();
      lastDelay = msgArriveTime-msgSendTime;
    }
    else {
     msgArriveTime = 0;
    }
  }
}


void setup() {
  Serial.begin(115200);

  timer = timerBegin(1000000);           	// timer 0, prescalar: 80, UP counting
  timerAttachInterrupt(timer, &onTimer); 	// Attach interrupt
  timerWrite(timer, 0);  		// Match value= 1000000 for 1 sec. delay.
  timerStart(timer);   
  timerAlarm(timer, 1000000, true, 0);

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  //esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  memcpy(&peerInfo.peer_addr, broadcastAddress, 6);
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
  if (DEVICE == D1) {
    audioPin = 35;
  }
  pinMode(audioPin, INPUT); 
  peakDetection.begin(30, 3, 0);   
  lastClap = millis();
  timerCounter = 0;
  modeSwitch(MODE_SEND_ANNOUNCE);
}

void loop() {
    double data = (double)analogRead(audioPin)/512-1;  // converts the sensor value to a range between -1 and 1
    peakDetection.add(data);                     // adds a new data point
    int peak = peakDetection.getPeak();          // 0, 1 or -1
    double filtered = peakDetection.getFilt();   // moving average
    if (peak == -1 and lastClap+1000 < millis()) {
      lastClapTime = micros();
      lastClap = millis();
      Serial.println("Clapped at ");
      Serial.println(lastClapTime);
      // print peak status
    }
    if (lastClap+5000 < millis()) {
      Serial.println("still alive");
      lastClap = millis();
    }
    if (clapTime.clapCounter > oldClapCounter) {
      oldClapCounter = clapTime.clapCounter;
      Serial.println(clapTime.timeStamp);
      Serial.print("Clap received at Base: ");
      Serial.println(lastClapTime);
      Serial.print("Difference ");
      Serial.print(clapTime.timeStamp - lastClapTime);
      Serial.print("ms \nIn Meters ");
      double inMeters = (0.343*(clapTime.timeStamp-lastClapTime))/1000;
      Serial.println((inMeters));
      delay(5000);

    }
  
}
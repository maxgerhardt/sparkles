#define V1 1
#define V2 2 
#define D1 3
#ifndef DEVICE_USED
#define DEVICE_USED V2
#endif
#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <PeakDetection.h> 
#include <ledHandler.h>
#include <stateMachine.h>
#include <messaging.h>

//#include "ESP32TimerInterrupt.h"
//#include "driver/gptimer.h"
#define TIMER_INTERVAL_MS       600
#define USING_TIM_DIV1 true

        uint8_t myAddress[6];

int mode;
#define NUM_DEVICES 20
hw_timer_t * timer = NULL;
PeakDetection peakDetection; 
ledHandler handleLed;
modeMachine modeHandler;
messaging messageHandler;
message_send_clap_times clapTimes;

int interruptCounter;  //for counting interrupt
int totalInterruptCounter;   	//total interrupt counting
bool start = true;

esp_now_peer_info_t peerInfo;

int audioPin = 5;
int oldClapCounter = 0;
int cycleCounter = 0;




//-----------
//Timer config variables
//-----------
unsigned long msgSendTime;
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
client_address clientAddresses[NUM_DEVICES];

void IRAM_ATTR onTimer()
{   
 msgSendTime = micros();
  //Serial.println(msgSendTime/1000);
    timerCounter++;
    //wait for timer vs wait for calibrate
    
    if (modeHandler.getMode() == MODE_SENDING_TIMER) {
      messageHandler.timerMessage.messageType = MSG_TIMER_CALIBRATION; 
      messageHandler.timerMessage.sendTime = msgSendTime;
      messageHandler.timerMessage.counter = timerCounter;
      messageHandler.timerMessage.lastDelay = lastDelay;
      esp_err_t result = esp_now_send(messageHandler.timerReceiver, (uint8_t *) &messageHandler.timerMessage, sizeof(messageHandler.timerMessage));
      
    }
    else if (modeHandler.getMode() != MODE_CALIBRATE and modeHandler.getMode() != MODE_NEUTRAL) {
      //todo announce thing raus und einfach host address hardcoden.
      messageHandler.announceMessage.sendTime = msgSendTime;
      esp_err_t result = esp_now_send(messageHandler.broadcastAddress, (uint8_t *) &messageHandler.announceMessage, sizeof(messageHandler.announceMessage));
    }

}


void  OnDataRecv(const esp_now_recv_info * mac, const uint8_t *incomingData, int len) {
  msgReceiveTime = micros();
  messageHandler.pushDataToReceivedQueue(mac, incomingData, len, msgReceiveTime);
}




void  OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t sendStatus) {

  if (modeHandler.getMode() == MODE_SENDING_TIMER) {
    if (sendStatus == ESP_NOW_SEND_SUCCESS) {
      msgArriveTime = micros();
      lastDelay = msgArriveTime-msgSendTime;
    }
    else {
      messageHandler.addError("No SUCCESS");
     msgArriveTime = 0;
    }
  }
  else if (modeHandler.getMode() != MODE_SEND_ANNOUNCE) {
     Serial.print("sent ");
     Serial.println(sendStatus);
  }
}


void setup() {
  Serial.begin(115200);
  delay(5000);
  timer = timerBegin(1000000);           	// timer 0, prescalar: 80, UP counting
  timerAttachInterrupt(timer, &onTimer); 	// Attach interrupt
  timerWrite(timer, 0);  		// Match value= 1000000 for 1 sec. delay.
  timerStart(timer);   
  timerAlarm(timer, TIMER_INTERVAL_MS*1000, true, 0);

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  handleLed.setup();
  messageHandler.setup(modeHandler, handleLed, peerInfo);

  //esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  memcpy(&peerInfo.peer_addr, messageHandler.broadcastAddress, 6);
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
  WiFi.macAddress(messageHandler.announceMessage.address);
  if (DEVICE_USED == D1) {
    audioPin = 35;
  }
  //pinMode(audioPin, INPUT); 
  peakDetection.begin(30, 3, 0);   
  lastClap = millis(); 
  timerCounter = 0;
  modeHandler.switchMode(MODE_SEND_ANNOUNCE);
  WiFi.macAddress(myAddress);
  
}

void loop() {
  messageHandler.handleErrors();
  messageHandler.processDataFromReceivedQueue();
  messageHandler.processDataFromSendQueue();
  if (lastClap+5000 < millis()) {
    modeHandler.printCurrentMode();
    lastClap = millis();
    cycleCounter++;
    Serial.print(DEVICE_MODE);
    Serial.print("-----\nMain still Alive ");
    Serial.println(cycleCounter);
    Serial.println("My Address: "+messageHandler.stringAddress(myAddress));
    Serial.println("---All addresses----");
    messageHandler.printAllAddresses();
    Serial.println("---All addresses----");
    //messageHandler.printAllAddresses();
    Serial.println(messageHandler.getMessageLog());
    Serial.println("-----");
    //messageHandler.printAddress(myAddress);

  }

  if (modeHandler.getMode() == MODE_CALIBRATE) {
    double data = (double)analogRead(audioPin)/512-1;
    peakDetection.add(data); 
    int peak = peakDetection.getPeak(); 
    double filtered = peakDetection.getFilt(); 
    //Serial.println(sensorValue);
    if (peak == -1 and millis() > lastClap+1000) {
      clapTimes.timeStamp[clapCounter] = micros();
      lastClap = millis();
      clapCounter++;
    }
  }
  if (messageHandler.clapsReceived == messageHandler.addressCounter) {
    Serial.println("All clap Times received");
    messageHandler.clapsReceived = 0;
  }
}
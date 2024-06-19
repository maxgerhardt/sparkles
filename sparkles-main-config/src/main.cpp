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
#include <LittleFS.h>
#define SWITCH_PIN 39
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
bool lfs_started = true;
esp_now_peer_info_t peerInfo;

int audioPin = 5;
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
int lastClap = 0;
uint32_t lastClapTime;


//receive addresses
//client_address clientAddresses[NUM_DEVICES];

void IRAM_ATTR onTimer()
{   
 msgSendTime = micros();
  //Serial.println(msgSendTime/1000);
    timerCounter++;
    //wait for timer vs wait for calibrate
    if (timerCounter == 100) {
      modeHandler.switchMode(MODE_NEUTRAL);
      messageHandler.setTimerReceiverUnavailable();
      return;
    }    
    if (modeHandler.getMode() == MODE_SENDING_TIMER or modeHandler.getMode() == MODE_RESET_TIMER) {

      messageHandler.timerMessage.messageType = MSG_TIMER_CALIBRATION; 
      messageHandler.timerMessage.sendTime = msgSendTime;
      messageHandler.timerMessage.counter = timerCounter;
      messageHandler.timerMessage.lastDelay = lastDelay;
      messageHandler.addError("Sending Timer+ "+String(timerCounter)+"\n");
      if (modeHandler.getMode() == MODE_RESET_TIMER) {
        messageHandler.timerMessage.reset = true;
      }
      messageHandler.addError(messageHandler.stringAddress(messageHandler.timerReceiver)+"\n");

      esp_err_t result = esp_now_send(messageHandler.timerReceiver, (uint8_t *) &messageHandler.timerMessage, sizeof(messageHandler.timerMessage));
      
    }
    /*else if (modeHandler.getMode() != MODE_CALIBRATE and modeHandler.getMode() != MODE_NEUTRAL) {
      //todo announce thing raus und einfach host address hardcoden.
      messageHandler.announceMessage.sendTime = msgSendTime;
      esp_err_t result = esp_now_send(messageHandler.broadcastAddress, (uint8_t *) &messageHandler.announceMessage, sizeof(messageHandler.announceMessage));
    }*/

}


void  OnDataRecv(const esp_now_recv_info * mac, const uint8_t *incomingData, int len) {
  Serial.println("rcvd");
  msgReceiveTime = micros();
  messageHandler.pushDataToReceivedQueue(mac, incomingData, len, msgReceiveTime);
}




void  OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t sendStatus) {
  if (modeHandler.getMode() == MODE_PING_RESET or modeHandler.getMode() == MODE_RESET_TIMER) {
    if (sendStatus == ESP_NOW_SEND_SUCCESS) {
      modeHandler.switchMode(MODE_RESET_TIMER);
    }
    else {
      messageHandler.addError("Ping reset not sent\n");      
    }
  }
  if (modeHandler.getMode() == MODE_SENDING_TIMER or modeHandler.getMode() == MODE_RESET_TIMER) {
    if (sendStatus == ESP_NOW_SEND_SUCCESS) {
      msgArriveTime = micros();
      lastDelay = msgArriveTime-msgSendTime;
      messageHandler.addError("SUCCESS "+String(lastDelay)+"\n");
    }
    else {
      messageHandler.addError("No SUCCESS");
     msgArriveTime = 0;
    }
  }
  else{
     if (sendStatus == ESP_NOW_SEND_SUCCESS) {
       messageHandler.addError("Sent to  "+messageHandler.stringAddress(mac_addr)+"\n");
     }
     else {
       messageHandler.addError("Not sent to "+messageHandler.stringAddress(mac_addr)+"\n");
     }

  }
}


void setup() {
  Serial.begin(115200);
  unsigned long startTime = millis();
  while (!Serial) {
    if (millis() - startTime > 2000) {
      break;
    }
  }
  if (!LittleFS.begin()) {
    Serial.println("LittleFS mount failed");
    lfs_started = false;
  }
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
  modeHandler.switchMode(MODE_NEUTRAL);
  messageHandler.setup(modeHandler, handleLed, peerInfo);
  Serial.println("after setup");
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
  WiFi.macAddress(myAddress);
  pinMode(SWITCH_PIN, INPUT_PULLDOWN); 
  randomSeed(analogRead(33));
  
}

void loop() {
  messageHandler.processDataFromReceivedQueue();
  messageHandler.processDataFromSendQueue();
  if (digitalRead(SWITCH_PIN) == HIGH) {
    if (LittleFS.format()) {
      Serial.println("LittleFS formatted successfully");
    }
    else {
      Serial.println("LittleFS formatting failed");
    }
    delay(5000);
    ESP.restart();
  }
  if (lastClap+5000 < millis()) {
    if (lfs_started == false) {
      if (LittleFS.format()) {
        Serial.println("LittleFS formatted successfully, retrying mount...");
        if (LittleFS.begin()) {
            Serial.println("LittleFS mount succeeded after formatting");
            lfs_started = true;
        } else {
            Serial.println("LittleFS mount failed after formatting");
            lfs_started = false;
        }
      } else {
        Serial.println("LittleFS formatting failed");
        lfs_started = false;
      }
    }
    messageHandler.handleErrors();
    modeHandler.printCurrentMode();
    lastClap = millis();
    cycleCounter++;
    Serial.print(DEVICE_MODE);
    Serial.print("-----\nMain still Alive ");
    Serial.println(cycleCounter);
    Serial.println("My Address: "+messageHandler.stringAddress(myAddress));
    Serial.println("---All addresses---- "+String(messageHandler.addressCounter));
    messageHandler.printAllAddresses();
    Serial.println("-----------");
    //messageHandler.printAllAddresses();
    Serial.println(messageHandler.getMessageLog());
    Serial.println("-----");
    int* sysTime = messageHandler.getSystemTime();
    Serial.println("Time is: "+String(sysTime[0])+":"+String(sysTime[1])+":"+String(sysTime[2]));
    //messageHandler.printAddress(myAddress);

  }

  if (modeHandler.getMode() == MODE_CALIBRATE) {
    double data = (double)analogRead(audioPin)/512-1;
    peakDetection.add(data); 
    int peak = peakDetection.getPeak(); 
    double filtered = peakDetection.getFilt(); 
    //Serial.println(sensorValue);
    if (peak == -1 and millis() > lastClap+1000) {
      messageHandler.addClap(micros());
      lastClap = millis();
      Serial.println("Clap!");
    }
  }
  if (messageHandler.clapsReceived == messageHandler.addressCounter && messageHandler.clapsReceived != 0) {
    Serial.println("All clap Times received");
    //messageHandler.clapsReceived = 0;
  }
  if (modeHandler.getMode() != MODE_SENDING_TIMER and modeHandler.getMode() != MODE_RESET_TIMER and modeHandler.getMode() != MODE_PING_RESET) {
    messageHandler.handleTimerUpdates();
  }
  if (modeHandler.getMode() == MODE_ANIMATE) {
    messageHandler.nextAnimation();
  }
  
  messageHandler.goodNight();
}
#define V1 1
#define V2 2 
#define D1 3
#define DEVICE_USED V2
#define DEVICE DEVICE_USED
#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <PeakDetection.h> 
#include <ledHandler.h>
#include <stateMachine.h>
#include <messaging.h>

//#include "ESP32TimerInterrupt.h"
//#include "driver/gptimer.h"
#define TIMER_INTERVAL_MS       500
#define USING_TIM_DIV1 true

        uint8_t myAddress[6];

int mode;
#define NUM_DEVICES 20
hw_timer_t * timer = NULL;
PeakDetection peakDetection; 
ledHandler handleLed;
modeMachine modeHandler;
messaging messageHandler;

int interruptCounter;  //for counting interrupt
int totalInterruptCounter;   	//total interrupt counting
bool start = true;

esp_now_peer_info_t peerInfo;

int audioPin = 5;
int oldClapCounter = 0;




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
      Serial.println("Sending timer msg");
      esp_err_t result = esp_now_send(messageHandler.timerReceiver, (uint8_t *) &messageHandler.timerMessage, sizeof(messageHandler.timerMessage));
      
    }
    else {
      //Serial.println("broadcasting");
      messageHandler.announceMessage.sendTime = msgSendTime;
      esp_err_t result = esp_now_send(messageHandler.broadcastAddress, (uint8_t *) &messageHandler.announceMessage, sizeof(messageHandler.announceMessage));
    }

  //return true;
}


void  OnDataRecv(const esp_now_recv_info * mac, const uint8_t *incomingData, int len) {
  Serial.print("received ");
  messageHandler.printMessage(incomingData[0]);
  

  switch (incomingData[0]) {
    case MSG_HELLO: 
      messageHandler.setTimerReceiver(incomingData);
    break;
    case MSG_GOT_TIMER: 
      messageHandler.removePeer(messageHandler.timerReceiver);
      timerCounter = 0;
      lastDelay = 0;
      modeHandler.switchMode(MODE_ANIMATE);
      break;
    case MSG_SEND_CLAP_TIME:
      messageHandler.handleClapTime(incomingData);

      break;
    case MSG_ANNOUNCE:
      Serial.println("why did i receive an announce message");
      messageHandler.printAddress(mac->src_addr);
      break;
    default: 
      Serial.println("MSG NOT RECOGNIZED");
      Serial.println(incomingData[0]);
    } 

  }




void  OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t sendStatus) {
  if (modeHandler.getMode() == MODE_SENDING_TIMER) {
    Serial.print("Sent Timer to  ");
    messageHandler.printAddress(mac_addr);
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
  timerAlarm(timer, TIMER_INTERVAL_MS*1000, true, 0);

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
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
  if (DEVICE == D1) {
    audioPin = 35;
  }
  pinMode(audioPin, INPUT); 
  peakDetection.begin(30, 3, 0);   
  lastClap = millis();
  timerCounter = 0;
  modeHandler.switchMode(MODE_SEND_ANNOUNCE);
  WiFi.macAddress(myAddress);
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
      messageHandler.printAddress(messageHandler.announceMessage.address);
      handleLed.flash(0, 255, 0, 200, 2, 50);
      lastClap = millis();
      Serial.println("MessageHandler Timer receiver ");
      messageHandler.printAddress(messageHandler.timerReceiver);
      Serial.println("myAddress)");
      messageHandler.printAddress(myAddress);
    }
    if (messageHandler.clapTime.clapCounter > oldClapCounter) {
      oldClapCounter = messageHandler.clapTime.clapCounter;
      Serial.println(messageHandler.clapTime.timeStamp);
      Serial.print("Clap received at Base: ");
      Serial.println(lastClapTime);
      Serial.print("Difference ");
      Serial.print(messageHandler.clapTime.timeStamp - lastClapTime);
      Serial.print("ms \nIn Meters ");
      double inMeters = (0.343*(messageHandler.clapTime.timeStamp-lastClapTime))/1000;
      Serial.println((inMeters));
      delay(5000);

    }
  
}
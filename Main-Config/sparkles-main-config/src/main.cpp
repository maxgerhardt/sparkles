#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <PeakDetection.h> 
#include <messaging.cpp>
#include <stateMachine.h>
#include <stateMachine.cpp>

//#include "ESP32TimerInterrupt.h"
//#include "driver/gptimer.h"
#define TIMER_INTERVAL_MS       1000
#define USING_TIM_DIV1 true

int mode;

//BOARDS
#define V1 1
#define V2 2 
#define D1 3

hw_timer_t * timer = NULL;
PeakDetection peakDetection; 

int interruptCounter;  //for counting interrupt
int totalInterruptCounter;   	//total interrupt counting


int audioPin = 5;
int oldClapCounter = 0;
modeMachine modeHandler;
messaging messageHandler(modeHandler);





//-----------
//Timer config variables
//-----------
int timerCounter = 0;
int lastDelay = 0;
int oldTimerCounter = 0;
int delayAvg = 0;


//calibration
int sensorValue;
int clapCounter = 0;
int lastClap = 0;
uint32_t lastClapTime;






void IRAM_ATTR onTimer()
{   
  messageHandler.setSendTime(micros);
    timerCounter++;
    //wait for timer vs wait for calibrate
    if (mode == MODE_SENDING_TIMER) {
      messageHandler.timerMessage.messageType = MSG_TIMER_CALIBRATION;
      
     messageHandler.timerMessage.sendTime = messageHandler.getSendTime();
     messageHandler.timerMessage.counter = timerCounter;
     messageHandler.timerMessage.lastDelay = lastDelay;
      esp_err_t result = esp_now_send(timerReceiver, (uint8_t *) &messageHandler.timerMessage, sizeof(messageHandler.timerMessage));
    }
    else {
      //Serial.println("broadcasting");
      messageHandler.announceMessage.sendTime = messageHandler.getSendTime();
      esp_err_t result = esp_now_send(messageHandler.broadcastAddress,  (uint8_t *) &messageHandler.announceMessage, sizeof(messageHandler.announceMessage));
    }

  //return true;
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

    // Add peer        

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
#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <PeakDetection.h> 
#include <messaging.cpp>

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
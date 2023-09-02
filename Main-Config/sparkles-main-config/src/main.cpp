#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include "ESP32TimerInterrupt.h"
#define TIMER_INTERVAL_MS       1000
#define USING_TIM_DIV1 true
#define SEND_ADDRESS 1
#define CALIBRATE 2
#define RETRIEVE_TIMES 3
#define END_CALIBRATION 4
#define ASSIGN_ID 5
#define RETURN 6
int microphonePin = A0;
int timesince;
int timenow;
int sensorValue;
int lastClap;
typedef struct claps {
  int timerCounter;
  int microsSince;
  int runtime;
} claps;
ESP32Timer ITimer(0);
uint32_t timerCounter = 0;
int mode = CALIBRATE;
uint8_t clientAddresses[300][6];
int clientAddressCounter = 0;
int clientAddressTimerCounter = 0;
//uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
//DC:4F:22:7D:D2:7D
//MAIN ESP32 
//uint8_t broadcastAddress[] = {0xB4,0xE6,0x2D,0xE9,0x3C,0x21};
//xiao address
//uint8_t broadcastAddress[] = {0x34,0x85,0x18,0x5,0x88,0xb0};
//CLIENT ESP32
uint8_t broadcastAddress[] = {0x7c,0x87,0xce,0x2d,0xcf,0x98};
typedef struct struct_message {
  uint8_t address[6];
  int counter;
  int microtime;
  float timing;
  int mode;
  bool truefalse;
} struct_message;
struct_message myData;
esp_now_peer_info_t peerInfo;
int msgTime;
bool newMsgTime = false;


bool IRAM_ATTR TimerHandler(void * timerNo)
{ 
  timerCounter++;
 /*    String out;
    WiFi.macAddress(broadcastAddress);
  for (int i = 0; i<sizeof(broadcastAddress);i++){
      out = out+":"+String(broadcastAddress[i]);    
    }
    Serial.println(out);
    */
  myData.counter = timerCounter;
  
  //esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
  /*if (mode == SEND_ADDRESS) {
    if (clientAddressTimerCounter !=0 && timerCounter > clientAddressTimerCounter+ 2) {
      mode = CALIBRATE;
    }
    myData.mode = SEND_ADDRESS;
    esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
  }
  else*/
   if (mode == CALIBRATE ) {

     timesince = micros();
    myData.mode = CALIBRATE;
    myData.counter = timerCounter;
    myData.microtime = timesince;
    msgTime = micros();
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
   
    if (result == ESP_OK) {
      //newMsgTime = true;
    }
    else {
      //Serial.println("Sending error");
    }
    
  }
  else if (mode == RETRIEVE_TIMES) {
    //figure out a good algorithm to retrieve the runtimes from the individual clients
    //keep in mind that each client needs to be polled individually and we need a different kind of banddwith than "per second" for this
    //esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
    //ESP_NOW_SEND_SUCCESS in sending callback function if the data is received successfully on the MAC layer. Otherwise, it will return ESP_NOW_SEND_FAIL. 
  }
  return true;
}


void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int lenn) {
  msgTime = micros()-msgTime;
  newMsgTime = true;
  Serial.println ("Message Received");
  /*if (mode == SEND_ADDRESS) {
    
    memcpy(&clientAddresses[clientAddressCounter], myData.address, 6);
    clientAddressTimerCounter = timerCounter;
    clientAddressCounter++;
  }
  */

}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t sendStatus) {
  //Serial.println("SENDING2");
  //Serial.print("Last Packet Send Status: ");
  //Serial.println(sendStatus == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");

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
  mode = CALIBRATE;
  

}

void loop() {
 sensorValue = analogRead(microphonePin);
  timenow = micros();
  if (sensorValue < 50 and timenow - lastClap > 1000000) {
    Serial.print ("Clap at Counter: ");
    Serial.print (timerCounter);
    Serial.print (" time difference ");
    Serial.println(timenow-timesince);
    lastClap = timenow;
  }
   sensorValue = analogRead(microphonePin);
if (newMsgTime == true) {
  Serial.print ("Message time ");
  Serial.println(msgTime);
  newMsgTime = false;
}

// analog read on analog read

}


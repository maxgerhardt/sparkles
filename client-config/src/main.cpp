#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#define SEND_ADDRESS 1
#define CALIBRATE 2
#define RETRIEVE_TIMES 3
#define END_CALIBRATION 4
#define ASSIGN_ID 5
#define RETURN 6
#define DEBUG 1
int mode = CALIBRATE;
//uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t broadcastAddress[] = {0xB4,0xE6,0x2D,0xE9,0x3C,0x21};
typedef struct struct_message {
  uint8_t address[6];
  int counter;
  int microtime;
  float timing;
  int mode;
  bool truefalse;
} struct_message;
int timeOfLastMessage = 0;
struct_message myData;
int timerCounter = 0;
uint8_t mainAddress[6];
float times[100];
bool addressReceived = false;
int myID = 0;
int microphonePin = A0;
int sensorValue = 0;
int lastClap = 0;
uint32_t lastBeep = 0;
uint32_t loopcount = 0;
int timeNow;
//master address

void sendTimes(int index) {
  struct_message returnData;
  returnData.mode = RETURN;
  returnData.counter = index;
  returnData.timing = times[index];
  esp_now_send(mainAddress, (uint8_t *) &returnData, sizeof(returnData));
}

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  //Serial.println("Received")
  if (len == sizeof(myData)) {
    memcpy(&myData, incomingData, sizeof(myData));
  }
  Serial.println("Sending Message Back") ;
  esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
  if (DEBUG == 1) {
    Serial.print("Mode ");
    Serial.println(myData.mode);
    Serial.print("TImer Counter ");
    Serial.println(myData.counter);
    Serial.print("Microtime : ");
    Serial.println(myData.microtime);
  }
  timeOfLastMessage = micros();
  if (myData.mode == CALIBRATE) {
    if (mode != CALIBRATE) {
      mode = CALIBRATE;
    }
    if (timerCounter != myData.counter){
    timerCounter = myData.counter;
    //Serial.println(timerCounter);
    }
    memcpy (&mainAddress, myData.address, sizeof(myData.address));

     //print out local mac address
     /*
    String out;
    WiFi.macAddress(broadcastAddress);
  for (int i = 0; i<sizeof(broadcastAddress);i++){
      out = out+":"+String(broadcastAddress[i]);    
    }
    Serial.println(out);
    */
    
  }
  if (myData.mode == END_CALIBRATION) {
    mode = 0;
  }
  if (myData.mode == RETRIEVE_TIMES) {
    if (mode != RETRIEVE_TIMES) {
      mode = RETRIEVE_TIMES;
    }
    sendTimes(myData.counter);
  }
  if (myData.mode == SEND_ADDRESS) { 
    if (myID == 0) {
      struct_message returnData;
      WiFi.macAddress(returnData.address);
      esp_now_send(myData.address, (uint8_t *) &returnData, sizeof(returnData));
    }
  }
  if (myData.mode == ASSIGN_ID) {
    myID = myData.counter;
  }

}

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);  
}

void loop() {
  if (mode == CALIBRATE or true) {
    sensorValue = analogRead(microphonePin);
    if (sensorValue < 50 and timerCounter > lastClap) {
      Serial.print ("Clap at Counter: ");
      Serial.print (timerCounter);
      timeNow = micros();
      Serial.print (" time difference ");
      Serial.println(timeNow-timeOfLastMessage);
      lastClap = timerCounter;
    }
  }
  if (DEBUG == 2) {
    Serial.println(analogRead(microphonePin));
    delay(100);
  }
}
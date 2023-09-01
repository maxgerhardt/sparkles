#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#define SEND_ADDRESS 1
#define CALIBRATE 2
#define RETRIEVE_TIMES 3
#define END_CALIBRATION 4
#define ASSIGN_ID 5
#define RETURN 6
int mode = 0;
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
typedef struct struct_message {
  uint8_t address[6];
  int counter;
  float timing;
  int mode;
  bool truefalse;
} struct_message;
int lastTime = 0;
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
void sendTimes(int index) {
  struct_message returnData;
  returnData.mode = RETURN;
  returnData.counter = index;
  returnData.timing = times[index];
  esp_now_send(mainAddress, (uint8_t *) &returnData, sizeof(returnData));
}

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {

  Serial.println("Received");
  if (len == sizeof(myData)) {
    memcpy(&myData, incomingData, sizeof(myData));
  }
  //Serial.println(myData.mode);
  lastTime = micros();
  if (myData.mode == CALIBRATE) {
    if (mode != CALIBRATE) {
      mode = CALIBRATE;
    }
    if (timerCounter != myData.counter){
    timerCounter = myData.counter;
    timerCounter++;
    }
    memcpy (&mainAddress, myData.address, sizeof(myData.address));
    String out;
    /*WiFi.macAddress(broadcastAddress);
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
/*
 WiFi.macAddress(broadcastAddress);
  String out;
  for (int i = 0; i<sizeof(broadcastAddress);i++){
      out = out+":"+String(broadcastAddress[i]);    
    }
    Serial.println(out);
  */
  sensorValue = analogRead(microphonePin);
  if (sensorValue < 50 and timerCounter > lastClap) {
    Serial.print ("Clap at Counter: ");
    Serial.print (timerCounter);
    timeNow = micros();
    Serial.print (" time difference ");
    Serial.println(timeNow-lastTime);
    lastClap = timerCounter;
  }
  if (timerCounter > lastClap) {
    Serial.println(sensorValue);
    lastClap = timerCounter;
    delay(500);
  }
  if (mode == CALIBRATE) {
    //analogRead
  }
   sensorValue = analogRead(microphonePin);
  loopcount++;
  if (sensorValue < 50 and loopcount > lastBeep+2000) {
    Serial.print ("Beep ");
    lastBeep = loopcount;
    Serial.println(lastBeep);
  }
}
#define V1 1
#define V2 2
#define D1 3
#define DEVICE_USED V2
#define DEVICE V2

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <PeakDetection.h> 
#include <ledHandler.h>



#include <messaging.h>
#include <stateMachine.h>
#define CALIBRATION_FREQUENCY 1000
int freq = 5000;
int resolution = 8;



bool test = true;
int audioPin = 5;

PeakDetection peakDetection; 
ledHandler handleLed;
modeMachine modeHandler;

messaging messageHandler(&modeHandler, &handleLed);

//calibration stuff
int sensorValue;
int microphonePin = A0;
int clapCounter = 0;
int lastClap = 0;
bool clapSent = false;

unsigned long msgReceiveTime;






/*
void OnDataRecv(const esp_now_recv_info * mac, const uint8_t *incomingData, int len) {
  Serial.print("received ");
  messageHandler.printMessage(incomingData[0]);
  msgReceiveTime = micros();
  memcpy(&messageHandler.hostAddress, mac->src_addr, 6);
  switch (incomingData[0]) {
    case MSG_ANNOUNCE: 
    {
      if (modeHandler.getMode() == MODE_WAIT_FOR_ANNOUNCE) {
        if (messageHandler.addPeer(messageHandler.hostAddress) == 1) {
          Serial.println("Adding peer ");
          handleLed.flash(0, 125, 125, 200, 1, 50);
          memcpy(&messageHandler.announceMessage, incomingData, sizeof(messageHandler.announceMessage));
          esp_now_send(messageHandler.hostAddress, (uint8_t*) &messageHandler.addressMessage, sizeof(messageHandler.addressMessage));
          //modeSwitch(MODE_WAIT_FOR_TIMER);
        }
      }
      
    }
      break;
    case MSG_TIMER_CALIBRATION:  
    { 
      messageHandler.addPeer(messageHandler.hostAddress);
      memcpy(&messageHandler.timerMessage,incomingData,sizeof(messageHandler.timerMessage));
      handleLed.flash(125, 0, 0, 200, 1, 50);
      messageHandler.receiveTimer(msgReceiveTime);
    }
      break;
    case MSG_ANIMATION:
    Serial.println("animation message incoming");
    if (modeHandler.getMode() == MODE_ANIMATE) {
      Serial.println("Calling Blink");
      memcpy(&messageHandler.animationMessage, incomingData, sizeof(messageHandler.animationMessage));
      Serial.print("Animation Message speed");
      Serial.println(messageHandler.animationMessage.speed);
      handleLed.candle(messageHandler.animationMessage.speed, messageHandler.animationMessage.reps,messageHandler.animationMessage.delay);
    }
      break;
    default: 
      Serial.println("Data type not recognized");
  }
  
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t sendStatus) {
  //turn into MODE WAIT FOR TIMER if message to host is saved.
  Serial.println("sent message to");
  messageHandler.printAddress(mac_addr);
  modeHandler.printMode(modeHandler.getMode());
  if (modeHandler.getMode() == MODE_WAIT_FOR_ANNOUNCE) {
    if (sendStatus == ESP_NOW_SEND_SUCCESS) {
      modeHandler.switchMode(MODE_WAIT_FOR_TIMER);
    }
  }
  else if (modeHandler.getMode() == MODE_WAIT_FOR_TIMER) {
    Serial.println("should send");
    if (sendStatus == ESP_NOW_SEND_SUCCESS) {
      modeHandler.switchMode(MODE_ANIMATE);
    }
  }
    else {
      //esp_now_send(broadcastAddress,(uint8_t *) &timerReceivedMessage, sizeof(timerReceivedMessage) );
    }
}

*/



void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  //WiFi.macAddress(messageHandler.myAddress);
/*
  // Init ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
 // memcpy(messageHandler.peerInfo.peer_addr, messageHandler.broadcastAddress, 6);
 // messageHandler.peerInfo.channel = 0;  
 // messageHandler.peerInfo.encrypt = false;
    // Add peer        
if (esp_now_add_peer(&messageHandler.peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
  modeHandler.switchMode(MODE_WAIT_FOR_ANNOUNCE);
  esp_now_register_recv_cb(OnDataRecv);  
  esp_now_register_send_cb(OnDataSent);
   WiFi.macAddress(messageHandler.addressMessage.address);
   esp_now_get_peer_num(&messageHandler.peerNum);

  Serial.print("Number of Peers start: ");
  Serial.println(messageHandler.peerNum.total_num);
  */
  if (DEVICE != D1){

  
  //handleLed.ledsOff();
  }
  else {
    audioPin = 35;
  }
 Serial.println("huch");
  pinMode(audioPin, INPUT); 
  peakDetection.begin(30, 3, 0);   
  delay(1000);
    //messageHandler.clapTime.clapCounter = 0;
  handleLed.flash(0, 255, 0, 200, 2, 50);
}

void loop() {
  /*
if (modeHandler.getMode() == MODE_ANIMATE) {
  //flash(0, 255, 0, 200, 2, 50);

 
  double data = (double)analogRead(audioPin)/512-1;
  peakDetection.add(data); 
  int peak = peakDetection.getPeak(); 
  double filtered = peakDetection.getFilt(); 
  //Serial.println(sensorValue);
  if (peak == -1 and millis() > lastClap+1000) {
    messageHandler.clapTime.clapCounter++;
    messageHandler.clapTime.timeStamp = micros()-messageHandler.getTimeOffset();
    Serial.print("Clap Time ") ;
    Serial.println(messageHandler.clapTime.timeStamp);
    Serial.print("Clap Counter ");
    Serial.println(messageHandler.clapTime.clapCounter);
    esp_now_send(messageHandler.hostAddress, (uint8_t *) &messageHandler.clapTime, sizeof(messageHandler.clapTime));
    handleLed.flash();
    //esp_now_send(broadcastAddress, (uint8_t *) &blinkMessage, sizeof(blinkMessage));
    lastClap = millis();
  }
  else if (millis()>(lastClap+5000)) 
  {
    Serial.println("Still alive");
    lastClap = millis();
    handleLed.flash(0, 255, 0, 200, 2, 50);
  }


*/
  if (millis()>(lastClap+5000)) 
  {
    Serial.println("Still alive");
    lastClap = millis();
    handleLed.flash(0, 255, 0, 200, 2, 50);
  }

/*    sensorValue = analogRead(microphonePin);
    if (sensorValue < 50 and millis() > lastClap+1000) {
      blinkMessage.color[0] = random(128);
       blinkMessage.color[1] = random(128);
       blinkMessage.color[2] = random(128);
       esp_now_send(broadcastAddress, (uint8_t *) &blinkMessage, sizeof(blinkMessage));
      lastClap = millis();
      clapCounter++;
      
    }
  }*/


    //printAddress(addressMessage.address);


} 
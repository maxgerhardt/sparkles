#define V1 1
#define V2 2 
#define D1 3
#define DEVICE_USED V2
#define DEVICE DEVICE_USED
#if (DEVICE == V1)
#define AUDIO_PIN 5
#elif (DEVICE == V2)
#define AUDIO_PIN 5
#elif (DEVICE == D1)
#define AUDIO_PIN 35
#endif
#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <PeakDetection.h> 
#include <ledHandler.h>
#include <messaging.h>
#include <stateMachine.h>
#define TIMER_INTERVAL_MS       1000
#define USING_TIM_DIV1 true
//BOARDS
hw_timer_t * timer = NULL;
PeakDetection peakDetection; 
   esp_now_peer_info_t peerInfo;
    esp_now_peer_num_t peerNum;
int oldClapCounter = 0;
modeMachine modeHandler;
ledHandler handleLed;
messaging messageHandler;

//calibration
int sensorValue;
int clapCounter = 0;
int lastClap = 0;
uint32_t lastClapTime;

struct message_announce2 {
  uint8_t messageType = MSG_ANNOUNCE;
  unsigned long sendTime;
  uint8_t address[6];
} announceMessage2;

uint8_t broadcastAddress[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

void printAddress(const uint8_t * mac_addr){
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
            mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    Serial.println(macStr);
}


void IRAM_ATTR onTimer()
{   
  Serial.println("Timer Called");
  modeHandler.printMode(modeHandler.getMode());
  messageHandler.setSendTime(micros());
  messageHandler.incrementTimerCounter();
    //wait for timer vs wait for calibrate
  if (modeHandler.getMode() == MODE_SENDING_TIMER) {

    messageHandler.prepareTimerMessage();
    esp_err_t result = esp_now_send(messageHandler.timerReceiver, (uint8_t *) &messageHandler.timerMessage, sizeof(messageHandler.timerMessage));
  }
  else {
    Serial.println("broadcasting");
    //messageHandler.prepareAnnounceMessage();
    esp_err_t result = esp_now_send(broadcastAddress,  (uint8_t *) &announceMessage2, sizeof(announceMessage2));
    Serial.println(esp_err_to_name(result));
  }

  //return true;
}

void OnDataRecv(const esp_now_recv_info * mac, const uint8_t *incomingData, int len) {
    Serial.print("Received ");
    messageHandler.printMessage(incomingData[0]);
    switch (incomingData[0]) {
        case MSG_HELLO: 
        memcpy(&messageHandler.addressMessage,incomingData,sizeof(messageHandler.addressMessage));
            messageHandler.handleAddressMessage(mac);
        break;
        case MSG_GOT_TIMER: 
            messageHandler.handleGotTimer() ;
            break;
        case MSG_SEND_CLAP_TIME:
            memcpy(&messageHandler.clapTime,incomingData,sizeof(messageHandler.clapTime));
        break;

        default: 
        Serial.println("MSG NOT RECOGNIZED");
        Serial.println(incomingData[0]);
        } 

  }


void  OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t sendStatus) {
  Serial.println("Sent");
  if (modeHandler.getMode() == MODE_SENDING_TIMER) {
    Serial.print("Sent Timer to  ");
    messageHandler.printAddress(mac_addr);
    if (sendStatus == ESP_NOW_SEND_SUCCESS) {
        messageHandler.setArriveTime(micros());
        messageHandler.setLastDelay(messageHandler.getArriveTime()-messageHandler.getSendTime());
    }
    else {
        messageHandler.setArriveTime(0);
    }
  }
}









void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);


  timer = timerBegin(1000000);           	// timer 0, prescalar: 80, UP counting
  timerAttachInterrupt(timer, &onTimer); 	// Attach interrupt
  timerWrite(timer, 0);  		// Match value= 1000000 for 1 sec. delay.
  timerStart(timer);   
  timerAlarm(timer, 1000000, true, 0);



  //esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);

    // Add peer        
      if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
    return;            
    }
 
    memcpy(&peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = 0;  
    peerInfo.encrypt = false;
    if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
  }
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);  
  messageHandler.setup(modeHandler, handleLed, peerInfo);

  //esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
  WiFi.macAddress(messageHandler.myAddress);
  WiFi.macAddress(announceMessage2.address);
  //memcpy(&messageHandler.announceMessage, messageHandler.myAddress, 6);
  pinMode(AUDIO_PIN, INPUT); 
  peakDetection.begin(30, 3, 0);   
  lastClap = millis();
  modeHandler.switchMode(MODE_SEND_ANNOUNCE);
}

void loop() {


    double data = (double)analogRead(AUDIO_PIN)/512-1;  // converts the sensor value to a range between -1 and 1
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
      handleLed.flash(0, 255, 0, 200, 2, 50);
      lastClap = millis();
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
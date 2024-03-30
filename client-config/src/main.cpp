#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
//#include "ESP32TimerInterrupt.h"
#define TIMER_INTERVAL_MS       1000
#define USING_TIM_DIV1 true

#define MSG_HELLO 0
#define MSG_ANNOUNCE 1
#define MSG_TIMER_CALIBRATION 2
#define MSG_GOT_TIMER 3
#define MSG_ASK_CLAP_TIME 5
#define MSG_SEND_CLAP_TIME 6
#define MSG_ANIMATION 7
#define MSG_NOCLAPFOUND -1

#define MODE_WAIT_FOR_ANNOUNCE 0
#define MODE_WAIT_FOR_TIMER 1
#define MODE_CALIBRATE 4
#define MODE_ANIMATE 7

#define CALIBRATION_FREQUENCY 1000

int freq = 5000;
int resolution = 8;

#define LEDC_TIMER_12_BIT  12
#define LEDC_BASE_FREQ     5000
#define LEDC_START_DUTY   (0)
#define LEDC_TARGET_DUTY  (4095)
#define LEDC_FADE_TIME    (3000)

bool test = true;

/*void ARDUINO_ISR_ATTR LED_FADE_ISR()
{
  fade_ended = true;
}
*/
const int ledPinBlue1 = 20;  // 16 corresponds to GPIO16
const int ledPinRed1 = 9; // 17 corresponds to GPIO17
const int ledPinGreen1 = 3;  // 5 corresponds to GPIO5
const int ledPinGreen2 = 8;
const int ledPinRed2 = 19;
const int ledPinBlue2 = 18;
const int ledChannelRed1 = 0;
const int ledChannelGreen1 = 1;
const int ledChannelBlue1 = 2;
const int ledChannelRed2 = 3;
const int ledChannelGreen2 = 4;
const int ledChannelBlue2 = 5;
float redfloat = 0, greenfloat = 0, bluefloat = 0;

#define TIMER_ARRAY_COUNT 3

int mode = MODE_WAIT_FOR_ANNOUNCE;



void printMessage(int message) { 
  Serial.print("Message: ");
  switch (message) {
    case MSG_HELLO:
    Serial.println("MSG_HELLO");
    break;
    case MSG_ANNOUNCE:
    Serial.println("MSG_ANNOUNCE");
    break;
    case MSG_TIMER_CALIBRATION : 
    Serial.println("MSG_TIMER_CALIBRATION ");
    break;
    case MSG_GOT_TIMER : 
    Serial.println("MSG_GOT_TIMER ");
    break;
    case MSG_ANIMATION : 
    Serial.println("MSG_ANIMATION ");
    break;
    default: 
    Serial.print("Didn't recognize Message ");
    Serial.println(message);
  }
}

void printMode(int mode) { 
  Serial.print("Mode: ");
  switch (mode) {
    case MODE_WAIT_FOR_ANNOUNCE:
    Serial.println("MODE_WAIT_FOR_ANNOUNCE");
    break;
    case MODE_WAIT_FOR_TIMER:
    Serial.println("MODE_WAIT_FOR_TIMER");
    break;
    case MODE_CALIBRATE: 
    Serial.println("MODE_CALIBRATE");
    break;
    case MODE_ANIMATE:
    Serial.println("MODE_ANIMATE");
    break;
  }
}

//Network
//uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
uint8_t hostAddress[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint8_t myAddress[6];
esp_now_peer_info_t peerInfo;
esp_now_peer_num_t peerNum;

//Variables for Time Offset
struct message_timer {
  uint8_t messageType;
  uint16_t counter;
  uint32_t sendTime;
  uint16_t lastDelay;
} timerMessage;

struct message_got_timer {
  uint8_t messageType = MSG_GOT_TIMER;
  uint16_t delayAvg;
} gotTimerMessage;

struct message_announce {
  uint8_t messageType = MSG_ANNOUNCE;
  uint32_t sendTime;
  uint8_t address[6];
} announceMessage;

struct message_mode {
  uint8_t messageType;
  uint8_t mode;
} modeChange;

struct message_timer_received {
  uint8_t messageType = MSG_GOT_TIMER;
  uint8_t address[6];
  uint32_t timerOffset;
} timerReceivedMessage;

struct message_clap_time {
  uint8_t messageType = MSG_SEND_CLAP_TIME;
  int clapCounter;
  int timerCounter;
  int timeStamp;
} claps[100];

struct message_send_clap {
  uint8_t messageType = MSG_ASK_CLAP_TIME;
  uint8_t address[6];
  uint8_t clapIndex;
} sendClap ;

struct message_address {
  uint8_t messageType = MSG_HELLO;
  uint8_t address[6];
} addressMessage;

struct no_clap_found {
  uint8_t messageType = MSG_NOCLAPFOUND;
  uint8_t address[6];
  uint8_t clapIndex;
} noClapFound;

struct animate {
  uint8_t messageType = MSG_ANIMATION; 
  uint8_t animationType;
  uint8_t speed;
  uint8_t delay;
  uint8_t reps;
  uint8_t rgb1[3];
  uint8_t rgb2[3];
  uint32_t startTime;
} animationMessage;



//timer stuff
int timeOffset;
uint32_t lastTime = 0;
uint32_t msgSendTime;
uint32_t msgArriveTime;
uint32_t msgReceiveTime;
int timerCounter = 0;
int lastDelay = 0;
int oldTimerCounter = 0;
int timerArray[TIMER_ARRAY_COUNT];
int arrayCounter = 0;
int delayAvg = 0;
//general setup



//calibration stuff
int sensorValue;
int microphonePin = A0;
int clapCounter = 0;
int lastClap;
bool clapSent = false;

//address sending
int addressReceived = false;
int addressSending = 0;


//timer stuff
//ESP32Timer ITimer(0);

void printAddress(const uint8_t * mac_addr){
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print(macStr);
}


int addPeer(uint8_t * address) {
  memcpy(&peerInfo.peer_addr, address, 6);
  if (esp_now_get_peer(peerInfo.peer_addr, &peerInfo) == ESP_OK) {
    Serial.println("Found Peer");
    return 0;
  }
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
    // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return -1;
  }
  else {
    Serial.println("Added Peer");
    return 1;
  }
}


void removePeer(uint8_t address[6]) {
  if (esp_now_del_peer(address) != ESP_OK) {
    Serial.println("coudln't delete peer");
    return;
  }
}
void ledsOff() {
      ledcWrite(ledPinRed2, 0);
  ledcWrite(ledPinGreen2, 0);
  ledcWrite(ledPinBlue2, 0);
  ledcWrite(ledPinRed1, 0);
  ledcWrite(ledPinGreen1, 0);
  ledcWrite(ledPinBlue1, 0);

}

void flash(int r, int g, int b, int duration, int reps, int pause) {
  Serial.println("flashing");
for (int i = 0; i < reps; i++ ){
  ledcFade(ledPinRed1, 0, r, duration);
  ledcFade(ledPinGreen1, 0, g, duration);
  ledcFade(ledPinBlue1, 0, b, duration);
  ledcFade(ledPinRed2, 0, r, duration);
  ledcFade(ledPinGreen2, 0, g, duration);
  ledcFade(ledPinBlue2, 0, b, duration);
  delay(duration);
  ledcFade(ledPinRed1, r, 0, duration);
  ledcFade(ledPinGreen1, g, 0, duration);
  ledcFade(ledPinBlue1, b, 0, duration);
  ledcFade(ledPinRed2, r, 0, duration);
  ledcFade(ledPinGreen2, g, 0, duration);
  ledcFade(ledPinBlue2, b, 0, duration);
  delay(pause);
  ledsOff(); 
  }

}

void blink(animate animationMessage) {
  Serial.println("should blink");
  uint32_t currentTime = micros();
  uint32_t difference = currentTime-timeOffset;
  if (animationMessage.startTime-difference > 0) {
    delayMicroseconds(animationMessage.startTime-difference);
    Serial.println("should delay");
    Serial.print("Reps " );
    Serial.println(animationMessage.reps);
    Serial.print("Delay " );
    Serial.println(animationMessage.delay);
    Serial.print("Speed " );
    Serial.println(animationMessage.speed);
    Serial.print("R " );
    Serial.println(animationMessage.rgb1[0]);
    Serial.print("G " );
    Serial.println(animationMessage.rgb1[1]);    
    flash(animationMessage.rgb1[0], animationMessage.rgb1[1], animationMessage.rgb1[2], animationMessage.speed, animationMessage.reps, animationMessage.delay);
  }
  else {
    Serial.println("too late, need to wait to chime in");
    //find algorithm that calculates length of animation and finds later point in time to jump in
  }  
return;
}


void receiveTimer(int messageArriveTime) {
  //add condition that if nothing happened after 5 seconds, situation goes back to start
  //wenn die letzte message maximal 300 mikrosekunden abweicht und der letzte delay auch nicht mehr als 1500ms her war, dann muss die msg korrekt sein
  int difference = messageArriveTime - lastTime;
  lastDelay = timerMessage.lastDelay;

  if (abs(difference-CALIBRATION_FREQUENCY*1000) < 500 and abs(timerMessage.lastDelay) <2500) {
    if (arrayCounter <TIMER_ARRAY_COUNT) {
      timerArray[arrayCounter] = timerMessage.lastDelay;
    }
    else {
      for (int i = 0; i< TIMER_ARRAY_COUNT; i++) {
        delayAvg += timerArray[i];
      } 
      delayAvg = delayAvg/TIMER_ARRAY_COUNT;
      gotTimerMessage.delayAvg = delayAvg;
      timeOffset = messageArriveTime-timerMessage.sendTime-delayAvg/2;
      //TODO: Make sure the message actually arrives.
      Serial.print("sending GOT_TIMER to ");
      printAddress(hostAddress);

      esp_now_send(hostAddress, (uint8_t *) &gotTimerMessage, sizeof(gotTimerMessage));
      Serial.print("delay avg ");
      Serial.println(delayAvg);
      Serial.println("Should Flash");
      flash(255,0,0, 200, 2, 300);
    }
    //damit hab ich den zeitoffset.. 
    // if zeit - timeoffset % 1000 = 0: blink
    //timeOffset = lastTime-oldMessage.sendTime;
    //    Serial.println(esp_now_send(broadcastAddress,(uint8_t *) &timerReceivedMessage, sizeof(timerReceivedMessage) ));
    arrayCounter++;
  }
  else {
    lastTime = messageArriveTime;
  }
}



void modeSwitch(int switchMode) {
  Serial.print("Switched mode to ");
  printMode(switchMode);
  mode = switchMode;
}



void OnDataRecv(const esp_now_recv_info * mac, const uint8_t *incomingData, int len) {
  Serial.print("received ");
  printMessage(incomingData[0]);
  msgReceiveTime = micros();
  memcpy(&hostAddress, mac->src_addr, sizeof(hostAddress));
  switch (incomingData[0]) {
    case MSG_ANNOUNCE: 
    {
      if (mode == MODE_WAIT_FOR_ANNOUNCE) {
        if (addPeer(hostAddress) == 1) {
          Serial.println("Adding peer ");
          memcpy(&announceMessage, incomingData, sizeof(announceMessage));
          esp_now_send(hostAddress, (uint8_t*) &addressMessage, sizeof(addressMessage));
          //modeSwitch(MODE_WAIT_FOR_TIMER);
        }
      }
      
    }
      break;
    case MSG_TIMER_CALIBRATION:  
    { 
      addPeer(hostAddress);
      memcpy(&timerMessage,incomingData,sizeof(timerMessage));
      receiveTimer(msgReceiveTime);
    }
      break;
    case MSG_ANIMATION:
    Serial.println("animation message incoming");
    if (mode == MODE_ANIMATE) {
      Serial.println("Calling Blink");
      memcpy(&animationMessage, incomingData, sizeof(animationMessage));
      blink(animationMessage);
    }
      break;
    default: 
      Serial.println("Data type not recognized");
  }
  
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t sendStatus) {
  //turn into MODE WAIT FOR TIMER if message to host is saved.
  Serial.println("sent message to");
  printAddress(mac_addr);
  printMode(mode);
  if (mode == MODE_WAIT_FOR_ANNOUNCE) {
    if (sendStatus == ESP_NOW_SEND_SUCCESS) {
      modeSwitch(MODE_WAIT_FOR_TIMER);
    }
  }
  else if (mode == MODE_WAIT_FOR_TIMER) {
    Serial.println("should send");
    if (sendStatus == ESP_NOW_SEND_SUCCESS) {
      modeSwitch(MODE_ANIMATE);
    }
  }
    else {
      //esp_now_send(broadcastAddress,(uint8_t *) &timerReceivedMessage, sizeof(timerReceivedMessage) );
    }
}





void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  WiFi.macAddress(addressMessage.address);

  // Init ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
    // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
  modeSwitch(MODE_WAIT_FOR_ANNOUNCE);
  esp_now_register_recv_cb(OnDataRecv);  
  esp_now_register_send_cb(OnDataSent);
   WiFi.macAddress(addressMessage.address);
   esp_now_get_peer_num(&peerNum);

  Serial.print("Number of Peers start: ");
  Serial.println(peerNum.total_num);
  ledcAttach(ledPinRed1, LEDC_BASE_FREQ, LEDC_TIMER_12_BIT);
  ledcAttach(ledPinGreen1, LEDC_BASE_FREQ, LEDC_TIMER_12_BIT);
  ledcAttach(ledPinBlue1, LEDC_BASE_FREQ, LEDC_TIMER_12_BIT);
  ledcAttach(ledPinRed2, LEDC_BASE_FREQ, LEDC_TIMER_12_BIT);
  ledcAttach(ledPinGreen2, LEDC_BASE_FREQ, LEDC_TIMER_12_BIT);
  ledcAttach(ledPinBlue2, LEDC_BASE_FREQ, LEDC_TIMER_12_BIT);
  
  ledsOff();

  
  delay(1000);
  
}

void loop() {
  test = false;
  if (test == true) {
    ledcFade(ledPinRed2, 0, 255, 2000);
    delay(1000);
    ledcFade(ledPinGreen2, 0, 255, 2000);
    delay(1000);
    ledcFade(ledPinRed2, 255, 0, 2000);
    delay(1000);
    ledcFade(ledPinGreen2, 255, 0, 2000);
    delay(2000);
    ledsOff();
    ledcWrite(ledPinBlue2, 255);
    delay(1000);
    ledsOff();
    ledcWrite(ledPinRed1, 255);
    delay(1000);
    ledsOff();
    ledcWrite(ledPinGreen1, 255);
    delay(1000);
    ledsOff();
    ledcWrite(ledPinBlue1, 255);
    delay(1000);
    ledsOff();

    test = false;
  }
  //printAddress(addressMessage.address);
  Serial.print("Regular timer: ");
  printMode(mode);
  delay(2000);

} 
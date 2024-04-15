#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>

#define MSG_HELLO 0
#define MSG_ANNOUNCE 1
#define MSG_TIMER_CALIBRATION 2
#define MSG_GOT_TIMER 3
#define MSG_ASK_CLAP_TIME 5
#define MSG_SEND_CLAP_TIME 6
#define MSG_ANIMATION 7
#define MSG_NOCLAPFOUND -1

uint8_t timerReceiver[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
esp_now_peer_info_t peerInfo;

void printMessage(int message) { 
  Serial.print("Message: ");
  switch (message) {
    case MSG_HELLO:
    Serial.println("MSG_HELLO");
    break;
    case MSG_ANNOUNCE:
    Serial.println("MSG_ANNOUNCE");
    break;
    case MSG_GOT_TIMER : 
    Serial.println("MSG_GOT_TIMER ");
    break;
    case MSG_SEND_CLAP_TIME:
    Serial.println("MSG_SEND_CLAP_TIME");
    default: 
    Serial.println("Didn't recognize Message");
  }
}

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
struct message_address{
  uint8_t messageType = MSG_HELLO;
  uint8_t address[6];
} ;

struct message_clap_time {
  uint8_t messageType = MSG_SEND_CLAP_TIME;
  int clapCounter;
  uint32_t timeStamp; //offsetted.
};


struct message_animate {
  uint8_t messageType = MSG_ANIMATION; 
  uint8_t animationType;
  uint16_t speed;
  uint16_t delay;
  uint16_t reps;
  uint8_t rgb1[3];
  uint8_t rgb2[3];
  uint32_t startTime;
} ;

class messaging {
     private: 
        uint8_t broadcastAddress[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
        uint8_t emptyAddress[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        uint8_t myAddress[6];
        esp_now_peer_info_t peerInfo;
    public: 
        message_animate animationMessage;
        message_clap_time clapTime;
        message_address addressMessage;
        messaging() {
              if (esp_now_init() != ESP_OK) {
                Serial.println("Error initializing ESP-NOW");
            return;            
            }
            memcpy(&peerInfo.peer_addr, broadcastAddress, 6);
            peerInfo.channel = 0;  
            peerInfo.encrypt = false;
            WiFi.macAddress(myAddress);
        }

        void removePeer(uint8_t address[6]) {
             if (esp_now_del_peer(address) != ESP_OK) {
                Serial.println("coudln't delete peer");
                return;
            }
        }
        void printAddress(const uint8_t * mac_addr){
            char macStr[18];
            snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
                    mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
            Serial.println(macStr);
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

};
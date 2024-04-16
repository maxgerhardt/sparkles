#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <stateMachine.cpp>

#define MSG_HELLO 0
#define MSG_ANNOUNCE 1
#define MSG_TIMER_CALIBRATION 2
#define MSG_GOT_TIMER 3
#define MSG_ASK_CLAP_TIME 5
#define MSG_SEND_CLAP_TIME 6
#define MSG_ANIMATION 7
#define MSG_NOCLAPFOUND -1

#define NUM_DEVICES 20


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
  unsigned long sendTime;
  uint16_t lastDelay;
} ;


struct message_got_timer {
  uint8_t messageType = MSG_GOT_TIMER;
  uint16_t delayAvg;
};
struct message_announce {
  uint8_t messageType = MSG_ANNOUNCE;
  unsigned long sendTime;
  uint8_t address[6];
} ;
struct message_address{
  uint8_t messageType = MSG_HELLO;
  uint8_t address[6];
} ;

struct message_clap_time {
  uint8_t messageType = MSG_SEND_CLAP_TIME;
  int clapCounter;
  unsigned long timeStamp; //offsetted.
};

struct message_animate {
  uint8_t messageType = MSG_ANIMATION; 
  uint8_t animationType;
  uint16_t speed;
  uint16_t delay;
  uint16_t reps;
  uint8_t rgb1[3];
  uint8_t rgb2[3];
  unsigned long startTime;
} ;

struct client_address {
  uint8_t address[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  int id;
  float xLoc;
  float yLoc;
  float zLoc;
} ;

class messaging {
     private: 
        esp_now_peer_info_t peerInfo;
        client_address clientAddresses[NUM_DEVICES];
        int addressCounter = 0;
        modeMachine messagingModeMachine;
        unsigned long arriveTime, receiveTime, sendTime, lastDelay;
        int timerCounter = 0;

    public: 
        message_animate animationMessage;
        message_clap_time clapTime;
        message_address addressMessage;
        message_timer timerMessage;
        message_got_timer gotTimerMessage;
        message_announce announceMessage;
        uint8_t broadcastAddress[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
        uint8_t emptyAddress[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        uint8_t myAddress[6];



        
        messaging(modeMachine globalModeMachine) {
              if (esp_now_init() != ESP_OK) {
                Serial.println("Error initializing ESP-NOW");
            return;            
            }
            memcpy(&peerInfo.peer_addr, broadcastAddress, 6);
            peerInfo.channel = 0;  
            peerInfo.encrypt = false;
            WiFi.macAddress(myAddress);
            messagingModeMachine = globalModeMachine;
        };

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

        void handleAddressMessage(const esp_now_recv_info * mac) {
            for (int i = 0; i < NUM_DEVICES; i++) {
                if (memcmp(clientAddresses[i].address, emptyAddress, 6) == 0) {
                    Serial.println("need to add peer");
                    memcpy(clientAddresses[i].address, addressMessage.address, 6);
                    memcpy(&timerReceiver, mac->src_addr, 6);
                    addPeer(timerReceiver);
                    addressCounter++;
                    messagingModeMachine.switchMode(MODE_SENDING_TIMER);
                    break;
                    }
                    else if (memcmp(&clientAddresses[i].address, &addressMessage.address, 6) == true) {
                    Serial.print("found: ");
                    printAddress(addressMessage.address);
                    messagingModeMachine.switchMode(MODE_SENDING_TIMER);

                    break;
                    }
                }

        }
        void handleGotTimer() {
            removePeer(timerReceiver);
            timerCounter = 0;
            lastDelay = 0;
            messagingModeMachine.switchMode(MODE_ANIMATE);
        }
        int getLastDelay() {
            return lastDelay;
        }
        void setLastDelay(int delay) {
            lastDelay = delay;
        }

        void handleClapTime() {
            
        }
        int getTimerCounter(){
            return timerCounter;
        }
        void setTimerCounter(int counter) {
           timerCounter = counter;
        }
        void incrementTimerCounter() {
           timerCounter++;
        }
        void setSendTime(unsigned long time) {
            sendTime = time;
        }
        void setArriveTime(unsigned long time) {
            arriveTime = time;
        }
        unsigned long getSendTime() {
            return sendTime;
        }
        unsigned long getArriveTime() {
            return arriveTime;
        }



  
};
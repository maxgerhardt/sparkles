#include <Arduino.h>
#include <esp_now.h>
#include <stateMachine.h>
#include <ledHandler.h>
#ifndef MESSAGING_H
#define MESSAGING_H
#define MSG_HELLO 0
#define MSG_ANNOUNCE 1
#define MSG_TIMER_CALIBRATION 2
#define MSG_GOT_TIMER 3
#define MSG_ASK_CLAP_TIME 5
#define MSG_SEND_CLAP_TIME 6
#define MSG_ANIMATION 7
#define MSG_NOCLAPFOUND -1
#define NUM_DEVICES 20
#ifndef CALIBRATION_FREQUENCY
#define CALIBRATION_FREQUENCY 1000
#endif
#ifndef TIMER_INTERVAL_MS
#define TIMER_INTERVAL_MS 600
#endif
#define TIMER_ARRAY_COUNT 3
#define LEDC_TIMER_12_BIT  12
#define LEDC_BASE_FREQ     5000
#define LEDC_START_DUTY   (0)
#define LEDC_TARGET_DUTY  (4095)
#define LEDC_FADE_TIME    (3000)


//MESSAGE STRUCTS
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

struct message_mode {
  uint8_t messageType;
  uint8_t mode;
} ;

struct message_timer_received {
  uint8_t messageType = MSG_GOT_TIMER;
  uint8_t address[6];
  uint32_t timerOffset;
} ;
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
        client_address clientAddresses[2];
        int addressCounter = 0;
        modeMachine messagingModeHandler;
        modeMachine* globalModeHandler;
        unsigned long arriveTime, receiveTime, sendTime, lastDelay, lastTime, timeOffset;
        int timerCounter = 0;
        int timerArray[TIMER_ARRAY_COUNT];
        int arrayCounter =0;
        int delayAvg = 0;
        ledHandler* handleLed;
        esp_now_peer_info_t* peerInfo;
    public: 
        int msgSendTime;
        message_animate animationMessage;
        message_clap_time clapTime;
        message_address addressMessage;
        message_timer timerMessage;
        message_got_timer gotTimerMessage;
        message_announce announceMessage;
        message_mode modeMessage;
        message_timer_received timerReceivedMessage;
        String error_message = "";
        String message_received = "";
        String message_sent = "";
        bool gotTimer = false;
        uint8_t broadcastAddress[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
        uint8_t emptyAddress[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        uint8_t hostAddress[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        uint8_t myAddress[6];
        uint8_t timerReceiver[6] {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        messaging();
        void setup(modeMachine &modeHandler, ledHandler &globalHandleLed, esp_now_peer_info_t &globalPeerInfo);
        void blink();
        void removePeer(uint8_t address[6]);
        void printAddress(const uint8_t * mac_addr);
        int addPeer(uint8_t * address);
        void handleAddressMessage(const esp_now_recv_info * mac);
        void setHostAddress(uint8_t address[6]);
        void handleGotTimer();
        int getLastDelay();
        void setLastDelay(int delay);
        void handleClapTime(const uint8_t *incomingData);
        int getTimerCounter();
        void setTimerCounter(int counter);
        void incrementTimerCounter();
        void setSendTime(unsigned long time);
        void setArriveTime(unsigned long time);
        unsigned long getSendTime();
        unsigned long getTimeOffset();
        void setTimeOffset();
        unsigned long getArriveTime();
        void printMessage(int message);
        void receiveTimer(int messageArriveTime);
        void prepareAnnounceMessage();
        void prepareTimerMessage();
        void printBroadcastAddress();
        void printAllPeers();
        void setTimerReceiver(const uint8_t *incomingData);
        void handleAnnounce(uint8_t address[6]);
        void respondAnnounce();
        void respondTimer();
        int getMessagingMode();
        void setMessagingMode(int mode);
        void handleErrors();
        void addError(String error);
        void handleReceived();
        void handleSent();
        void addSent(String sent);
        
};



#endif
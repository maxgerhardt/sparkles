#include <myDefines.h>
#include <Arduino.h>

#include <stateMachine.h>
#include <ledHandler.h>
#include <iostream>
#include <queue>
#include <mutex>
#include <vector>
#include <cstdint>


#ifndef MESSAGING_H
#define MESSAGING_H


class messaging {
    private:  
        client_address clientAddresses[NUM_DEVICES];
        int addressCounter = 0;
        modeMachine messagingModeHandler;
        modeMachine* globalModeHandler;
        unsigned long arriveTime, receiveTime, sendTime, lastDelay, lastTime, timeOffset;
        int timerCounter = 0;
        int timerArray[TIMER_ARRAY_COUNT];
        int arrayCounter =0;
        int delayAvg = 0;
        unsigned long oldMsgReceiveTime; 
        ledHandler* handleLed;
        esp_now_peer_info_t* peerInfo;
        bool haveSentAddress = false;
      struct ReceivedData {
          const esp_now_recv_info* mac;
          const uint8_t* incomingData;
          int len;
          unsigned long msgReceiveTime;
      };
      std::queue<ReceivedData> dataQueue;
      std::mutex queueMutex;

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
        message_test testMessage;
        String error_message = "";
        String message_received = "";
        String message_sent = "";
        String messageLog = "";
        bool gotTimer = false;
        uint8_t broadcastAddress[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
        uint8_t emptyAddress[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        uint8_t hostAddress[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        uint8_t myAddress[6];
        uint8_t timerReceiver[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        uint8_t esp_test_address[6] = {0x30, 0xAE, 0xA4, 0x8D, 0xCD, 0x4C};
        messaging();
        void setup(modeMachine &modeHandler, ledHandler &globalHandleLed, esp_now_peer_info_t &globalPeerInfo);
        void blink();
        void removePeer(uint8_t address[6]);
        void printAddress(const uint8_t * mac_addr);
        int addPeer(uint8_t * address);
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
        void printAllAddresses();
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
        void pushDataToQueue(const esp_now_recv_info* mac, const uint8_t* incomingData, int len, unsigned long msgReceiveTime);
        void processDataFromQueue();
        void handleReceive(const esp_now_recv_info * mac, const uint8_t *incomingData, int len, unsigned long msgReceiveTime);
        void printMessagingMode();
        String stringAddress(const uint8_t * mac_addr);
        void printMessageModeLog();
        String getMessageLog();
        void addMessageLog(String message);
        String messageCodeToText(int message);
};




#endif
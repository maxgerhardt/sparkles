#include <myDefines.h>
#include <Arduino.h>
#include <stateMachine.h>
#if DEVICE_MODE != 2
#include <ledHandler.h>
#else
#include <webserver.h>
class webserver;
#endif
#include <iostream>
#include <queue>
#include <mutex>
#include <vector>
#include <cstdint>
#include <LittleFS.h>



#ifndef MESSAGING_H
#define MESSAGING_H


class messaging {
    private:  
        client_address clientAddresses[NUM_DEVICES];
        modeMachine* globalModeHandler;
        #if DEVICE_MODE !=2
        modeMachine messagingModeHandler;
        ledHandler* handleLed;
        struct SendData {
          const uint8_t * address;
          int messageId;
          int param =-1;
        };
        #else
        webserver* webServer;
      struct SendData {
        int messageId;
        int param;
      };
        #endif
        unsigned long arriveTime, receiveTime, sendTime, lastDelay, lastTime, timeOffset;
        int timerCounter = 0;
        int timerArray[TIMER_ARRAY_COUNT];
        int arrayCounter =0;
        int delayAvg = 0;
        unsigned long oldMsgReceiveTime; 
        esp_now_peer_info_t* peerInfo;
        bool haveSentAddress = false;
      struct ReceivedData {
          const esp_now_recv_info* mac;
          const uint8_t* incomingData;
          int len;
          unsigned long msgReceiveTime;
      };

      std::queue<ReceivedData> dataQueue;
      std::queue<SendData> sendQueue;
      std::mutex sendQueueMutex;
      std::mutex receiveQueueMutex;
      int msgCounter = 0;
    public: 
        int addressCounter = 0;
        int updatingAddress = 0;
        int msgSendTime;
        int announceTime = 0;
        message_animate animationMessage;
        message_send_clap_times sendClapTimes;
        message_address addressMessage;
        message_timer timerMessage;
        message_got_timer gotTimerMessage;
        message_announce announceMessage;
        message_switch_mode switchModeMessage;
        message_timer_received timerReceivedMessage;
        message_address_list addressListMessage;
        message_command commandMessage;
        message_ask_clap_times askClapTimesMessage;
        message_status_update statusUpdateMessage;
        message_send_clap_times webserverClapTimes;
        message_distance distanceMessage;
        String error_message = "";
        String message_received = "";
        String message_sent = "";
        String messageLog = "";
        bool gotTimer = false;
        bool receivedAnnounce = false;
        uint8_t broadcastAddress[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
        uint8_t emptyAddress[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        uint8_t hostAddress[6] = {0x68, 0xb6, 0xb3, 0x08, 0xe9, 0xae}; 
        uint8_t clientAddress[6] = {0x68, 0xb6, 0xb3, 0x08, 0xbd, 0x8a};
        uint8_t myAddress[6];
        uint8_t timerReceiver[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        int clapsReceived = 0;
        int clapsAsked = 1;
        int timersUpdated =1;
        int timerUpdateCounter = 1; 
        unsigned long lastTry = 0;
        //esp8266
        //uint8_t webserverAddress[6] = {0xe8, 0xdb, 0x84, 0x99, 0x5e, 0x44};
        uint8_t webserverAddress[6] = {0x80, 0x65, 0x99, 0xc7, 0xc2, 0x3c};
        messaging();
        #if DEVICE_MODE != 2
        void setup(modeMachine &modeHandler, ledHandler &globalHandleLed, esp_now_peer_info_t &globalPeerInfo);
        #else
        void setup(webserver &Webserver, modeMachine &modeHandler, esp_now_peer_info_t &globalPeerInfo);
        #endif
        void blink();
        void removePeer(uint8_t address[6]);
        void printAddress(const uint8_t * mac_addr);
        int addPeer(uint8_t * address);
        void setHostAddress(uint8_t address[6]);
        void handleGotTimer(const uint8_t *incomingData, uint8_t * address);
        int getLastDelay();
        void setLastDelay(int delay);
        void handleClapTimes(const uint8_t *incomingData);
        void calculateDistances(int id);
        void addClap(unsigned long timeStamp);
        void getClapTimes(int i);
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
        void stringAllAddresses();
        void setTimerReceiver(const uint8_t *incomingData);
        void announceAddress();
        void respondTimer();
        int getMessagingMode();
        void setMessagingMode(int mode);
        void handleErrors();
        void addError(String error);
        void handleReceived();
        void handleSent();
        void addSent(String sent);
        void pushDataToReceivedQueue(const esp_now_recv_info* mac, const uint8_t* incomingData, int len, unsigned long msgReceiveTime);
        void processDataFromReceivedQueue();
        void pushDataToSendQueue(const uint8_t * address, int messageId, int param);
        void pushDataToSendQueue(int messageId, int param);
        void processDataFromSendQueue();
        void handleReceive(const esp_now_recv_info * mac, const uint8_t *incomingData, int len, unsigned long msgReceiveTime);
        void printMessagingMode();
        String stringAddress(const uint8_t * mac_addr);
        void printMessageModeLog();
        String getMessageLog();
        void addMessageLog(String message);
        String messageCodeToText(int message);
        void sendAddressList();
        void prepareSendAddress(int i);
        void updateAddressToWebserver(const uint8_t * address);
        int getAddressId(const uint8_t * address);
        void filterClaps(int index);
        void writeStructsToFile(const client_address* data, int count, const char* filename);
        bool readStructsFromFile(client_address* data, int count, const char* filename);
        void updateTimers(int addressId);
        void goToSleep(unsigned long sleepTime);
        void handleTimerUpdates();
        void globalHandleTimerUpdates();
        void setNoSuccess();


};




#endif
#include <Arduino.h>

#include <esp_now.h>
#include <WiFi.h>
#include <messaging.h>



messaging::messaging() {
};

void messaging::setup(modeMachine &modeHandler, ledHandler &globalHandleLed, esp_now_peer_info_t &globalPeerInfo) {
    WiFi.macAddress(myAddress);
    handleLed = &globalHandleLed;
    globalModeHandler = &modeHandler;
    peerInfo = &globalPeerInfo;
    if (DEVICE_MODE == 0) {
        addPeer(webserverAddress);
        WiFi.macAddress(clientAddresses[0].address);
        clientAddresses[0].xLoc = 0;
        clientAddresses[0].yLoc = 0;
        clientAddresses[0].zLoc = 0;
        clientAddresses[0].timerOffset = 0;
        clientAddresses[0].delay = 0;
        addressCounter++;
    }
}

void messaging::removePeer(uint8_t address[6]) {
        Serial.println("REMOVING PEER");
        printAddress(address);
        if (esp_now_del_peer(address) != ESP_OK) {
        Serial.println("coudln't delete peer");
        return;
    }
}
void messaging::printAddress(const uint8_t * mac_addr){
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
            mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    Serial.println(macStr);
}

String messaging::stringAddress(const uint8_t * mac_addr){
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
            mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    return(String(macStr));
}


void messaging::printBroadcastAddress(){
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
            broadcastAddress[0], broadcastAddress[1], broadcastAddress[2], broadcastAddress[3], broadcastAddress[4], broadcastAddress[5]);
    Serial.println("blubl");
    Serial.println(macStr);
}



void messaging::blink() {
    //handleLed->flash(0, 0, 255, 200, 2, 50);
}
int messaging::getLastDelay() {
    return lastDelay;
}
void messaging::setLastDelay(int delay) {
    lastDelay = delay;
}
 
void messaging::calculateDistances() {
    //go through all client devices
    for (int i = 1;i < NUM_DEVICES;i++) {
        int cumul = 0;
        int clapCount = 0;
        //go through all claps on the client device
        for (int j = 0; j<clientAddresses[i].clapTimes.clapCounter; j++) {
            //if the client device's clap from the counter isn't around the master's device one:
            // this is until i have the clapping board ready
            int clapId = -1;
            unsigned long timeStampDifference = (clientAddresses[0].clapTimes.timeStamp[j] > clientAddresses[i].clapTimes.timeStamp[j]) ?
                                                (clientAddresses[0].clapTimes.timeStamp[j] - clientAddresses[i].clapTimes.timeStamp[j]) :
                                                (clientAddresses[i].clapTimes.timeStamp[j] - clientAddresses[0].clapTimes.timeStamp[j]);
            if (timeStampDifference > 1000) {
                for (int k = 0; k < clientAddresses[0].clapTimes.clapCounter; k++) {
                    unsigned long timeStampDifference2 = (clientAddresses[0].clapTimes.timeStamp[k] > clientAddresses[i].clapTimes.timeStamp[j]) ?
                                     (clientAddresses[0].clapTimes.timeStamp[k] - clientAddresses[i].clapTimes.timeStamp[j]) :
                                     (clientAddresses[i].clapTimes.timeStamp[j] - clientAddresses[0].clapTimes.timeStamp[k]);

                    if(timeStampDifference2 < 1000) {
                        clapId = k;
                    }
                }
            }
            else {
                clapId = j;
            }
            if (clapId != -1) {
                clapCount++;
                unsigned long timeStampDifference3 = (clientAddresses[0].clapTimes.timeStamp[clapId] > clientAddresses[i].clapTimes.timeStamp[j]) ?
                                     (clientAddresses[0].clapTimes.timeStamp[clapId] - clientAddresses[i].clapTimes.timeStamp[j]) :
                                     (clientAddresses[i].clapTimes.timeStamp[j] - clientAddresses[0].clapTimes.timeStamp[clapId]);

                cumul += timeStampDifference3;
            }
        }
        if( cumul > 0) {
            clientAddresses[i].distance = (float)((float)cumul/clapCount);
        }
        else {
            clientAddresses[i].distance = 0;
        }
    }
}


void messaging::addClap(int clapCounter, unsigned long timeStamp) {
    sendClapTimes.clapCounter = clapCounter;
    sendClapTimes.timeStamp[clapCounter] = timeStamp;
}
unsigned long messaging::getTimeOffset() {
    return timeOffset;
}
int messaging::getTimerCounter(){
    return timerCounter;
}
void messaging::setTimerCounter(int counter) {
    timerCounter = counter;
}
void messaging::incrementTimerCounter() {
    timerCounter++;
}
void messaging::setSendTime(unsigned long time) {
    sendTime = time;
}
void messaging::setArriveTime(unsigned long time) {
    arriveTime = time;
}
unsigned long messaging::getSendTime() {
    return sendTime;
}
unsigned long messaging::getArriveTime() {
    return arriveTime;
}
void messaging::setHostAddress(uint8_t address[6]) {
    memcpy (&hostAddress, address, 6);
}

void messaging::printMessage(int message) { 
    Serial.println(messageCodeToText(message));
}
String messaging::messageCodeToText(int message) {
    String out = "";
    out += "message ";
    switch (message) {
        case MSG_ADDRESS:
            out = "MSG_ADDRESS";
            break;
        case MSG_ANNOUNCE:
            out = "MSG_ANNOUNCE";
            break;
        case MSG_TIMER_CALIBRATION:
            out = "MSG_TIMER_CALIBRATION";
            break;
        case MSG_GOT_TIMER:
            out = "MSG_GOT_TIMER";
            break;
        case MSG_ASK_CLAP_TIMES:
            out = "MSG_ASK_CLAP_TIMES";
            break;
        case MSG_SEND_CLAP_TIMES:
            out = "MSG_SEND_CLAP_TIMES";
            break;
        case MSG_ANIMATION:
            out = "MSG_ANIMATION";
            break;
        case MSG_SWITCH_MODE:
            out = "MSG_SWITCH_MODE";
            break;
        case MSG_NOCLAPFOUND:
            out = "MSG_NOCLAPFOUND";
            break;
        case MSG_COMMANDS:
            out = "MSG_COMMANDS";
            break;
        case MSG_ADDRESS_LIST:
            out = "MSG_ADDRESS_LIST";
            break;
        case MSG_STATUS_UPDATE:
            out = "MSG_STATUS_UPDATE";
            break;
        case CMD_MSG_SEND_ADDRESS_LIST:
            out = "CMD_MSG_SEND_ADDRESS_LIST";
            break;
        case CMD_START_CALIBRATION_MODE:
            out = "CMD_START_CALIBRATION_MODE";
            break;
        case CMD_END_CALIBRATION_MODE:
            out = "CMD_END_CALIBRATION_MODE";
            break;
        case CMD_BLINK:
            out = "CMD_BLINK";
            break;
    default: 
        out += "Didn't recognize Message";
        out += message;
        break;       
    }
    return out;
}
String messaging::getMessageLog() {
    String returnLog = messageLog;
    messageLog = "";
    return returnLog;
}
void messaging::addMessageLog(String message){
    messageLog +=message;
}



void messaging::printAllPeers() {
       // Get the peer list
    esp_now_peer_info_t peerList;
    Serial.println("Peer List:");
    for (int i = 0;i<2;i++) {
        if (i == 0) {
            esp_now_fetch_peer(true, &peerList);
        }
        else {
            esp_now_fetch_peer(true, &peerList);
        }
        Serial.print("Peer ");
        Serial.print(": MAC Address=");
        for (int j = 0; j < 6; ++j) {
            Serial.print(peerList.peer_addr[j], HEX);
            if (j < 5) {
                Serial.print(":");
            }
        }
        Serial.print(", Channel=");
        Serial.print(peerList.channel);
        Serial.println();

    }
      
}
void messaging::printAllAddresses() {
    for (int i=1;i<addressCounter; i++) {
        printAddress(clientAddresses[i].address);
    }
}


int messaging::getMessagingMode() {
    return messagingModeHandler.getMode();
}
void messaging::printMessagingMode() {
    messagingModeHandler.printCurrentMode();
}
void messaging::setMessagingMode(int mode) {
    messagingModeHandler.switchMode(mode);
    addError(messagingModeHandler.modeToText(mode));
}

void messaging::handleErrors() {
    if (error_message != "") {
        Serial.print("Error Message from Handler: ");
        Serial.println(error_message);
        error_message = "";
    }
}
void messaging::handleSent() {
    if (message_sent != "") {
        Serial.print("Sent from Handler: ");
        Serial.println(message_sent);
        message_sent = "";
    } 
}
void messaging::addError(String error) {
    error_message += error;

}
void messaging::addSent(String sent) {
    message_sent += sent;
    message_sent += "\n";
}




void messaging::printMessageModeLog() {
    messagingModeHandler.printLog();
}

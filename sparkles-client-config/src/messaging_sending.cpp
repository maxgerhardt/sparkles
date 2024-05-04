#include <Arduino.h>

#include <esp_now.h>
#include <WiFi.h>
#include <messaging.h>


  void messaging::prepareAnnounceMessage() {
    
    memcpy(&announceMessage.address, myAddress, 6);
    announceMessage.sendTime = sendTime;
  }
  void messaging::prepareTimerMessage() {
    timerMessage.sendTime = sendTime;
    timerMessage.counter = timerCounter;
    timerMessage.lastDelay = lastDelay;
  }
void messaging::getClapTimes(int i) {
    if (i < NUM_DEVICES) {
        pushDataToSendQueue(clientAddresses[i].address, MSG_ASK_CLAP_TIMES);
    }
}

void messaging::pushDataToSendQueue(const uint8_t * address, int messageId) {
    Serial.println(messageCodeToText(messageId));
    std::lock_guard<std::mutex> lock(sendQueueMutex); // Lock the mutex
    SendData sendData {address, messageId};
    sendQueue.push(sendData); // Push the received data into the queue
}
void messaging::processDataFromSendQueue() {
    std::lock_guard<std::mutex> lock(sendQueueMutex); // Lock the mutex
    uint8_t peerAddress[6];
    int foundPeer = 0;
    while (!sendQueue.empty()) {
        Serial.println("Processing Data from queue");
        SendData sendData = sendQueue.front(); // Get the front element
        if (memcmp(sendData.address, broadcastAddress, 6) != 0 and memcmp(sendData.address, webserverAddress, 6) != 0) {
            Serial.println("not the same");
            memcpy(peerAddress, sendData.address, 6);
            foundPeer = addPeer(peerAddress);
        }
        else {
            Serial.println("memcmp didn't work");
            printAddress(sendData.address);
            Serial.print("Should send message ");
            Serial.println(messageCodeToText(sendData.messageId));
            

        }

        // Process the received data here...
        sendQueue.pop(); // Remove the front element from the queue
        addSent(messageCodeToText(sendData.messageId));
        switch(sendData.messageId) {
            case MSG_ADDRESS:
                esp_now_send(sendData.address, (uint8_t*) &addressMessage, sizeof(addressMessage));
                break;
            case MSG_ANNOUNCE:
                esp_now_send(sendData.address, (uint8_t*) &announceMessage, sizeof(announceMessage));
                break;
            case MSG_TIMER_CALIBRATION:
                esp_now_send(sendData.address, (uint8_t*) &timerMessage, sizeof(timerMessage));
                break;
            case MSG_GOT_TIMER:
                esp_now_send(sendData.address, (uint8_t*) &gotTimerMessage, sizeof(gotTimerMessage));
                break;
            case MSG_ASK_CLAP_TIMES:
                esp_now_send(sendData.address, (uint8_t*) &askClapTimesMessage, sizeof(askClapTimesMessage));
                break;
            case MSG_SWITCH_MODE: 
                printAddress(sendData.address);
                esp_now_send(sendData.address, (uint8_t*) &switchModeMessage, sizeof(switchModeMessage));
            case MSG_SEND_CLAP_TIMES:
                esp_now_send(sendData.address, (uint8_t*) &sendClapTimes, sizeof(sendClapTimes));
                break;
            case MSG_ANIMATION:
                esp_now_send(sendData.address, (uint8_t*) &animationMessage, sizeof(animationMessage));
                break;
            case MSG_NOCLAPFOUND:
                addError("No Clap Found");
                // Handle MSG_NOCLAPFOUND message type
                break;
            case MSG_COMMANDS:
                esp_now_send(sendData.address, (uint8_t*) &commandMessage, sizeof(commandMessage));
                break;
            case MSG_ADDRESS_LIST:
                esp_now_send(sendData.address, (uint8_t*) &addressListMessage, sizeof(addressListMessage));
                break;
            default: 
                addError("Message to send: unknown");
                break;
        }
        if (memcmp(sendData.address, broadcastAddress, 6) != 0 and memcmp(sendData.address, webserverAddress, 6) != 0 and foundPeer >0) {

            removePeer(peerAddress);
        }
    }
}   

 void messaging::sendAddressList() {
    messageLog += "sending address list\n";
    for (int i = 1;i < addressCounter; i++){
        messageLog += String(i);
        messageLog += stringAddress(clientAddresses[i].address);
        memcpy(&addressListMessage.clientAddress, &clientAddresses[i], 6);
        addressListMessage.index = i;
        messageLog +="sent address";
        pushDataToSendQueue(webserverAddress, MSG_ADDRESS_LIST);
    }
}

void messaging::updateAddressToWebserver(const uint8_t * address) {
    int addressId = getAddressId(address);
    addError("Should have updated Address to Webserver");
    addressListMessage.status = globalModeHandler->getMode();
    memcpy(&addressListMessage.clientAddress, &clientAddresses[addressId], sizeof(clientAddresses[addressId]));
    pushDataToSendQueue(webserverAddress, MSG_ADDRESS_LIST);
    commandMessage.messageId = CMD_BLINK;
    commandMessage.param = 100;
    pushDataToSendQueue(addressListMessage.clientAddress.address, MSG_COMMANDS);
    }

int messaging::getAddressId(const uint8_t * address) {
    for (int i = 0; i < NUM_DEVICES; i++) {
        if (memcmp(&clientAddresses[i].address, emptyAddress, 6) == 0) {
            return -1;
        }
        else if (memcmp(&clientAddresses[i].address, address, 6) == 0){
            return i;
        }
    }
    return 0;
}





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
    Serial.println("Get clap times called" + String(i) + "\n");
    stringAllAddresses();
    if (i == -1) {
        addError("Asking Webserver for Clap Time\n");
        pushDataToSendQueue(webserverAddress, MSG_ASK_CLAP_TIMES, -1);
    }
    else if (i < NUM_DEVICES) {
        addError("asking device " + String(i) + " for clap time\n");
        addError("Address: "+stringAddress(clientAddresses[i].address)+"\n");
        pushDataToSendQueue(clientAddresses[i].address, MSG_ASK_CLAP_TIMES, -1);
    }
}

void messaging::pushDataToSendQueue(const uint8_t * address, int messageId, int param) {
    addError("Sending message "+messageCodeToText(messageId)+ "\n");
    addError("To: "+stringAddress(address)+ "\n");

    std::lock_guard<std::mutex> lock(sendQueueMutex); // Lock the mutex
    SendData sendData {address, messageId, param};
    sendQueue.push(sendData); // Push the received data into the queue
}
void messaging::processDataFromSendQueue() {
    std::lock_guard<std::mutex> lock(sendQueueMutex); // Lock the mutex
    uint8_t peerAddress[6];
    int foundPeer = 0;
    while (!sendQueue.empty()) {
        SendData sendData = sendQueue.front(); // Get the front element
        if (memcmp(sendData.address, broadcastAddress, 6) != 0 and memcmp(sendData.address, webserverAddress, 6) != 0) {
            memcpy(peerAddress, sendData.address, 6);
            foundPeer = addPeer(peerAddress);
            addError("Sent "+messageCodeToText(sendData.messageId)+" to ");
            addError(stringAddress(sendData.address)+"\n");
        }
        else {
            addError("Sent "+messageCodeToText(sendData.messageId)+" to ");
            addError(stringAddress(sendData.address)+"\n");
            

        }

        // Process the received data here...
        sendQueue.pop(); // Remove the front element from the queue
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
                addError("ASK CLAP TIMES CALLED, sending to "+stringAddress(sendData.address)+" and ask clap times message type is "+String(askClapTimesMessage.message_type)+"\n");
                esp_now_send(sendData.address, (uint8_t*) &askClapTimesMessage, sizeof(askClapTimesMessage));
                break;
            case MSG_SWITCH_MODE: 
                esp_now_send(sendData.address, (uint8_t*) &switchModeMessage, sizeof(switchModeMessage));
            case MSG_SEND_CLAP_TIMES:
                esp_now_send(sendData.address, (uint8_t*) &sendClapTimes, sizeof(sendClapTimes));
                break;
            case MSG_ANIMATION:
                esp_now_send(sendData.address, (uint8_t*) &animationMessage, sizeof(animationMessage));
                break;
            case MSG_DISTANCE:
                addError("Sending distance message\n");
                esp_now_send(sendData.address, (uint8_t*) &distanceMessage, sizeof(distanceMessage));
                break;
            case MSG_NOCLAPFOUND:
                addError("No Clap Found\n");
                // Handle MSG_NOCLAPFOUND message type
                break;
            case MSG_COMMANDS:
                esp_now_send(sendData.address, (uint8_t*) &commandMessage, sizeof(commandMessage));
                break;
            case MSG_ADDRESS_LIST:
                if (sendData.param != -1) {
                    memcpy(&addressListMessage.clientAddress, &clientAddresses[sendData.param], sizeof(client_address));
                    addressListMessage.index = sendData.param;
                }
                esp_now_send(sendData.address, (uint8_t*) &addressListMessage, sizeof(addressListMessage));
                break;
            default: 
                addError("Message to send: unknown\n");
                break;
        }
        if (memcmp(sendData.address, hostAddress, 6) != 0 and memcmp(sendData.address, broadcastAddress, 6) != 0 and memcmp(sendData.address, webserverAddress, 6) != 0 and foundPeer >0) {

            removePeer(peerAddress);
            addError("Removed Peer "+stringAddress(peerAddress)); 
        }
    }
}   

 void messaging::sendAddressList() {
    addError( "sending address list\n");
    for (int i = 1;i < addressCounter; i++){
        addError( String(i));
        addError(stringAddress(clientAddresses[i].address)+"\n");
        addError("ID: "+String(clientAddresses[i].id));
        addError("Delay"+String(clientAddresses[i].delay)+"\n");
        addError("Distance: "+String(clientAddresses[i].distance));
        memcpy(&addressListMessage.clientAddress, &clientAddresses[i], sizeof(client_address));
        addError("Distance: "+String(addressListMessage.clientAddress.distance));

        addressListMessage.index = i;
        addError("sent address\n");
        pushDataToSendQueue(webserverAddress, MSG_ADDRESS_LIST, i);
    }
}

void messaging::updateAddressToWebserver(const uint8_t * address) {
    int addressId = getAddressId(address);
    addError("Should have updated Address to Webserver\n");
    addressListMessage.status = globalModeHandler->getMode();
    memcpy(&addressListMessage.clientAddress, &clientAddresses[addressId], sizeof(clientAddresses[addressId]));
    pushDataToSendQueue(webserverAddress, MSG_ADDRESS_LIST, -1);
    commandMessage.messageId = CMD_BLINK;
    commandMessage.param = 100;
    pushDataToSendQueue(addressListMessage.clientAddress.address, MSG_COMMANDS, -1);
    }







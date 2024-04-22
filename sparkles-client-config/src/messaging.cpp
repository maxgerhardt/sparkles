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
}

void messaging::removePeer(uint8_t address[6]) {
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

void messaging::setTimerReceiver(const uint8_t * incomingData) {
    memcpy(&addressMessage,incomingData,sizeof(addressMessage));
    
    for (int i = 0; i < NUM_DEVICES; i++) {
        if (memcmp(&clientAddresses[i].address, emptyAddress, 6) == 0) {
            //printAddress(addressMessage.address);
            memcpy(&clientAddresses[i].address, addressMessage.address, 6);
            memcpy(&timerReceiver, addressMessage.address, 6);
            addPeer(timerReceiver);
            addError("should have added timerReceiver");
            addError(stringAddress(timerReceiver));
            addressCounter++;
            addMessageLog("Switching Global Mode to Mode Sending Timer\n");
            globalModeHandler->switchMode( MODE_SENDING_TIMER);
            break;
        }
        else if (memcmp(&clientAddresses[i].address, addressMessage.address, 6) == 0) {
            addMessageLog("Address was in List. Switching Global Mode to Wait Announce Responce\n");
            globalModeHandler->switchMode(MODE_SENDING_TIMER);
            addError("address was already in list");
            addError(stringAddress(timerReceiver));
            break;
        }
    }
}

int messaging::addPeer(uint8_t * address) {
    memcpy(&peerInfo->peer_addr, address, 6);
    
    if (esp_now_get_peer(peerInfo->peer_addr, peerInfo) == ESP_OK) {
        //Serial.println("Found Peer");
        return 0;
    }
    peerInfo->channel = 0;  
    peerInfo->encrypt = false;
         // Add peer        
    if (esp_now_add_peer(peerInfo) != ESP_OK){
        //Serial.println("Failed to add peer");
        return -1;
    }
    else {
        //Serial.println("Added Peer");
        return 1;
    }
    
}

void messaging::handleAddressMessage(const esp_now_recv_info * mac) {
    for (int i = 0; i < NUM_DEVICES; i++) {
        if (memcmp(clientAddresses[i].address, emptyAddress, 6) == 0) {
            Serial.println("need to add peer");
            memcpy(clientAddresses[i].address, addressMessage.address, 6);
            memcpy(&timerReceiver, mac->src_addr, 6);
            addPeer(timerReceiver);
            addressCounter++;
                addMessageLog("Switching Messaging Mode to Mode Sending Timer\n");
            globalModeHandler->switchMode(MODE_SENDING_TIMER);
            break;
            }
            else if (memcmp(&clientAddresses[i].address, &addressMessage.address, 6) == true) {
            Serial.print("found: ");
            printAddress(addressMessage.address);
            globalModeHandler->switchMode(MODE_SENDING_TIMER);

            break;
            }
        }

}
void messaging::handleGotTimer() {
    removePeer(timerReceiver);
    timerCounter = 0;
    lastDelay = 0;
    //messagingModeMachine.switchMode(MODE_ANIMATE);
}

void messaging::blink() {
    handleLed->flash(0, 0, 255, 200, 2, 50);
}
int messaging::getLastDelay() {
    return lastDelay;
}
void messaging::setLastDelay(int delay) {
    lastDelay = delay;
}

void messaging::handleClapTime(const uint8_t *incomingdata) {
    memcpy(&clapTime, incomingdata, sizeof(clapTime));
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
    case MSG_HELLO:
        out += "MSG_HELLO";
        break;
    case MSG_ANNOUNCE:
        out += "MSG_ANNOUNCE";
        break;
    case MSG_GOT_TIMER : 
        out += "MSG_GOT_TIMER";
        break;
    case MSG_TIMER_CALIBRATION : 
        out += "MSG_TIMER_CALIBRATION";
        break;
    case MSG_SEND_CLAP_TIME:
        out += "MSG_SEND_CLAP_TIME";
        break;
    case MSG_ANIMATION: 
        out += "MSG_ANIMATION";
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

void messaging::receiveTimer(int messageArriveTime) {
  //add condition that if nothing happened after 5 seconds, situation goes back to start
  //wenn die letzte message maximal 300 mikrosekunden abweicht und der letzte delay auch nicht mehr als 1500ms her war, dann muss die msg korrekt sein
  int difference = messageArriveTime - lastTime;
  lastDelay = timerMessage.lastDelay;

  if (abs(difference-CALIBRATION_FREQUENCY*TIMER_INTERVAL_MS) < 500 and abs(timerMessage.lastDelay) <2500) {
    addMessageLog("Counts. Arraycounter: ");
    addMessageLog(String(arrayCounter));
    addMessageLog("\n");

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
      if (messagingModeHandler.getMode()!=MODE_NO_SEND) {
        Serial.println("Alerta");
        messagingModeHandler.printCurrentMode();
      }
      else{
        addMessageLog("Set Got Timer true \n");
        addMessageLog("Switched Mode to Mode Respond Timer ");
        messagingModeHandler.switchMode(MODE_RESPOND_TIMER);
        
        gotTimer = true;
      }
      //handleLed->flash(255,0,0, 200, 3, 300);
    }
    //damit hab ich den zeitoffset.. 
    // if zeit - timeoffset % 1000 = 0: blink
    //timeOffset = lastTime-oldMessage.sendTime;
    //    Serial.println(esp_now_send(broadcastAddress,(uint8_t *) &timerReceivedMessage, sizeof(timerReceivedMessage) ));
    arrayCounter++;
  }
  else {
    /*error_message += "Not enough. Difference =  ";
    error_message += String(difference);
    error_message += "timerMessage last delay = ";
    error_message += String(timerMessage.lastDelay);
    */
    addMessageLog("Doesn't Count.");
    if (abs(difference-CALIBRATION_FREQUENCY*TIMER_INTERVAL_MS) >= 500) {
        addMessageLog(" Difference ");
        addMessageLog(String(abs(difference-CALIBRATION_FREQUENCY*TIMER_INTERVAL_MS)));
    }
    else if (abs(timerMessage.lastDelay) >=2500) {
    addMessageLog(" Last delay = ");
    addMessageLog(String(abs(timerMessage.lastDelay)));
    }
    addMessageLog("\n");
  }
   lastTime = messageArriveTime;
}
  void messaging::prepareAnnounceMessage() {
    
    memcpy(&announceMessage.address, myAddress, 6);
    //printAddress(announceMessage.address);
    announceMessage.sendTime = sendTime;
    //Serial.print("Size");
    
    //Serial.print("Msg ");
    //Serial.println((uint8_t *) &announceMessage);
    //Serial.println("preparing ");
    //Serial.print ("sendtime" );
    //Serial.println(sendTime);
  }
  void messaging::prepareTimerMessage() {
    timerMessage.sendTime = sendTime;
    timerMessage.counter = timerCounter;
    timerMessage.lastDelay = lastDelay;
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

void messaging::handleAnnounce(uint8_t address[6]) {
    haveSentAddress = true;
    if (gotTimer == true) {
        return;
    }
    addError("Should have handled announce \n");
    setHostAddress(address);    
    if (addPeer(hostAddress) == 1) {
        //handleLed->flash(0, 125, 125, 200, 1, 50);
    }
    else {
        addError("COULD NOT ADD PEER");
    }
    handleLed->flash(0, 0, 125, 100, 2, 50);
    addMessageLog("Switching Messaging Mode to Respond Announce\n");
    messagingModeHandler.switchMode(MODE_RESPOND_ANNOUNCE);
}
void messaging::respondAnnounce() {
    addSent(String(globalModeHandler->getMode()));
    addMessageLog("Switching Messaging Mode to Wait Announce Responce\n");
    messagingModeHandler.switchMode(MODE_WAIT_ANNOUNCE_RESPONCE);
    msgSendTime = millis();
    addMessageLog("Sent: ");
    addMessageLog(messageCodeToText((MSG_HELLO)));
    addMessageLog("\n");
    esp_now_send(hostAddress, (uint8_t*) &addressMessage, sizeof(addressMessage));
    addMessageLog("Switching Global Mode to Wait Announce Responce\n");
    globalModeHandler->switchMode(MODE_WAIT_FOR_TIMER);
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
void messaging::respondTimer() {
    addError("Got Timer ");
    addSent("Sent timerMessage");
    addError("flashRed");
    handleLed->flash(255, 125, 0, 200, 3, 50);
    addMessageLog("Switching Messaging Mode to Wait Timer Responce");
    messagingModeHandler.switchMode(MODE_WAIT_TIMER_RESPONSE);
    msgSendTime = millis();
    addMessageLog("Sent: ");
    addMessageLog(messageCodeToText((MSG_HELLO)));
    addMessageLog("\n");
    esp_now_send(hostAddress, (uint8_t *) &gotTimerMessage, sizeof(gotTimerMessage));
}
void messaging::handleReceived() {
    if (message_received != "") {
        Serial.print("Message received: ");
        Serial.println(message_received);
        message_received = "";
    }
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
    error_message += "\n";
    error_message += error;
    error_message += "\n";
}
void messaging::addSent(String sent) {
    message_sent += "\n";
    message_sent += sent;
    message_sent += "\n";
}
void messaging::pushDataToQueue(const esp_now_recv_info * mac, const uint8_t *incomingData, int len, unsigned long msgReceiveTime) {
    std::lock_guard<std::mutex> lock(queueMutex); // Lock the mutex
    dataQueue.push({mac, incomingData, len, msgReceiveTime}); // Push the received data into the queue
}
void messaging::processDataFromQueue() {
    std::lock_guard<std::mutex> lock(queueMutex); // Lock the mutex
    while (!dataQueue.empty()) {
        ReceivedData receivedData = dataQueue.front(); // Get the front element
        // Process the received data here...
        std::cout << "Processing received data with len: " << receivedData.len << std::endl;
        dataQueue.pop(); // Remove the front element from the queue
        handleReceive(receivedData.mac, receivedData.incomingData, receivedData.len, receivedData.msgReceiveTime);
    }
}   
void messaging::handleReceive(const esp_now_recv_info * mac, const uint8_t *incomingData, int len, unsigned long msgReceiveTime) {
    messageLog += "Received Message at ";
    messageLog += String(msgReceiveTime-oldMsgReceiveTime);
    messageLog += " us since last message arrived\n";
    oldMsgReceiveTime = msgReceiveTime;
    if (globalModeHandler->getMode() == MODE_WAIT_FOR_ANNOUNCE and incomingData[0] == MSG_TIMER_CALIBRATION and haveSentAddress == false) {
        Serial.println("WHY THE HELL");
    }
    switch (incomingData[0]) {
        case MSG_ANNOUNCE: 
        handleAnnounce(mac->src_addr);
        break;
    case MSG_TIMER_CALIBRATION:  
    { 
      if (gotTimer == true) {
        addError("Already got timer");
        break;
      }
      addPeer(hostAddress);
      memcpy(&timerMessage,incomingData,sizeof(timerMessage));
        //handleLed->flash(0, 0, 125 , 100, 2, 50); 
        receiveTimer(msgReceiveTime);
    }
    break;
    case MSG_ANIMATION:
    addError("Animation Message Incoming");
    if (globalModeHandler->getMode() == MODE_ANIMATE) {
      addError("Blinking");
      memcpy(&animationMessage, incomingData, sizeof(animationMessage));
    handleLed->candle(animationMessage.speed, animationMessage.reps, animationMessage.delay);
    }
    break;
    default: 
        addError("message not recognized");
        break;
    }
}
void messaging::printMessageModeLog() {
    messagingModeHandler.printLog();
}


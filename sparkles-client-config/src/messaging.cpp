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

void messaging::setTimerReceiver(const uint8_t * incomingData) {
    memcpy(&addressMessage,incomingData,sizeof(addressMessage));
    for (int i = 1; i < NUM_DEVICES; i++) {
        if (memcmp(&clientAddresses[i].address, emptyAddress, 6) == 0) {
            //printAddress(addressMessage.address);
            memcpy(&clientAddresses[i].address, addressMessage.address, 6);
            memcpy(&timerReceiver, addressMessage.address, 6);
            addPeer(timerReceiver);
            updateAddressToWebserver(timerReceiver);
            addressCounter++;
            globalModeHandler->switchMode( MODE_SENDING_TIMER);
            break;
        }
        else if (memcmp(&clientAddresses[i].address, addressMessage.address, 6) == 0) {
            globalModeHandler->switchMode(MODE_SENDING_TIMER);
            break;
        }
    }
}

int messaging::addPeer(uint8_t * address) {
    memcpy(&peerInfo->peer_addr, address, 6);
    
    if (esp_now_get_peer(peerInfo->peer_addr, peerInfo) == ESP_OK) {
        addError("found peer\n");
        //Serial.println("Found Peer");
        return 0;
    }
    peerInfo->channel = 0;  
    peerInfo->encrypt = false;
         // Add peer        
    if (esp_now_add_peer(peerInfo) != ESP_OK){
        addError("failed to add peer\n");
        return -1;
    }
    else {
        addError("added peer\n");
        Serial.println("Added Peer");
        return 1;
    }
    
}


void messaging::handleGotTimer(const uint8_t * incomingData, uint8_t * macAddress) {
    memcpy(&gotTimerMessage, incomingData, sizeof(incomingData));
    removePeer(timerReceiver);
    timerCounter = 0;
    lastDelay = 0;
    int addressId = getAddressId(macAddress);
    clientAddresses[addressId].delay=gotTimerMessage.delayAvg;
    clientAddresses[addressId].timerOffset = gotTimerMessage.timerOffset;
    updateAddressToWebserver(timerReceiver);
    globalModeHandler->switchMode(MODE_SEND_ANNOUNCE);
    //messagingModeMachine.switchMode(MODE_ANIMATE);
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
 
void messaging::getClapTimes(int i) {
    if (i < NUM_DEVICES) {
        pushDataToSendQueue(clientAddresses[i].address, MSG_ASK_CLAP_TIMES);
    }
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

void messaging::receiveTimer(int messageArriveTime) {
  //add condition that if nothing happened after 5 seconds, situation goes back to start
  //wenn die letzte message maximal 300 mikrosekunden abweicht und der letzte delay auch nicht mehr als 1500ms her war, dann muss die msg korrekt sein
  int difference = messageArriveTime - lastTime;
  lastDelay = timerMessage.lastDelay;

  if (abs(difference-CALIBRATION_FREQUENCY*TIMER_INTERVAL_MS) < 1000 and abs(timerMessage.lastDelay) <2500) {
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
      gotTimerMessage.timerOffset = timeOffset;
      pushDataToSendQueue(hostAddress, MSG_GOT_TIMER);
      gotTimer = true;
      handleLed->flash(255,0,0, 200, 3, 300);
      globalModeHandler->switchMode(MODE_GOT_TIMER);
    }
    arrayCounter++;
  }
  else {
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
    announceMessage.sendTime = sendTime;
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
void messaging::printAllAddresses() {
    for (int i=1;i<addressCounter; i++) {
        printAddress(clientAddresses[i].address);
    }
}

void messaging::handleAnnounce(uint8_t address[6]) {
    haveSentAddress = true;
    if (gotTimer == true) {
        return;
    }
    if (globalModeHandler->getMode() == MODE_WAIT_FOR_TIMER) {
        if (lastTime < millis()+2000 and lastTime != 0) {
             return;   
        }
    }
    addError("Should have handled announce \n");
    setHostAddress(address);
    int peerMsg = addPeer(hostAddress);
    if (peerMsg == 1) {
        //handleLed->flash(0, 125, 125, 200, 1, 50);
    }
    else if (peerMsg == -1) {
        addError("COULD NOT ADD PEER ");
        addError(stringAddress(address));
        addError ("--");
        addError(stringAddress(hostAddress));
        addError("\n");
    }
    
    //handleLed->flash(0, 0, 125, 100, 2, 50);
    msgSendTime = millis();
    globalModeHandler->switchMode(MODE_WAIT_FOR_TIMER);
    pushDataToSendQueue(hostAddress, MSG_ADDRESS);

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
void messaging::pushDataToReceivedQueue(const esp_now_recv_info * mac, const uint8_t *incomingData, int len, unsigned long msgReceiveTime) {
    std::lock_guard<std::mutex> lock(receiveQueueMutex); // Lock the mutex
    dataQueue.push({mac, incomingData, len, msgReceiveTime}); // Push the received data into the queue
}
void messaging::processDataFromReceivedQueue() {
    std::lock_guard<std::mutex> lock(receiveQueueMutex); // Lock the mutex
    while (!dataQueue.empty()) {
        ReceivedData receivedData = dataQueue.front(); // Get the front element
        // Process the received data here...
        std::cout << "Processing received data with len: " << receivedData.len << std::endl;
        dataQueue.pop(); // Remove the front element from the queue
    
        handleReceive(receivedData.mac, receivedData.incomingData, receivedData.len, receivedData.msgReceiveTime);
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
void messaging::handleReceive(const esp_now_recv_info * mac, const uint8_t *incomingData, int len, unsigned long msgReceiveTime) {
    if (DEVICE_MODE == 1 and incomingData[0] != MSG_ANNOUNCE) {
        if (memcmp(mac->src_addr, hostAddress, 6) !=0 ) {
            addError("received command from untrusted source\n");
            return;
        }
    }
    messageLog += "Received ";
    messageLog += messageCodeToText(incomingData[0]);
    messageLog += " at ";
    messageLog += String(msgReceiveTime-oldMsgReceiveTime);
    messageLog += " us since last message arrived\n";
    oldMsgReceiveTime = msgReceiveTime;
    if (globalModeHandler->getMode() == MODE_WAIT_FOR_ANNOUNCE and incomingData[0] == MSG_TIMER_CALIBRATION and haveSentAddress == false) {
        Serial.println("WHY THE HELL");
        if (receivedAnnounce == false) {
            Serial.println("never received announce");
        }
    }
    switch (incomingData[0]) {
        //cases for main
        case MSG_COMMANDS: 
            memcpy(&commandMessage,incomingData,sizeof(commandMessage));
            messageLog +="command:";
            messageLog += messageCodeToText(commandMessage.messageId);
            switch (commandMessage.messageId) {
                case CMD_MSG_SEND_ADDRESS_LIST: 
                if (commandMessage.param != -1) {
                    messageLog +="should update one";
                    updateAddressToWebserver(clientAddresses[commandMessage.param].address);
                }
                else {
                    messageLog += "Should update all";
                sendAddressList();
                }
                break;
                case CMD_START_CALIBRATION_MODE: 
                globalModeHandler->switchMode(MODE_CALIBRATE);
                switchModeMessage.mode = MODE_CALIBRATE;
                messageLog += "Starting calib\n";
                pushDataToSendQueue(broadcastAddress, MSG_SWITCH_MODE);
                break;
                case CMD_END_CALIBRATION_MODE: 
                globalModeHandler->switchMode(MODE_SEND_ANNOUNCE);
                switchModeMessage.mode = MODE_NEUTRAL;
                messageLog += "ending calib\n";
                pushDataToSendQueue(broadcastAddress, MSG_SWITCH_MODE);
                getClapTimes(0);
                break;
                case CMD_MODE_NEUTRAL:
                switchModeMessage.mode = MODE_NEUTRAL;
                pushDataToSendQueue(broadcastAddress, MSG_SWITCH_MODE);
                
                case CMD_BLINK: 
                handleLed->flash(255, 255, 255, commandMessage.param, 1, 50);
                break;
            }
            break;
        case MSG_ADDRESS: 
            addError("Received Address and setting timer receiver\n");
            setTimerReceiver(incomingData);
            break;
        case MSG_GOT_TIMER:
            handleGotTimer(incomingData, mac->src_addr);
            break; 
        //cases for client
        case MSG_ANNOUNCE: 
        handleAnnounce(mac->src_addr);
        break;
        case MSG_SWITCH_MODE: 
            memcpy(&switchModeMessage, incomingData, sizeof(switchModeMessage));
            if (switchModeMessage.mode == MODE_CALIBRATE) {
                handleLed->flash(0, 255, 0, 100, 1, 50);
            }
            else if (switchModeMessage.mode = MODE_NEUTRAL and globalModeHandler->getMode() == MODE_CALIBRATE) {
                handleLed->flash(0, 255, 0, 100, 3, 50);
            }
            globalModeHandler->switchMode(switchModeMessage.mode);
        break;
        case MSG_TIMER_CALIBRATION:  
        { 
        if (gotTimer == true) {
            addError("Already got timer\n");
            break;
        }
        memcpy(&timerMessage,incomingData,sizeof(timerMessage));
            //handleLed->flash(0, 0, 125 , 100, 2, 50); 
        receiveTimer(msgReceiveTime);
        }
        break;
        case MSG_ASK_CLAP_TIMES: 
        {
            pushDataToSendQueue(hostAddress, MSG_SEND_CLAP_TIMES);
        }
        break;
        case MSG_SEND_CLAP_TIMES:
        {
            int id = getAddressId(mac->src_addr);
            memcpy(&clientAddresses[id].clapTimes, incomingData, sizeof(incomingData));
            clapsReceived++;
            if (clapsReceived+1 == addressCounter) {
                calculateDistances();
                clapsReceived = 0;
                globalModeHandler->switchMode(MODE_NEUTRAL);
                sendAddressList();
            }
        }
        break;
        case MSG_ANIMATION:
        addError("Animation Message Incoming\n");
        if (globalModeHandler->getMode() == MODE_ANIMATE) {
        addError("Blinking\n");
        memcpy(&animationMessage, incomingData, sizeof(animationMessage));
        handleLed->candle(animationMessage.speed, animationMessage.reps, animationMessage.delay);
        }
        break;
        default: 
            addError("message not recognized");
            addError(messageCodeToText(incomingData[0]));
            addError("\n");

            break;
    }
}
void messaging::printMessageModeLog() {
    messagingModeHandler.printLog();
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





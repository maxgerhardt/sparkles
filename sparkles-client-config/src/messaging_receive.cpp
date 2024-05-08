#include <Arduino.h>

#include <esp_now.h>
#include <WiFi.h>
#include <messaging.h>


void messaging::setTimerReceiver(const uint8_t * incomingData) {
    memcpy(&addressMessage,incomingData,sizeof(addressMessage));
    if (memcmp(&addressMessage.address, webserverAddress, 6) == 0) {
        memcpy(&timerReceiver, addressMessage.address, 6);
        return;
    }
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


void messaging::handleGotTimer(const uint8_t * incomingData, uint8_t * macAddress) {
    memcpy(&gotTimerMessage, incomingData, sizeof(incomingData));
    if (memcmp(timerReceiver, webserverAddress, 6) != 0) {
        removePeer(timerReceiver);
        int addressId = getAddressId(macAddress);
        clientAddresses[addressId].delay=gotTimerMessage.delayAvg;
        clientAddresses[addressId].timerOffset = gotTimerMessage.timerOffset;
        updateAddressToWebserver(timerReceiver);
        globalModeHandler->switchMode(MODE_SEND_ANNOUNCE);
    }
    else {
        globalModeHandler->switchMode(MODE_NEUTRAL);
    }
    timerCounter = 0;
    lastDelay = 0;
    //messagingModeMachine.switchMode(MODE_ANIMATE);
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
    setHostAddress(address);
    int peerMsg = addPeer(hostAddress);
    if (peerMsg == 1) {
        //handleLed->flash(0, 125, 125, 200, 1, 50);
    }
    else if (peerMsg == -1) {
        //
    }
    
    //handleLed->flash(0, 0, 125, 100, 2, 50);
    msgSendTime = millis();
    globalModeHandler->switchMode(MODE_WAIT_FOR_TIMER);
    pushDataToSendQueue(hostAddress, MSG_ADDRESS);

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

void messaging::handleReceive(const esp_now_recv_info * mac, const uint8_t *incomingData, int len, unsigned long msgReceiveTime) {
    if (DEVICE_MODE == 1 and incomingData[0] != MSG_ANNOUNCE) {
        if (memcmp(mac->src_addr, hostAddress, 6) !=0 ) {
            addError("received command from untrusted source\n");
            return;
        }
    }
    if (incomingData[0] != MSG_ANNOUNCE or gotTimer == false) {
        addError("Handling Received ");
        addError(messageCodeToText(incomingData[0]));
        addError(" from ");
        addError(stringAddress(mac->src_addr));
        addError("\n");
    }
    oldMsgReceiveTime = msgReceiveTime;
    if (globalModeHandler->getMode() == MODE_WAIT_FOR_ANNOUNCE and incomingData[0] == MSG_TIMER_CALIBRATION and haveSentAddress == false) {
        Serial.println("WHY THE HELL");
        if (receivedAnnounce == false) {
            Serial.println("never received announce");
        }
    }
    if (DEVICE_MODE == 1 and gotTimer == false and (incomingData[0] != MSG_ANNOUNCE && incomingData[0] != MSG_TIMER_CALIBRATION)) {
        addError("HANDLING ANNOUNCE BUT MESSAGE WAS "+String(messageCodeToText(incomingData[0]))+"\n");
        addError("DEVICE MODE WAS "+String(DEVICE_MODE));
        addError(" got timer is "+String(gotTimer));

        handleAnnounce(mac->src_addr);
    }
    switch (incomingData[0]) {
        //cases for main
        case MSG_COMMANDS: 
            memcpy(&commandMessage,incomingData,sizeof(commandMessage));
            addError("command:");
            addError(messageCodeToText(commandMessage.messageId));
            addError("\n");
            switch (commandMessage.messageId) {
                case CMD_MSG_SEND_ADDRESS_LIST: 
                if (commandMessage.param != -1) {
                    addError("should update one\n");
                    updateAddressToWebserver(clientAddresses[commandMessage.param].address);
                }
                else {
                    addError("Should update all\n");
                sendAddressList();
                }
                break;
                case CMD_GET_TIMER: 
                    if (globalModeHandler->getMode() == MODE_SEND_ANNOUNCE or globalModeHandler->getMode()== MODE_ANIMATE) {
                        memcpy(&timerReceiver, webserverAddress, 6);
                        globalModeHandler->switchMode(MODE_SENDING_TIMER);
                    };
                break;
                case CMD_START_CALIBRATION_MODE: 
                globalModeHandler->switchMode(MODE_CALIBRATE);
                switchModeMessage.mode = MODE_CALIBRATE;
                addError("Starting calib\n");
                pushDataToSendQueue(broadcastAddress, MSG_SWITCH_MODE);
                break;
                case CMD_END_CALIBRATION_MODE: 
                globalModeHandler->switchMode(MODE_NEUTRAL);
                switchModeMessage.mode = MODE_NEUTRAL;
                addError("ending calib\n");
                pushDataToSendQueue(broadcastAddress, MSG_SWITCH_MODE);
                getClapTimes(-1);
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
            if (memcmp(mac->src_addr, webserverAddress, 6) == 0) {
                memcpy(&webserverClapTimes, incomingData, sizeof(incomingData));
                addError("Received Clap Times from Webserver\n");
                addError("Clap Times: ");
                addError(String(webserverClapTimes.clapCounter));
                addError("\n");
                getClapTimes(1);
                break;
            }
            else {
                int id = getAddressId(mac->src_addr);
                memcpy(&clientAddresses[id].clapTimes, incomingData, sizeof(incomingData));
                clapsReceived++;
                addError("Received Clap Times from Address ");
                addError(stringAddress(mac->src_addr)+"\n");
                addError("Clap Times: ");
                addError(String(clientAddresses[id].clapTimes.clapCounter));
                addError("\n");
                if (clapsReceived+1 == addressCounter) {
                    addError("Received all clap times\n");
                    calculateDistances();
                    addError("Calculated Distances\n");
                    clapsReceived = 0;
                    globalModeHandler->switchMode(MODE_NEUTRAL);
                    sendAddressList();
                }
                else {
                    clapsAsked++;
                    getClapTimes(clapsAsked);
                }
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
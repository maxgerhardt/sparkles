#include <Arduino.h>

#include <esp_now.h>
#include <WiFi.h>
#include <messaging.h>


void messaging::setTimerReceiver(const uint8_t * incomingData) {
    memcpy(&addressMessage,incomingData,sizeof(addressMessage));
    globalModeHandler->switchMode( MODE_SENDING_TIMER);

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
            handleLed->flash(0, 125, 0, 100, 2, 50);
            break;
        }
        else if (memcmp(&clientAddresses[i].address, addressMessage.address, 6) == 0) {
            memcpy(&timerReceiver, addressMessage.address, 6);
            addPeer(timerReceiver);
            break;
        }
    }
}


void messaging::handleGotTimer(const uint8_t * incomingData, uint8_t * macAddress) {
    memcpy(&gotTimerMessage, incomingData, sizeof(gotTimerMessage));
    if (memcmp(timerReceiver, webserverAddress, 6) != 0) {
        removePeer(timerReceiver);
        int addressId = getAddressId(macAddress);
        clientAddresses[addressId].delay=gotTimerMessage.delayAvg;
        clientAddresses[addressId].timerOffset = gotTimerMessage.timerOffset;
        updateAddressToWebserver(timerReceiver);
    }

    globalModeHandler->switchMode(MODE_NEUTRAL);

    timerCounter = 0;
    lastDelay = 0;
}


void messaging::announceAddress() {
    haveSentAddress = true;
    if (gotTimer == true) {
        return;
    }
    if (globalModeHandler->getMode() == MODE_WAIT_FOR_TIMER) {
        if (lastTime < millis()+2000 and lastTime != 0) {
             return;   
        }
    }
    int peerMsg = addPeer(hostAddress);
    if (peerMsg == 1) {
        //handleLed->flash(0, 125, 125, 200, 1, 50);
    }
    else if (peerMsg == -1) {
        //
        addError("COULD NOT ADD PEER");
    }
    
    //handleLed->flash(0, 0, 125, 100, 2, 50);
    announceTime = millis();
    globalModeHandler->switchMode(MODE_WAIT_FOR_TIMER);
    pushDataToSendQueue(hostAddress, MSG_ADDRESS, -1);

}


void messaging::processDataFromReceivedQueue() {
    std::vector<ReceivedData> receivedDataList; // To store data temporarily
    {
        std::lock_guard<std::mutex> lock(receiveQueueMutex); // Lock the mutex
        while (!dataQueue.empty()) {
            ReceivedData receivedData = dataQueue.front(); // Get the front element
            // Process the received data here...
            std::cout << "Processing received data with len: " << receivedData.len << std::endl;
            dataQueue.pop(); // Remove the front element from the queue
            receivedDataList.push_back(receivedData);
        }
    } // Mutex is unlocked here

    // Call handleReceive outside the mutex scope
    for (const auto& receivedData : receivedDataList) {
        handleReceive(receivedData.mac, receivedData.incomingData, receivedData.len, receivedData.msgReceiveTime);
    }
}


void messaging::handleReceive(const esp_now_recv_info * mac, const uint8_t *incomingData, int len, unsigned long msgReceiveTime) {
    if (DEVICE_MODE == 1 and incomingData[0] != MSG_ANNOUNCE and incomingData[0] != MSG_TIMER_CALIBRATION) {
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

    if (DEVICE_MODE == 1 and gotTimer == false and (incomingData[0] != MSG_TIMER_CALIBRATION)) {
        addError("HANDLING ANNOUNCE BUT MESSAGE WAS "+String(messageCodeToText(incomingData[0]))+"\n");
        addError("DEVICE MODE WAS "+String(DEVICE_MODE));
        addError(" got timer is "+String(gotTimer));
        return;
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
                    if (globalModeHandler->getMode() == MODE_SEND_ANNOUNCE or globalModeHandler->getMode()== MODE_ANIMATE or globalModeHandler->getMode()== MODE_NEUTRAL) {
                        handleLed->flash(0, 65, 65, 100, 2, 50);
                        memcpy(&timerReceiver, webserverAddress, 6);
                        globalModeHandler->switchMode(MODE_SENDING_TIMER);
                    };
                break;
                case CMD_START_CALIBRATION_MODE: 
                globalModeHandler->switchMode(MODE_CALIBRATE);
                switchModeMessage.mode = MODE_CALIBRATE;
                addError("Starting calib\n");
                pushDataToSendQueue(broadcastAddress, MSG_SWITCH_MODE, -1);
                break;
                case CMD_END_CALIBRATION_MODE: 
                globalModeHandler->switchMode(MODE_NEUTRAL);
                switchModeMessage.mode = MODE_NEUTRAL;
                addError("ending calib\n");
                pushDataToSendQueue(broadcastAddress, MSG_SWITCH_MODE, -1);
                getClapTimes(-1);
                break;
                case CMD_MODE_NEUTRAL:
                switchModeMessage.mode = MODE_NEUTRAL;
                pushDataToSendQueue(broadcastAddress, MSG_SWITCH_MODE, -1);
                case CMD_START_ANIMATION:
                globalModeHandler->switchMode(MODE_ANIMATE);
                animationMessage.delay = 1000;
                //TODO rückwärts?
                animationMessage.reps = 50;
                animationMessage.speed = 1000;
                animationMessage.startTime = micros()+3000000;
                pushDataToSendQueue(broadcastAddress, MSG_ANIMATION, -1);
                break;
                case CMD_BLINK: 
                handleLed->flash(125, 125, 55, commandMessage.param, 1, 50);
                break;
            }
            break;
        case MSG_ADDRESS: 
            addError("Received Address and setting timer receiver\n");
            if (globalModeHandler->getMode() == MODE_WAIT_FOR_TIMER) {
                break;
            }
            else {
                setTimerReceiver(incomingData);
            }
            break;
        case MSG_GOT_TIMER:
            handleGotTimer(incomingData, mac->src_addr);
            break; 
        //cases for client

        case MSG_SWITCH_MODE: 
            memcpy(&switchModeMessage, incomingData, sizeof(switchModeMessage));
            if (switchModeMessage.mode == MODE_CALIBRATE) {
                handleLed->flash(0, 125, 0, 100, 1, 50);
            }
            else if (switchModeMessage.mode = MODE_NEUTRAL and globalModeHandler->getMode() == MODE_CALIBRATE) {
                handleLed->flash(0, 125, 0, 100, 3, 50);
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
        globalModeHandler->switchMode(MODE_WAIT_FOR_TIMER);
        receiveTimer(msgReceiveTime);
        }
        break;
        case MSG_ASK_CLAP_TIMES: 
        {
            pushDataToSendQueue(hostAddress, MSG_SEND_CLAP_TIMES, -1);
        }
        break;
        case MSG_DISTANCE: 
        {
        
         memcpy(&distanceMessage, incomingData, sizeof(distanceMessage));
         handleLed->setDistance(distanceMessage.distance);
        }
        break;
        case MSG_SEND_CLAP_TIMES:
        {   Serial.println("rcvd clap times");
            Serial.print("Free Heap: ");
            size_t freeHeap = ESP.getFreeHeap();
            Serial.print(freeHeap);
            Serial.println(" bytes");
            if (memcmp(mac->src_addr, webserverAddress, 6) == 0) {
                Serial.print("a");
                memcpy(&webserverClapTimes, incomingData, len);
                Serial.println("received clap times from Webserver");
                addError("Received Clap Times from Webserver\n");
                addError("Clap Times: ");
                addError(String(webserverClapTimes.clapCounter));
                addError("\n");
                if (clapsAsked == 1) {
                    getClapTimes(clapsAsked);
                }
                break;
            }
            else {
                Serial.print("b");
                Serial.println(stringAddress(mac->src_addr));
                int id = getAddressId(mac->src_addr);
                Serial.print("c");
                if (id == -1) {
                    Serial.println("Address not found\n");
                    break;
                }
                memcpy(&clientAddresses[id].clapTimes, incomingData, len);
                Serial.print("d");
                clapsReceived++;
                Serial.println("received clap times "+String(id));
                Serial.println("numclaps "+String(clientAddresses[id].clapTimes.clapCounter));
                Serial.println("Claps Received: "+String(clapsReceived)+" address counter "+String(addressCounter)+"\n");
                addError("Claps Received: "+String(clapsReceived)+"\n");
                addError("Received Clap Times from Address ");
                addError(stringAddress(mac->src_addr)+"\n");
                addError("Clap Times: ");
                addError(String(clientAddresses[id].clapTimes.clapCounter));
                addError("\n");
                for (int j = 0; j <clientAddresses[id].clapTimes.clapCounter; j++) {
                    Serial.print("a");
                    addError("Clap: "+String(j)+": "+String(clientAddresses[id].clapTimes.timeStamp[j])+"\n");
                }
                if (clapsReceived == addressCounter-1) {
                    addError("Received all clap times\n");
                    Serial.println("done");
                    //calculateDistances();
                    addError("Calculated Distances\n");
                    clapsReceived = 0;
                    globalModeHandler->switchMode(MODE_NEUTRAL);
                    sendAddressList();
                }
                else {
                    clapsAsked++;
                    addError("Calling Get Claps times for"+String(clapsAsked));
                    Serial.println("calling get clap times ");
                    getClapTimes(id+1);
                    Serial.println("done2");
                }
            }
        }
        break;
        case MSG_ANIMATION:
        addError("Animation Message Incoming\n");
        if (globalModeHandler->getMode() == MODE_ANIMATE) {
        addError("Blinking\n");
        memcpy(&animationMessage, incomingData, sizeof(animationMessage));

        handleLed->candle(animationMessage.speed, animationMessage.reps, animationMessage.delay, animationMessage.startTime, timeOffset);
        }
        break;
        default: 
            addError("message not recognized");
            addError(messageCodeToText(incomingData[0]));
            addError("\n");

            break;
    }
}
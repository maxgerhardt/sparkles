#include "Arduino.h"

#include "esp_now.h"
#include "WiFi.h"
#include "messaging.h"


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
            clientAddresses[i].id = i;
            memcpy(&timerReceiver, addressMessage.address, 6);
            addPeer(timerReceiver);
            timerMessage.addressId = i;
            updateAddressToWebserver(timerReceiver);
            addressCounter++;
            handleLed->flash(0, 125, 0, 100, 2, 50);
            break;
        }
        else if (memcmp(&clientAddresses[i].address, addressMessage.address, 6) == 0) {
            memcpy(&timerReceiver, addressMessage.address, 6);
            timerMessage.addressId = i;
            addPeer(timerReceiver);
            break;
        }
    }
}


void messaging::handleGotTimer(const uint8_t * incomingData, uint8_t * macAddress) {
    addError("Handling Got Timer\n");
    memcpy(&gotTimerMessage, incomingData, sizeof(gotTimerMessage));
    if (memcmp(timerReceiver, webserverAddress, 6) != 0) {
        removePeer(timerReceiver);
        int addressId = getAddressId(macAddress);
        clientAddresses[addressId].delay=gotTimerMessage.delayAvg;
        clientAddresses[addressId].timerOffset = gotTimerMessage.timerOffset;
        clientAddresses[addressId].active = ACTIVE;
        updateAddressToWebserver(timerReceiver);
    }
    if (globalModeHandler->getMode() == MODE_RESET_TIMER) {
        if (timersUpdated < addressCounter) {
            timersUpdated++;
            timerUpdateCounter++;
            globalModeHandler->switchMode(MODE_NEUTRAL);
            handleTimerUpdates();
            return;
        }
    }
    globalModeHandler->switchMode(MODE_NEUTRAL);
    timersUpdated = addressCounter;
    timerCounter = 0;
    lastDelay = 0;
    addError("Writing Struct to file\n");
    writeStructsToFile(clientAddresses, NUM_DEVICES, "/clientAddress");
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

    //handleLed->flash(0, 0, 125, 100, 2, 50);
    announceTime = millis();
    pushDataToSendQueue(hostAddress, MSG_ADDRESS, -1);
}


void messaging::processDataFromReceivedQueue() {

    std::vector<ReceivedData> receivedDataList; // To store data temporarily
    {
        std::lock_guard<std::mutex> lock(receiveQueueMutex); // Lock the mutex
        while (!dataQueue.empty()) {
            ReceivedData receivedData = dataQueue.front(); // Get the front element
            // Process the received data here...
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
    if (DEVICE_MODE == CLIENT and incomingData[0] != MSG_ANNOUNCE and incomingData[0] != MSG_TIMER_CALIBRATION) {
        if (memcmp(mac->src_addr, hostAddress, 6) !=0 ) {
            forceDebug();
            addError("received command from untrusted source\n");
            return;
        }

    }
    addError("Handling Received ");
    addError(messageCodeToText(incomingData[0]));
    addError(" from ");
    addError(stringAddress(mac->src_addr));
    addError("\n");
    if (incomingData[0] == MSG_SWITCH_MODE){
        memcpy(&switchModeMessage, incomingData, sizeof(switchModeMessage));
    }
    oldMsgReceiveTime = msgReceiveTime;
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
                    commandMessage.param = CMD_GET_BATTERY_STATUS;
                    pushDataToSendQueue(clientAddresses[commandMessage.param].address, MSG_COMMANDS, -1);
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
                break;
                case CMD_START_ANIMATION:
                    globalModeHandler->switchMode(MODE_ANIMATE);
                    switchModeMessage.mode = MODE_ANIMATE;
                    pushDataToSendQueue(broadcastAddress, MSG_SWITCH_MODE, -1);
                    animationMessage.animationType = SYNC_ASYNC_BLINK;
                    handleLed->getNextAnimation(&animationMessage);
                    animationMessage.startTime = micros()+1000000;
                    animationMessage.num_devices = addressCounter -1;

                    handleLed->printAnimationMessage(animationMessage);
                    nextAnimationPing = millis()+handleLed->calculate(&animationMessage);
                    pushDataToSendQueue(broadcastAddress, MSG_ANIMATION, -1);
                break;
                case CMD_STOP_ANIMATION:
                    globalModeHandler->switchMode(MODE_NEUTRAL);
                    switchModeMessage.mode = MODE_NEUTRAL;
                    pushDataToSendQueue(broadcastAddress, MSG_SWITCH_MODE, -1);
                break;
                case CMD_BLINK: 
                handleLed->flash(125, 125, 55, commandMessage.param, 1, 50);
                break;

                case CMD_TIMER_CALIBRATION: 
                    globalModeHandler->switchMode(MODE_WAIT_FOR_TIMER);
                break;
                case CMD_DELETE_CLIENTS: 
                    for (int i = 1; i < NUM_DEVICES; i++) {
                        memset(&clientAddresses[i], 0, sizeof(clientAddresses[i]));
                    }
                    addressCounter = 0;
                    writeStructsToFile(clientAddresses, NUM_DEVICES, "/clientAddress");
                break;
                case CMD_GO_TO_SLEEP:
                    if (DEVICE_MODE == 0) {
                        int* time = getSystemTime();
                        sleepTime.hours = time[0];
                        sleepTime.minutes = time[1]+1;
                        sleepTime.seconds = 15;
                        wakeupTime.hours = time[0];
                        wakeupTime.minutes = time[1]+2;
                        //goodNight();
                    }
                    else {
                        goToSleep(commandMessage.param*1000000);
                    }
                break;
                case CMD_RESET:
                    ESP.restart();
                break;
                case CMD_RESET_SYSTEM:
                    commandMessage.messageId = CMD_RESET;
                    pushDataToSendQueue(broadcastAddress, MSG_COMMANDS, -1);
                break;
                case CMD_GET_BATTERY_STATUS:
                    setBattery();
                    pushDataToSendQueue(hostAddress, MSG_BATTERY_STATUS, -1);
                

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
            else if (switchModeMessage.mode == MODE_NEUTRAL and globalModeHandler->getMode() == MODE_CALIBRATE) {
                handleLed->flash(125, 125, 0, 100, 3, 50);
            }

            addError("Switching mode to "+String(switchModeMessage.mode)+"\n");
            globalModeHandler->switchMode(switchModeMessage.mode);
        break;
        case MSG_TIMER_CALIBRATION:  
        { 
            addError("received timer calibration message\n");
        memcpy(&timerMessage,incomingData,sizeof(timerMessage));
        if (timerMessage.reset == true and timerMessage.counter <=3) {
            addError("Timerreset = true");
            gotTimer = false;
        }
        if (gotTimer == true and timerMessage.reset == false) {
            addError("Timerreset = false");
            break;
        }
        if (timerMessage.addressId != 0) {
            handleLed->setPosition(timerMessage.addressId);

        }
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
         addError("Distance: "+String(distanceMessage.distance)+"\n");
         handleLed->setDistance(distanceMessage.distance);
        }
        break;
        case MSG_SEND_CLAP_TIMES:
        {   if (memcmp(mac->src_addr, webserverAddress, 6) == 0) {
                addError("received from Webserver\n");
                memcpy(&webserverClapTimes, incomingData, sizeof(webserverClapTimes));
                for (int j=0;j<webserverClapTimes.clapCounter;j++) {
                    addError("Clap: "+String(webserverClapTimes.timeStamp[j])+"\n");
                }
                if (clapsAsked == 1) {
                    getClapTimes(1);
                    clapsAsked++;
                }
            }
            else {
                int id = getAddressId(mac->src_addr);
                addError("id = "+String(id)+"\n");

                if (id != -1) {
                    memcpy(&clientAddresses[id].clapTimes, incomingData, sizeof(clientAddresses[id].clapTimes));
                    for (int j=0; j<clientAddresses[id].clapTimes.clapCounter; j++) {
                        addError("Clap: "+String(clientAddresses[id].clapTimes.timeStamp[j])+"\n");
                    }
                    addError("Clap Counter "+String(clientAddresses[id].clapTimes.clapCounter)+"\n");
                    addError("addresscounter -1 = "+String(addressCounter-1)+" and clapsAsked = "+String(clapsAsked)+"\n");
                    addError("Getting calculate distances for "+String(id)+"\n");
                    if (id > 0 and id<addressCounter) {
                        
                        calculateDistances(id);
                    }
                    else if (id == addressCounter) {
                    }
                    if (clapsAsked < addressCounter) {
                        getClapTimes(clapsAsked);
                        clapsAsked = id+1;
                    }
                    else {
                        writeStructsToFile(clientAddresses, NUM_DEVICES, "/clientAddress");
                        globalModeHandler->switchMode(MODE_NEUTRAL);

                    }

                }
            }
        }
        break;
        case MSG_ANIMATION:
            addError("Animation Message Incoming\n");
            if (DEVICE_MODE == MAIN and memcmp(mac->src_addr, webserverAddress, 6) == 0) {
                memcpy(&animationMessage, incomingData, sizeof(animationMessage));
                animationMessage.startTime = micros()+3000000;
                animationMessage.animationreps = 20;
                animationMessage.num_devices = addressCounter -1;
                Serial.println("Starting animation with num devices "+String(animationMessage.num_devices));
                delay(10000);
                pushDataToSendQueue(broadcastAddress, MSG_ANIMATION, -1);
                nextAnimationPing = millis()+handleLed->calculate(&animationMessage);
                Serial.println("Animation ends at "+String(millis()));
                globalModeHandler->switchMode(MODE_ANIMATE);
                endAnimation = true;
            }
            else {
                if (globalModeHandler->getMode() == MODE_ANIMATE or globalModeHandler->getMode() == MODE_NEUTRAL) {
                addError("Blinking\n");
                globalModeHandler->switchMode(MODE_ANIMATE);
                memcpy(&animationMessage, incomingData, sizeof(animationMessage));
                handleLed->setupAnimation(&animationMessage);
                nextAnimationPing = millis()+handleLed->calculate(&animationMessage);
                endAnimation = true;
                }
            }
        break;
        case MSG_SET_TIME: 
            addError("Setting Time\n");
            memcpy(&setTimeMessage, incomingData, sizeof(setTimeMessage));
            addError("Setting Time to: "+String(setTimeMessage.hours)+":"+String(setTimeMessage.minutes)+":"+String(setTimeMessage.seconds)+"\n");
            setClock(setTimeMessage.hours,setTimeMessage.minutes, setTimeMessage.seconds);
        break;
        case MSG_SET_POSITIONS: 
            addError("Setting Positions\n");
            if (DEVICE_MODE == MAIN) {
                memcpy(&setPositionsMessage, incomingData, sizeof(setPositionsMessage));
                clientAddresses[setPositionsMessage.id].xLoc = setPositionsMessage.xpos;
                clientAddresses[setPositionsMessage.id].yLoc = setPositionsMessage.ypos;
                clientAddresses[setPositionsMessage.id].zLoc = setPositionsMessage.zpos;
                writeStructsToFile(clientAddresses, NUM_DEVICES, "/clientAddress");
                pushDataToSendQueue(clientAddresses[setPositionsMessage.id].address, MSG_SET_POSITIONS, -1);
            }
            else if (DEVICE_MODE == CLIENT) {
                memcpy(&setPositionsMessage, incomingData, sizeof(setPositionsMessage));
                handleLed->setLocation(setPositionsMessage.xpos, setPositionsMessage.ypos, setPositionsMessage.zpos);
            }
        break;
        case MSG_BATTERY_STATUS: {
            memcpy(&batteryStatusMessage, incomingData, sizeof(batteryStatusMessage));
            addError("Battery Status: "+String(batteryStatusMessage.batteryStatus)+"\n");
            int id = getAddressId(mac->src_addr);
            clientAddresses[id].batteryStatus = batteryStatusMessage.batteryStatus;
            updateAddressToWebserver(mac->src_addr);
        break;    
        }
        case MSG_SET_SLEEP_WAKEUP: {
            memcpy(&setSleepWakeupMessage, incomingData, sizeof(setSleepWakeupMessage));
            addError("Setting Sleep Wakeup\n");
            if (setSleepWakeupMessage.sleep_wakeup == true) {
                addError("Setting sleep: "+String(setSleepWakeupMessage.hours)+"\n");
                sleepTime.hours = setSleepWakeupMessage.hours;
                sleepTime.minutes = setSleepWakeupMessage.minutes;
                sleepTime.seconds = setSleepWakeupMessage.seconds;
            }
            else {
                addError("Setting wakeup: "+String(setSleepWakeupMessage.hours)+"\n");
                wakeupTime.hours = setSleepWakeupMessage.hours;
                wakeupTime.minutes = setSleepWakeupMessage.minutes;
                wakeupTime.seconds = setSleepWakeupMessage.seconds;
            }
            break;
        }
        default: 
            addError("message not recognized");
            addError(messageCodeToText(incomingData[0]));
            addError("\n");

            break;
    }
}
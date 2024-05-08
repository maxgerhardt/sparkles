#include <Arduino.h>

#include <esp_now.h>
#include <WiFi.h>
#include <messaging.h>



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

void reorderTimestamps(unsigned long timestamps[], int size) {
    // Sort the timestamps
    sort(timestamps, timestamps + size);
    
    // Find the first occurrence of zero
    unsigned long* zeroPos = find(timestamps, timestamps + size, 0);

    // Move all non-zero elements to the beginning of the array
    rotate(timestamps, zeroPos, std::remove(timestamps, timestamps + size, 0));
}

void messaging::filterClaps(int index) {
    float THRESHOLD = 1000;
    for (int i = 0; i < webserverClapTimes.clapCounter; i++) {
        bool found = false;
        for (int j = 0; j < clientAddresses[index].clapTimes.clapCounter; j++) {
            if ((webserverClapTimes.timeStamp[i] > clientAddresses[index].clapTimes.timeStamp[j] ? 
                webserverClapTimes.timeStamp[i] - clientAddresses[index].clapTimes.timeStamp[j] : 
                clientAddresses[index].clapTimes.timeStamp[j] - webserverClapTimes.timeStamp[i]) <= THRESHOLD) {
                found = true;
                break;
            }
        }
        if (!found) {
            clientAddresses[index].timeStamp[i] = 0;
            clientAddresses[index].timeStamp[i]--;
        }
    }
    sort(clientAddresses[index].timeStamp, clientAddresses[index].timeStamp + NUM_CLAPS);
}


void messaging::calculateDistances() {
    //filterClaps(0);
    //go through all devices
    for (int i = 1; i < addressCounter; i++ ){
        //filterClaps(i);
        //initialize cumulative distance value and clap counter
        int cumul = 0;
        int clapCount = 0;
        //initialize bools for "this clap was found in the other device"
        bool countmeWeb = false;
        bool countmeMain = false;
        //initialize last clap index for webserver and main device so that i don't have to start the loops again
        int lastWebserverClap = 0;
        int lastMainClap = 0;
        //output some stuff
        addError("Device: "+String(i)+"\n");
        addError("Clap Counter: "+String(clientAddresses[i].clapTimes.clapCounter)+"\n");
        //if there are no claps, lets just move on
        if (clientAddresses[i].clapTimes.clapCounter == 0) { addError("No claps detected\n"); continue; }
        //iterate through the claps of the device
        for (int j = 0 ; j < clientAddresses[i].clapTimes.clapCounter; j++) {
            //iterate through the webserver/s claps
            for (int k = lastWebserverClap; k < webserverClapTimes.clapCounter; k++) {
                //calculate the difference between the claps on the webserver and the device
                unsigned long timeStampDifference = (clientAddresses[i].clapTimes.timeStamp[j] > webserverClapTimes.timeStamp[k]) ?
                                                (clientAddresses[i].clapTimes.timeStamp[j] - webserverClapTimes.timeStamp[k]) :
                                                (webserverClapTimes.timeStamp[k] - clientAddresses[i].clapTimes.timeStamp[j]);
                //if the difference is less than a second we count the clap                                                
                if (timeStampDifference < 1000) {
                    countmeWeb = true;
                    //set the index so that the next iteration starts appropriately
                    lastWebserverClap = k-1;
                    break;
                }
                //if we have iterated too far, we break the loop. the device's clap could not be found on the webserver. false positive.
                if (clientAddresses[i].clapTimes.timeStamp[j]+1000 < webserverClapTimes.timeStamp[k]) {
                    lastWebserverClap = k-1;
                    break;
                }
            }
            //do the same for the host device
            for (int k = lastMainClap; k < clientAddresses[0].clapTimes.clapCounter; k++) {
                unsigned long timeStampDifference = (clientAddresses[i].clapTimes.timeStamp[j] > clientAddresses[0].clapTimes.timeStamp[k]) ?
                                                (clientAddresses[i].clapTimes.timeStamp[j] - clientAddresses[0].clapTimes.timeStamp[k]) :
                                                (clientAddresses[0].clapTimes.timeStamp[k] - clientAddresses[i].clapTimes.timeStamp[j]);
                if (timeStampDifference < 1000) {
                    countmeMain = true;
                    lastMainClap = k-1;
                    break;
                }
                if (clientAddresses[i].clapTimes.timeStamp[j]+1000 < clientAddresses[0].clapTimes.timeStamp[k]) {
                    lastMainClap = k-1;
                    countmeMain = false;
                    break;
                }
            }
            //for all claps that were found on all three devices devices, calculate the difference and add it to the cumulative distance
            if (countmeWeb and countmeMain) {
                clapCount++;
                unsigned long timeStampDifference = (clientAddresses[0].clapTimes.timeStamp[lastMainClap] > clientAddresses[i].clapTimes.timeStamp[j]) ?
                                                (clientAddresses[0].clapTimes.timeStamp[lastMainClap] - clientAddresses[i].clapTimes.timeStamp[j]) :
                                                (clientAddresses[i].clapTimes.timeStamp[j] - clientAddresses[0].clapTimes.timeStamp[lastMainClap]);
                cumul += timeStampDifference;
            }
        }
        //make sure i don't divide by zero, then divide.
        if( cumul > 0 and clapCount != 0) {
            clientAddresses[i].distance = (float)((float)cumul/clapCount);
        }
        else {
            //if there are no claps, or the claps are too far apart, set the distance to 0
            clientAddresses[i].distance = 0;
        }
    }
}
/*
void messaging::calculateDistances() {
    //go through all devices
    for (int i = 1; i < addressCounter; i++ ){
        int cumul = 0;
        int clapCount = 0;
        addError("Device: "+String(i)+"\n");
        addError("Clap Counter: "+String(clientAddresses[i].clapTimes.clapCounter)+"\n");
        if (clientAddresses[i].clapTimes.clapCounter == 0) { continue; }
        for (int j = 0; j < webserverClapTimes.clapCounter; j++){
            int clapId = -1;
            ind clapIdW = -1
            for (int k = 0; k < clientAddresses[i].clapTimes.clapCounter; k++) {
                unsigned long timeStampDifference = (clientAddresses[i].clapTimes.timeStamp[k] > webserverClapTimes.timeStamp[j]) ?
                                                (clientAddresses[i].clapTimes.timeStamp[k] - webserverClapTimes.timeStamp[j]) :
                                                (webserverClapTimes.timeStamp[j] - clientAddresses[i].clapTimes.timeStamp[k]);
                if (timeStampDifference < 1000) {
                    clapId = k;
                    clapIdW = j;
                }
            }

            unsigned long timeStampDifference = (clientAddresses[i].clapTimes.timeStamp > webserverClapTimes.timeStamp[j]) ?
                                                (clientAddresses[i].clapTimes.timeStamp - webserverClapTimes.timeStamp[j]) :
                                                (webserverClapTimes.timeStamp[j] - clientAddresses[i].clapTimes.timeStamp);
        }
    }
}
/*
void messaging::calculateDistances() {
    //go through all client devices
    for (int i = 1;i < NUM_DEVICES;i++) {
        int cumul = 0;
        int clapCount = 0;
        //go through all claps on the client device
        addError("Device: "+String(i))+"\n");
        addError("Clap Counter: "+String(clientAddresses[i].clapTimes.clapCounter)+"\n");
        if (clientAddresses[i].clapTimes.clapCounter == 0){ continue;}
        for (int j = 0; j<clientAddresses[i].clapTimes.clapCounter; j++) {
            //if the client device's clap from the counter isn't around the master's device one:
            // this is until i have the clapping board ready
            int clapId = -1;
            unsigned long timeStampDifference = (clientAddresses[0].clapTimes.timeStamp[j] > clientAddresses[i].clapTimes.timeStamp[j]) ?
                                                (clientAddresses[0].clapTimes.timeStamp[j] - clientAddresses[i].clapTimes.timeStamp[j]) :
                                                (clientAddresses[i].clapTimes.timeStamp[j] - clientAddresses[0].clapTimes.timeStamp[j]);
            if (timeStampDifference < 1000) {
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
        if( cumul > 0 and clapCount != 0) {
            clientAddresses[i].distance = (float)((float)cumul/clapCount);
        }
        else {
            clientAddresses[i].distance = 0;
        }
        addError("Distance: ");
        addError(String(clientAddresses[i].distance));
    }
}

*/



void messaging::setHostAddress(uint8_t address[6]) {
    memcpy (&hostAddress, address, 6);
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


void messaging::printMessageModeLog() {
    messagingModeHandler.printLog();
}

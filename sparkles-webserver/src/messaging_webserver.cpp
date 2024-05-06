#include <webserver.h>
#define DEVICE_MODE 2
#include <messaging.h>
#include <helperFuncs.h>




void messaging::setup(webserver &Webserver, modeMachine &modeHandler) {
    webServer = &Webserver;
    globalModeHandler = &modeHandler;
}


void messaging::pushDataToSendQueue(int messageId, int param) {
    std::lock_guard<std::mutex> lock(sendQueueMutex);
    sendQueue.push({messageId, param}); // Push the received data into the queue
} 


void messaging::processDataFromReceivedQueue() {
    std::lock_guard<std::mutex> lock(receiveQueueMutex); // Lock the mutex
    while (!dataQueue.empty()) {
        ReceivedData receivedData = dataQueue.front(); // Get the front element

        // Process the received data here...
        dataQueue.pop(); // Remove the front element from the queue
        
        handleReceive(receivedData.mac, receivedData.incomingData, receivedData.len, receivedData.msgReceiveTime);
    }
}  

void messaging::processDataFromSendQueue() {
    std::lock_guard<std::mutex> lock(sendQueueMutex);
    while (!sendQueue.empty()) {
        SendData sendData = sendQueue.front(); // Get the front element
        // Process the received data here...
        sendQueue.pop(); // Remove the front element from the queue
        if (sendData.messageId > CMD_START and commandMessage.messageId < CMD_END) {
          commandMessage.messageId = sendData.messageId;
          commandMessage.param = sendData.param;
          Serial.print("Sending");
          Serial.print(sendData.messageId);
          Serial.print(" -- ");
          Serial.println(sendData.param);
        } 
        switch (sendData.messageId) {
          case MSG_SEND_CLAP_TIMES: 
            esp_now_send(hostAddress, (uint8_t *) &sendClapTimes, sizeof(sendClapTimes));
            break;

        }
        esp_now_send(hostAddress, (uint8_t *) &commandMessage, sizeof(commandMessage));
    }
} 


void messaging::handleReceive(const esp_now_recv_info * mac, const uint8_t *incomingData, int len, unsigned long msgReceiveTime) {
  if (incomingData[0] != MSG_ANNOUNCE) {
    Serial.print("Received ");
    Serial.print(msgCounter);
    Serial.print(" - ");
    Serial.println(messageCodeToText(incomingData[0]));
    //printAddress(mac->src_addr);
    Serial.print("aha");
  }
  msgCounter++;
  String jsonString;
  JsonDocument receivedJson;
  switch(incomingData[0]) {
    case MSG_TIMER_CALIBRATION:
        memcpy(&timerMessage,incomingData,sizeof(timerMessage));
            //handleLed->flash(0, 0, 125 , 100, 2, 50); 
        receiveTimer(msgReceiveTime);
    break;
    case MSG_ADDRESS_LIST: 
      Serial.println("received address List Msg");
      memcpy(&addressListMessage,incomingData,sizeof(addressListMessage));
      receivedJson["index"] = String(addressListMessage.index);
      receivedJson["address"] = stringAddress(addressListMessage.clientAddress.address);
      receivedJson["delay"] = String(addressListMessage.clientAddress.delay);
      receivedJson["status"] = modeToText(addressListMessage.status);
      receivedJson["distance"] = String(addressListMessage.clientAddress.distance);
      serializeJson(receivedJson, jsonString);
      Serial.println(jsonString.c_str());
      webServer->events.send(jsonString.c_str(), "new_readings", millis());
      break;
    case MSG_STATUS_UPDATE: 

      Serial.println("received status update");
      memcpy(&statusUpdateMessage, incomingData, sizeof(statusUpdateMessage));
      receivedJson["status"] = modeToText(statusUpdateMessage.mode);
      receivedJson["statusId"] = String(statusUpdateMessage.mode);
      serializeJson(receivedJson, jsonString);
      webServer->events.send(jsonString.c_str(), "new_status", millis());
      break;
    case MSG_ASK_CLAP_TIMES: 
    Serial.println(" asked for clap times");
    pushDataToSendQueue(MSG_SEND_CLAP_TIMES, 0);
    break;
  }

}


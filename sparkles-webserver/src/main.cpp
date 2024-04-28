#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <FS.h>
#include "ESPAsyncWebServer.h"
#include <ArduinoJson.h>
#include <myDefines.h>
//#include <ESPAsyncTCP.h>
#include <index_html.h>
#include <addressList.h>
#include <queue>
#include <mutex>
#include <cstdint>
#include <helperFuncs.h>
// put function declarations here:

bool timerRecvd = false;
bool msgRecvd = false;
uint8_t broadcastAddress[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t myAddress[6]= {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t hostAddress[6] = {0x68, 0xb6, 0xb3, 0x08, 0xe9, 0xae};
int sent = 0;
int msgCounter = 0;

 const char* ssid = "Spargels";
const char* password = "sparkles";
esp_now_peer_info_t peerInfo;
const char* PARAM_INPUT_1 = "input1";
#define ADDRESS_LIST 1

struct ReceivedData {
    const esp_now_recv_info * mac;
    const uint8_t* incomingData;
    int len;
};

struct SendData {
  int commandId;
  int param;
};
std::queue<ReceivedData> dataQueue;
std::queue<SendData> sendQueue;

message_command commandMessage;
message_status_update statusUpdateMessage;
message_address_list addressListMessage;

int msgType = 0;
int deviceId = -1;
String outputJson;
AsyncWebServer server(80);
AsyncEventSource events("/events");
//AsyncEventSource calibrate("/commandCalibrate");
bool calibrationStatus = false;

unsigned long lastDings = 0;

String addressToStr(const uint8_t * mac_addr){
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
            mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    return(String(macStr));
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t sendStatus) {
  Serial.println("Sent");
   if (sendStatus == 0) {
      sent = 1;
    }
    else {
     sent = -1;
    }
}

void handleReceive(const esp_now_recv_info *mac, const uint8_t *incomingData, uint8_t len) {
  if (incomingData[0] != MSG_ANNOUNCE) {
    Serial.print("Received ");
    Serial.print(msgCounter);
    Serial.print(" - ");
    Serial.println(messageCodeToText(incomingData[0]));
    printAddress(mac->src_addr);
  }
  msgCounter++;
  String jsonString;
  JsonDocument receivedJson;
  switch(incomingData[0]) {
    case MSG_ADDRESS_LIST: 
      Serial.println("received address List Msg");
      memcpy(&addressListMessage,incomingData,sizeof(addressListMessage));
      receivedJson["index"] = String(addressListMessage.index);
      receivedJson["address"] = addressToStr(addressListMessage.clientAddress.address);
      receivedJson["delay"] = String(addressListMessage.clientAddress.delay);
      receivedJson["status"] = modeToText(addressListMessage.status);
      serializeJson(receivedJson, jsonString);
      Serial.println(jsonString.c_str());
      events.send(jsonString.c_str(), "new_readings", millis());
      break;
    case MSG_STATUS_UPDATE: 

      Serial.println("received status update");
      memcpy(&statusUpdateMessage, incomingData, sizeof(statusUpdateMessage));
      receivedJson["status"] = modeToText(statusUpdateMessage.mode);
      receivedJson["statusId"] = String(statusUpdateMessage.mode);
      serializeJson(receivedJson, jsonString);
      events.send(jsonString.c_str(), "new_status", millis());
      break;
  }

}


void pushDataToReceiveQueue(const esp_now_recv_info *mac, const uint8_t *incomingData, uint8_t len) {
  if (incomingData[0] != MSG_ANNOUNCE) {
    dataQueue.push({mac, incomingData, len}); // Push the received data into the queue
    }
} 

void pushDataToSendQueue(int commandId, int param) {
    sendQueue.push({commandId, param}); // Push the received data into the queue
} 

void processDataFromSendQueue() {
    while (!sendQueue.empty()) {
        SendData sendData = sendQueue.front(); // Get the front element
        // Process the received data here...
        sendQueue.pop(); // Remove the front element from the queue
        commandMessage.messageId = sendData.commandId;
        commandMessage.param = sendData.param;
        Serial.print("Sending");
        Serial.print(sendData.commandId);
        Serial.print(" -- ");
        Serial.println(sendData.param);
        esp_now_send(hostAddress, (uint8_t *) &commandMessage, sizeof(commandMessage));
    }
} 

void processDataFromReceiveQueue() {
    while (!dataQueue.empty()) {
        ReceivedData receivedData = dataQueue.front(); // Get the front element

        // Process the received data here...
        dataQueue.pop(); // Remove the front element from the queue
        handleReceive(receivedData.mac, receivedData.incomingData, receivedData.len);
    }
}  


void  OnDataRecv(const esp_now_recv_info * mac, const uint8_t *incomingData, int len) {
  pushDataToReceiveQueue(mac, incomingData, len);
}


bool connected = false;
int count = 0;




void setup() {
    Serial.begin(115200);
if (DEVICE_USED == 2) {
    ledcAttach(ledPinRed1, LEDC_BASE_FREQ, LEDC_TIMER_12_BIT);
  ledcAttach(ledPinGreen1, LEDC_BASE_FREQ, LEDC_TIMER_12_BIT);
  ledcAttach(ledPinBlue1, LEDC_BASE_FREQ, LEDC_TIMER_12_BIT);
  ledcAttach(ledPinRed2, LEDC_BASE_FREQ, LEDC_TIMER_12_BIT);
  ledcAttach(ledPinGreen2, LEDC_BASE_FREQ, LEDC_TIMER_12_BIT);
  ledcAttach(ledPinBlue2, LEDC_BASE_FREQ, LEDC_TIMER_12_BIT);
  ledsOff();
}
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ssid, password);
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  Serial.print("Station IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Wi-Fi Channel: ");
  Serial.println(WiFi.channel());
  
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", addressList);
    msgType = ADDRESS_LIST;

  });
  
  events.onConnect([](AsyncEventSourceClient *client){

    // send event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    connected = true;
    client->send("");
  });
  
  server.on("/updateDeviceList", HTTP_GET, [] (AsyncWebServerRequest *request){

    Serial.println("Called UpdateDeviceList");
    msgType=ADDRESS_LIST;
    if (request->hasParam("id")) {
      deviceId  = request->getParam("id")->value().toInt();
    }
    else {
      deviceId = -1;
    }
    pushDataToSendQueue(CMD_MSG_SEND_ADDRESS_LIST, deviceId);
     request->send(200, "text/html", "OK");
  });
  server.on("/commandCalibrate", HTTP_GET, [] (AsyncWebServerRequest *request){
    Serial.println("Called Calibrate");
    if (calibrationStatus == false) {
    pushDataToSendQueue(CMD_START_CALIBRATION_MODE, -1);
     request->send(204);
     String jsonString;
     jsonString = "{\"status\" : \"true\"}";
     events.send(jsonString.c_str(), "calibrateStatus", millis());
     calibrationStatus = true;
    }
    else if (calibrationStatus == true) {
      pushDataToSendQueue(CMD_END_CALIBRATION_MODE, -1);
      request->send(204);
      String jsonString;
      jsonString = "{\"status\" : \"false\"}";
     events.send(jsonString.c_str(), "calibrateStatus", millis());
     calibrationStatus = false;
    }
  });

//restart calibration mode//

  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    String inputParam;
    int foo = 1;
    // GET input1 value on <ESP_IP>/get?input1=<inputMessage>
    
    if (request->hasParam(PARAM_INPUT_1)) {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();
      inputParam = PARAM_INPUT_1;
      foo = 1;


    }
  request->send(200, "text/html", addressList);
   //"HTTP GET request sent to your ESP on input field (" 
     //                                + inputParam + ") with value: " + inputMessage +
      //                               "<br><a href=\"/\">Return to Home Page</a>");
    msgType = foo;
  });

  server.addHandler(&events);
  server.begin();
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
  memcpy(peerInfo.peer_addr, hostAddress, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);
  
  WiFi.macAddress(myAddress);

  // put your setup code here, to run once:
}
void loop() {
  sent = false;
  processDataFromReceiveQueue();
  processDataFromSendQueue();
  // put your main code here, to run repeatedly:
  if (outputJson != "")  {
    Serial.print ("JSON: \n");
    Serial.println(outputJson);
    outputJson = "";
  }
    if (lastDings + 5000 < millis() )
    {
      Serial.print("Still alive ");
      Serial.println(count);
      Serial.println(WiFi.softAPIP());
      Serial.println(WiFi.channel());
      printAddress(myAddress);

      count++;
      //printAddress(myAddress);
      lastDings = millis();
    }
}


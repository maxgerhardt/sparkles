#define DEVICE_USED 4
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <espnow.h>
#include "ESPAsyncWebServer.h"
#include <ArduinoJson.h>
#include <myDefines.h>
#include <ESPAsyncTCP.h>
#include <index_html.h>
#include <addressList.h>
#include <queue>
#include <mutex>
#include <cstdint>
// put function declarations here:

bool timerRecvd = false;
bool msgRecvd = false;
uint8_t broadcastAddress[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t myAddress[6]= {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t hostAddress[6] = {0x68, 0xb6, 0xb3, 0x08, 0xe9, 0xae};
int sent = 0;
std::queue<String> dataQueue;

 const char* ssid = "Sparkles";
const char* password = "sparkles";

const char* PARAM_INPUT_1 = "input1";
const char* PARAM_INPUT_2 = "input2";
const char* PARAM_INPUT_3 = "input3";


message_command commandMessage;

int msgType = 0;
JsonDocument receivedAddress;
String outputJson;
AsyncWebServer server(80);
AsyncEventSource events("/events");

int lastDings = 0;

String addressToStr(const uint8_t * mac_addr){
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
            mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    return(String(macStr));
}

void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.println("Sent");
   if (sendStatus == 0) {
      sent = 1;
    }
    else {
     sent = -1;
    }
}
message_address_list addressListMessage;

void pushDataToQueue(String jsonString) {
    dataQueue.push(jsonString); // Push the received data into the queue
}


void processDataFromQueue() {
    while (!dataQueue.empty()) {
        String jsonString = dataQueue.front(); // Get the front element
        // Process the received data here...
        dataQueue.pop(); // Remove the front element from the queue
        delay(1000);
        events.send(jsonString.c_str(), "new_readings", millis());
    }
}  
void OnDataRecv(uint8_t *mac, uint8_t *incomingData, uint8_t len) {
  switch(incomingData[0]) {
    case MSG_ADDRESS_LIST: 
      Serial.println("received msg");
      memcpy(&addressListMessage,incomingData,sizeof(addressListMessage));
      receivedAddress["index"] = String(addressListMessage.index);
      receivedAddress["address"] = addressToStr(addressListMessage.address);
      receivedAddress["delay"] = String(addressListMessage.delay);
      String jsonString;
      serializeJson(receivedAddress, jsonString);
      pushDataToQueue(jsonString);
      events.send("hello!", NULL, millis(), 10000);
      //events.send(jsonString.c_str(), "new_readings", millis());
  }
  
}


bool connected = false;
int count = 0;


void printAddress(const uint8_t * mac_addr){
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
            mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    Serial.println(macStr);
}

void setup() {
    Serial.begin(115200);
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ssid, password);


   Serial.print("Station IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Wi-Fi Channel: ");
  Serial.println(WiFi.channel());
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });
  events.onConnect([](AsyncEventSourceClient *client){

    // send event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    connected = true;
    client->send("hello!", NULL, millis(), 10000);
  });
  
  server.on("/updateDeviceList", HTTP_GET, [] (AsyncWebServerRequest *request){
    msgType=1;
     request->send(200, "text/html", "OK");
  });



  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    String inputParam;
    int foo;
    // GET input1 value on <ESP_IP>/get?input1=<inputMessage>
    if (request->hasParam(PARAM_INPUT_1)) {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();
      inputParam = PARAM_INPUT_1;
      foo = 1;


    }
    // GET input2 value on <ESP_IP>/get?input2=<inputMessage>
    else if (request->hasParam(PARAM_INPUT_2)) {
      inputMessage = request->getParam(PARAM_INPUT_2)->value();
      inputParam = PARAM_INPUT_2;
      foo = 2;
    }
    // GET input3 value on <ESP_IP>/get?input3=<inputMessage>
    else if (request->hasParam(PARAM_INPUT_3)) {
      inputMessage = request->getParam(PARAM_INPUT_3)->value();
      inputParam = PARAM_INPUT_3;
      foo = 3;
    }
    else {
      inputMessage = "No message sent";
      inputParam = "none";
      foo = 0;
    }
   request->send(200, "text/html", addressList);
   //"HTTP GET request sent to your ESP on input field (" 
     //                                + inputParam + ") with value: " + inputMessage +
      //                               "<br><a href=\"/\">Return to Home Page</a>");
    msgType = foo;
  });

  server.addHandler(&events);
  server.begin();
  esp_now_add_peer(hostAddress, ESP_NOW_ROLE_COMBO, 0, NULL, 0);
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);
  WiFi.macAddress(myAddress);

  // put your setup code here, to run once:
}

void loop() {
  sent = false;
  processDataFromQueue();
  //Serial.print("sent:");
  /*
  if (sent == 1) {
    Serial.println(" true");
  }
  else if (sent == -1) {
    Serial.println(" false");
  }
  else if (sent == 0) {
    Serial.println (" not called");
  }
  sent = 0;
  if (lastDings + 5000 < millis() )
  {
    Serial.println("Still alive");
    printAddress(myAddress);
  }
  */
  // put your main code here, to run repeatedly:
  if (outputJson != "")  {
    Serial.print ("JSON: \n");
    Serial.println(outputJson);
    outputJson = "";
  }
  if (msgType != 0) {
    commandMessage.messageId = CMD_MSG_SEND_ADDRESS_LIST;
    Serial.println("sending command");
     esp_now_send(hostAddress, (uint8_t *) &commandMessage, sizeof(commandMessage));
    msgType = 0;
  }
    if (lastDings + 5000 < millis() )
    {
      Serial.print("Still alive ");
      Serial.println(count);
      count++;
      //printAddress(myAddress);
      lastDings = millis();
    }
}


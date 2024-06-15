#include <webserver.h>
#include <myDefines.h>



webserver::webserver(FS* fs) : server(80), events("/events"), filesystem(fs) {
    // Constructor body. Any additional setup code can go here.
}


void webserver::setup(messaging &Messaging, modeMachine &modeHandler) {
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(ssid, password);  
    configRoutes(); 
    messageHandler = &Messaging;
    server.addHandler(&events);
    server.begin();
    stateMachine = &modeHandler;
}






void webserver::configRoutes() {
  events.onConnect([this](AsyncEventSourceClient *client){
    // send event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    this->handleClientConnect(client);
  });
    server.onNotFound([this](AsyncWebServerRequest *request) {
        this->serveStaticFile(request);
    });
     server.on("/updateDeviceList", HTTP_GET, [this] (AsyncWebServerRequest *request){
        this->updateDeviceList(request);
    });
    server.on("/commandCalibrate", HTTP_GET, [this] (AsyncWebServerRequest *request){
      this->commandCalibrate(request);
    });
    server.on("/commandAnimate", HTTP_GET, [this] (AsyncWebServerRequest *request){
      this->commandAnimate(request);
    });

}

void webserver::handleClientConnect(AsyncEventSourceClient *client) {
    connected = true;
    client->send("");
}

void webserver::updateDeviceList(AsyncWebServerRequest *request) {
    messageHandler->addError("Called UpdateDeviceList");
    msgType = ADDRESS_LIST;

    messageHandler->pushDataToSendQueue(CMD_MSG_SEND_ADDRESS_LIST, request->hasParam("id") ? request->getParam("id")->value().toInt() : -1);
     request->send(200, "text/html", "OK");

}

void webserver::commandCalibrate(AsyncWebServerRequest *request) {
      messageHandler->addError("Called Calibrate");
      if (stateMachine->getMode() == MODE_WAIT_FOR_TIMER) {
        request->send(400);
        return;
      }
      if (stateMachine->getMode() != MODE_CALIBRATE && stateMachine->getMode() != MODE_WAIT_FOR_TIMER) {
        messageHandler->pushDataToSendQueue(CMD_GET_TIMER, 0);
        request->send(204);
        String jsonString;
        jsonString = "{\"status\" : \"true\"}";
        events.send(jsonString.c_str(), "calibrateStatus", millis()); 
        stateMachine->switchMode(MODE_WAIT_FOR_TIMER);
        messageHandler->addError("starting timer mode");

      }
      else if (stateMachine->getMode() == MODE_CALIBRATE) {
        messageHandler->pushDataToSendQueue(CMD_END_CALIBRATION_MODE, -1);
        Serial.println("ENDING CALIBRATION MODE");
        request->send(204);
        String jsonString;
        jsonString = "{\"status\" : \"false\"}";
        events.send(jsonString.c_str(), "calibrateStatus", millis());
        stateMachine->switchMode(MODE_NEUTRAL);
        messageHandler->addError ("Ending calibration. Claps: ");
        messageHandler->addError(String(messageHandler->sendClapTimes.clapCounter));
      }
      
    
}

void webserver::commandAnimate(AsyncWebServerRequest *request) {
  messageHandler->addError("Called Animate");
  String jsonString;

  if (stateMachine->getMode() == MODE_WAIT_FOR_TIMER || stateMachine->getMode() == MODE_CALIBRATE) {
    request->send(400);
    return;
  }
  else if (stateMachine->getMode() == MODE_NEUTRAL) {
    request->send(204);
    jsonString = "{\"status\" : \"true\"}";
    events.send(jsonString.c_str(), "animateStatus", millis()); 
    messageHandler->pushDataToSendQueue(CMD_START_ANIMATION, -1);
    stateMachine->switchMode(MODE_ANIMATE);
  }
  else if (stateMachine->getMode() == MODE_ANIMATE) {
    request->send(204);
        jsonString = "{\"status\" : \"false\"}";
    events.send(jsonString.c_str(), "animateStatus", millis()); 
    messageHandler->pushDataToSendQueue(CMD_STOP_ANIMATION, -1);
    stateMachine->switchMode(MODE_NEUTRAL);
  }
  }

void webserver::serveStaticFile(AsyncWebServerRequest *request) {
  // Get the file path from the request
  String path = request->url();

  // Check if the file exists
  if (path == "/" || path == "/index.html") { // Modify this condition as needed
    path = "/addressList.html"; // Adjust the file path here
  }
  Serial.print("asked for static file");
  Serial.println(path);
  // Check if the file exists
  if (LittleFS.exists(path)) {
      // Open the file for reading
      File file = LittleFS.open(path, "r");
      if (file) {
        // Read the contents of the file into a String
        String fileContent;
        while (file.available()) {
          fileContent += char(file.read());
        }

        // Close the file
        file.close();

        // Send the file content as response
        request->send(200, "text/html", fileContent);
        return;
      }
  }

  // If file not found, send 404
  request->send(404, "text/plain", "File not found");
}

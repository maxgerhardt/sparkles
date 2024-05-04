#include <webserver.h>
#include <myDefines.h>

webserver::webserver(FS* fs) : server(80), events("/events"), filesystem(fs) {
    // Constructor body. Any additional setup code can go here.
}

void webserver::setup(){
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(ssid, password);  
    configRoutes(); 
}


void webserver::configRoutes() {
  events.onConnect([this](AsyncEventSourceClient *client){
    // send event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    this->handleClientConnect(client)
  });
    server.onNotFound([this](AsyncWebServerRequest *request) {
        this->serveStaticFile(request);
    });
     server.on("/updateDeviceList", HTTP_GET, [this] (AsyncWebServerRequest *request){
        this->updateDeviceList(request);
    });
}

void webserver::handleClientConnect(AsyncEventSourceClient *client) {
    connected = true;
    client->send("");
}

void webserver::updateDeviceList(AsyncWebServerRequest *request) {
    Serial.println("Called UpdateDeviceList");
    msgType=ADDRESS_LIST;

    pushDataToSendQueue(CMD_MSG_SEND_ADDRESS_LIST, request->hasParam("id") ? request->getParam("id")->value().toInt() : -1);
     request->send(200, "text/html", "OK");

}
void webserver::serveStaticFile(AsyncWebServerRequest *request) {
  // Get the file path from the request
  String path = request->url();

  // Check if the file exists
  if (path == "/" || path == "/index.html") { // Modify this condition as needed
    path = "/addressList.html"; // Adjust the file path here
  }

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

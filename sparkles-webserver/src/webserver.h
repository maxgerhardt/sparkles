#ifndef WEBSERV
#define WEBSERV
#include <Arduino.h>
#include <WiFi.h>
#include <FS.h>
#include "ESPAsyncWebServer.h"
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <myDefines.h>
#include <queue>
#include <mutex>
#include <cstdint>
//#include <helperFuncs.h>
#include <messaging.h>
#include <stateMachine.h>

class webserver {
    private:
        const char* ssid = "Spargels";
        const char* password = "sparkles";
        int msgType;
        String outputJson;
        bool calibrationStatus = false;
        bool connected = false;

    public:
    webserver(FS* fs);
    void setup(messaging &Messaging, modeMachine &modeHandler);
    void serveStaticFile(AsyncWebServerRequest *request);
    AsyncWebServer server;
    AsyncEventSource events;
    FS* filesystem;
    messaging* messageHandler;
    modeMachine* stateMachine;
    void configRoutes();
    void handleClientConnect(AsyncEventSourceClient *client);
    void updateDeviceList(AsyncWebServerRequest *request);
    void commandCalibrate(AsyncWebServerRequest *request);
    void commandAnimate(AsyncWebServerRequest *request);
};
#endif

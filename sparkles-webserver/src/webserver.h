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
#include <helperFuncs.h>
#endif

class webserver {
    private:
        const char* ssid = "Spargels";
        const char* password = "sparkles";
        int msgType;
        String outputJson;
        bool calibrationStatus = false;
        bool connected = false;
    public:
    webserver(FS* fs) :  server(80), events("/events"), filesystem(fs) {}
    void setup();
    void serveStaticFile(AsyncWebServerRequest *request);
    AsyncWebServer server;
    AsyncEventSource events;
    FS* filesystem;
    void configRoutes();
    void handleClientConnect(AsyncEventSourceClient *client);
    void updateDeviceList(AsyncWebServerRequest *request)
}


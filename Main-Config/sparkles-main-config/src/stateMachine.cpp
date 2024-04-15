#include <Arduino.h>
#include <stateMachine.h>



class modeMachine {
    private: 
        int currentMode = MODE_INIT;

    public:
        modeMachine() {

        };
    void switchMode(int mode) {
        currentMode = mode;
    }
    int getMode() {
        return currentMode;
    }
    void printMode(int mode) { 
    Serial.print("Mode: ");
    switch (mode) {
        case MODE_SEND_ANNOUNCE:
        Serial.println("MODE_SEND_ANNOUNCE");
        break;
        case MODE_SENDING_TIMER:
        Serial.println("MODE_SENDING_TIMER");
        break;
        case MODE_CALIBRATE: 
        Serial.println("MODE_CALIBRATE");
        break;
        case MODE_ANIMATE:
        Serial.println("MODE_ANIMATE");
        break;
    }
    }


};
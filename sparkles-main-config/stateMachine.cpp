#include <Arduino.h>
#include <../../sparkles-main-config/src/stateMachine.h>

 
modeMachine::modeMachine() {

}
void modeMachine::switchMode(int mode) {
    currentMode = mode;
}
int modeMachine::getMode() {
    return currentMode;
}
void modeMachine::printMode(int mode) { 
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


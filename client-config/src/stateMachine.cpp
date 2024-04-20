#include <Arduino.h>
#include <stateMachine.h>

 
modeMachine::modeMachine() {

}
void modeMachine::switchMode(int mode) {
    Serial.print("Switched Mode to ");
    printMode(mode);
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
        case MODE_WAIT_FOR_ANNOUNCE:
        Serial.println("MODE_WAIT_FOR_ANNOUNCE");
        break;
        case MODE_WAIT_FOR_TIMER:
        Serial.println("MODE_WAIT_FOR_TIMER");
        case MODE_CALIBRATE: 
        Serial.println("MODE_CALIBRATE");
        break;
        case MODE_ANIMATE:
        Serial.println("MODE_ANIMATE");
        break;
        case MODE_NO_SEND: 
        Serial.println("MODE_NO_SEND");
        break;
        case MODE_RESPOND_ANNOUNCE:
        Serial.println("MODE_RESPOND_ANNOUNCE");
        break;
        case MODE_RESPOND_TIMER:
        Serial.println("MODE_RESPOND_TIMER");
        break;
        default: 
        Serial.print("Mode unknown ");
        Serial.println(mode);
        break;
    }   
}
void modeMachine::printCurrentMode() {
    printMode(currentMode);
}


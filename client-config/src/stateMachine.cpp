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
    Serial.println(modeToText(mode));   
}
String modeMachine::modeToText(int mode) {
    String out ;
    out = "Mode: ";
    switch (mode) {
        case MODE_SEND_ANNOUNCE:
        out += "MODE_SEND_ANNOUNCE";
        break;
    case MODE_SENDING_TIMER:
        out += "MODE_SENDING_TIMER";
        break;
    case MODE_WAIT_FOR_ANNOUNCE:
        out += "MODE_WAIT_FOR_ANNOUNCE";
        break;
    case MODE_WAIT_FOR_TIMER:
        out += "MODE_WAIT_FOR_TIMER";
        break;
    case MODE_CALIBRATE: 
        out += "MODE_CALIBRATE";
        break;
    case MODE_ANIMATE:
        out += "MODE_ANIMATE";
        break;
    case MODE_NO_SEND: 
        out += "MODE_NO_SEND";
        break;
    case MODE_RESPOND_ANNOUNCE:
        out += "MODE_RESPOND_ANNOUNCE";
        break;
    case MODE_RESPOND_TIMER:
        out += "MODE_RESPOND_TIMER";
        break;
    default: 
        out += "Mode unknown ";
        out += String(mode); // Convert 'mode' integer to String and concatenate
        break;
    }   
    return out;
}
void modeMachine::printCurrentMode() {
    printMode(currentMode);
}


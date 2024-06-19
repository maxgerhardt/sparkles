
#include <helperFuncs.h>
/*
void ledsOff() {
      ledcWrite(ledPinRed2, 0);
  ledcWrite(ledPinGreen2, 0);
  ledcWrite(ledPinBlue2, 0);
  ledcWrite(ledPinRed1, 0);
  ledcWrite(ledPinGreen1, 0);
  ledcWrite(ledPinBlue1, 0);

}*/


String modeToText(int mode) {
    String out ;
    out = "Mode: ";
    switch (mode) {
        case MODE_INIT:
        out += "INIT";
        break;
        case MODE_SEND_ANNOUNCE:
        out += "SEND_ANNOUNCE";
        break;
    case MODE_SENDING_TIMER:
        out += "SENDING_TIMER";
        break;
    case MODE_WAIT_FOR_TIMER:
        out += "WAIT_FOR_TIMER";
        break;
    case MODE_CALIBRATE: 
        out += "CALIBRATE";
        break;
    case MODE_ANIMATE:
        out += "ANIMATE";
        break;
    case MODE_NO_SEND: 
        out += "NO_SEND";
        break;
    case MODE_RESPOND_ANNOUNCE:
        out += "RESPOND_ANNOUNCE";
        break;
    case MODE_RESPOND_TIMER:
        out += "RESPOND_TIMER";
        break;
    case MODE_WAIT_TIMER_RESPONSE: 
        out += "WAIT_TIMER_RESPONSE";  
    break;      
    case MODE_WAIT_ANNOUNCE_RESPONCE:
        out += "WAIT_ANNOUNCE_RESPONCE";
        break;
    case MODE_NEUTRAL:
        out += "NEUTRAL";
        break;
    default: 
        out += "Mode unknown ";
        out += String(mode); // Convert 'mode' integer to String and concatenate
        break;
    }   
    return out;
}


String messageCodeToText(int message) {
    String out = "";
    out += "message ";
    switch (message) {
        case MSG_ADDRESS:
            out = "MSG_ADDRESS";
            break;
        case MSG_ANNOUNCE:
            out = "MSG_ANNOUNCE";
            break;
        case MSG_TIMER_CALIBRATION:
            out = "MSG_TIMER_CALIBRATION";
            break;
        case MSG_GOT_TIMER:
            out = "MSG_GOT_TIMER";
            break;
        case MSG_ASK_CLAP_TIMES:
            out = "MSG_ASK_CLAP_TIMES";
            break;
        case MSG_SEND_CLAP_TIMES:
            out = "MSG_SEND_CLAP_TIMES";
            break;
        case MSG_ANIMATION:
            out = "MSG_ANIMATION";
            break;
        case MSG_SWITCH_MODE:
            out = "MSG_SWITCH_MODE";
            break;
        case MSG_NOCLAPFOUND:
            out = "MSG_NOCLAPFOUND";
            break;
        case MSG_COMMANDS:
            out = "MSG_COMMANDS";
            break;
        case MSG_ADDRESS_LIST:
            out = "MSG_ADDRESS_LIST";
            break;
        case MSG_STATUS_UPDATE:
            out = "MSG_STATUS_UPDATE";
            break;
        case CMD_MSG_SEND_ADDRESS_LIST:
            out = "CMD_MSG_SEND_ADDRESS_LIST";
            break;
        case CMD_START_CALIBRATION_MODE:
            out = "CMD_START_CALIBRATION_MODE";
            break;
        case CMD_END_CALIBRATION_MODE:
            out = "CMD_END_CALIBRATION_MODE";
            break;
        case CMD_BLINK:
            out = "CMD_BLINK";
            break;
    default: 
        out += "Didn't recognize Message";
        out += message;
        break;       
    }
    return out;
}

void printAddress(const uint8_t * mac_addr){
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
            mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    Serial.println(macStr);
}
String addressToStr(const uint8_t * mac_addr){
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
            mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    return(String(macStr));
}


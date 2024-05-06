#include <myDefines.h>
#include <Arduino.h>
#ifndef HELPERFUNC
#define HELPERFUNC


void ledsOff();

String modeToText(int mode);
String messageCodeToText(int message);
void printAddress(const uint8_t * mac_addr);
String addressToStr(const uint8_t * mac_addr);
#endif


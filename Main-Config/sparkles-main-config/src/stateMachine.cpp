#define MODE_INIT -1
#define MODE_SEND_ANNOUNCE 0
#define MODE_SENDING_TIMER 1
#define MODE_CALIBRATE 4
#define MODE_ANIMATE 7
#include <Arduino.h>



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


};
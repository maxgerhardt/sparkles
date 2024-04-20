#ifndef MODE_MACHINE_H
#define MODE_MACHINE_H
#define MODE_INIT -1
#define MODE_SEND_ANNOUNCE 0
#define MODE_SENDING_TIMER 1
#define MODE_WAIT_FOR_ANNOUNCE 2
#define MODE_WAIT_FOR_TIMER 3

#define MODE_CALIBRATE 4
#define MODE_ANIMATE 7


#define MODE_NO_SEND 90
#define MODE_RESPOND_ANNOUNCE 91
#define MODE_RESPOND_TIMER 92
#define MODE_WAIT_TIMER_RESPONSE 93
#define MODE_WAIT_ANNOUNCE_RESPONCE 94

    class modeMachine {
        private: 
            int currentMode = MODE_INIT;
        public:
            modeMachine();
            void switchMode(int mode);
            int getMode();
            void printMode(int mode);
            String modeToText(int mode);
            void printCurrentMode();
    };
#endif
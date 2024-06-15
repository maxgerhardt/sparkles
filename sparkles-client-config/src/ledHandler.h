#include <myDefines.h>
#ifndef LED_HANDLER_H
#define LED_HANDLER_H
//#include <stateMachine.h>

//#include <../../sparkles-main-config/src/messaging.h>
#include <queue>



class ledHandler {
    private: 
        float rgb[3];
        float redfloat = 0, greenfloat = 0, bluefloat = 0;
        float redsteps, greensteps, bluesteps;
        concentric_animation concentricAnimation;
        float distance;
        animationEnum currentAnimation;      
        message_animate animationMessage;
        int animationRun = 0;
        int repeatCounter = 0;
        int animationNextStep = 0;
        int cycleStart = 0;
        unsigned long timerOffset = 0;
        unsigned long repeatRuntime = 0;
    public:
    ledHandler();
    void setup();
    void setupAnimation(const message_animate animationSetupMessage);
    void run();
    float fract(float x);
    float mix(float a, float b, float t);
    float step(float e, float x);
    float* hsv2rgb(float h, float s, float b, float* rgb);
    void ledsOff();
    void flash(int r = 255, int g = 0, int b = 0, int duration = 50, int reps = 2, int pause = 50);
    void blink();
    void candle(int duration, int reps, int pause, unsigned long startTime, unsigned long timeOffset);
    void setupSyncAsyncBlink();
    void syncAsyncBlink();
    void setTimerOffset(unsigned long setOffset);

    void ledOn(int r, int g, int b, int duration, bool half);
    void concentric();
    void setDistance(float dist);
    void writeLeds();
    float calculateFlash(int targetVal, unsigned long timeElapsed);
};

#endif
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
        using MessageQueue = std::queue<std::function<void()>>;
        MessageQueue queue;
    public:
    ledHandler();
    void setup();
    float fract(float x);
    float mix(float a, float b, float t);
    float step(float e, float x);
    float* hsv2rgb(float h, float s, float b, float* rgb);
    void ledsOff();
    void flash(int r = 255, int g = 0, int b = 0, int duration = 50, int reps = 2, int pause = 50);
    void blink();
    void candle(int duration, int reps, int pause);
    void addToQueue(std::function<void()> func);
    void processQueue() ;
    void ledOn(int r, int g, int b, int duration, bool half);
};

#endif
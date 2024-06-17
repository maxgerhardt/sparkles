#include <Arduino.h>
#include <ledHandler.h>
#ifndef DEVICE
#define V1 1
#define V2 2 
#define D1 3
#define DEVICE V2
#endif 


ledHandler::ledHandler() {

};

void ledHandler::setup() {
  ledcAttach(ledPinRed1, LEDC_BASE_FREQ, LEDC_TIMER_12_BIT);
  ledcAttach(ledPinGreen1, LEDC_BASE_FREQ, LEDC_TIMER_12_BIT);
  ledcAttach(ledPinBlue1, LEDC_BASE_FREQ, LEDC_TIMER_12_BIT);
  ledcAttach(ledPinRed2, LEDC_BASE_FREQ, LEDC_TIMER_12_BIT);
  ledcAttach(ledPinGreen2, LEDC_BASE_FREQ, LEDC_TIMER_12_BIT);
  ledcAttach(ledPinBlue2, LEDC_BASE_FREQ, LEDC_TIMER_12_BIT);
  ledsOff();
}


/*
void ledHandler::setAnimation(int animationType) {
  switch (animationType) {
    case 1:
      concentric();
      break;
    case 2:
      blink();
      break;
    case 3:
      candle(1000, 5, 100, 0, 0);
      break;
    case 4:
      flash(255, 0, 0, 50, 2, 50);
      break;
    case 5:
      ledOn(255, 0, 0, 1000, false);
      break;
    case 6:
      ledOn(255, 0, 0, 1000, true);
      break;
    default:
      break;
  }
}
*/
float ledHandler::fract(float x) { return x - int(x); }

float ledHandler::mix(float a, float b, float t) { return a + (b - a) * t; }

float ledHandler::step(float e, float x) { return x < e ? 0.0 : 1.0; }

float* ledHandler::hsv2rgb(float h, float s, float b, float* rgb) {
  rgb[0] = b * mix(1.0, constrain(abs(fract(h + 1.0) * 6.0 - 3.0) - 1.0, 0.0, 1.0), s);
  Serial.println(rgb[0]); 
  rgb[1] = b * mix(1.0, constrain(abs(fract(h + 0.6666666) * 6.0 - 3.0) - 1.0, 0.0, 1.0), s);
  rgb[2] = b * mix(1.0, constrain(abs(fract(h + 0.3333333) * 6.0 - 3.0) - 1.0, 0.0, 1.0), s);
  return rgb;
}
void ledHandler::ledsOff() {
      ledcWrite(ledPinRed2, 0);
  ledcWrite(ledPinGreen2, 0);
  ledcWrite(ledPinBlue2, 0);
  ledcWrite(ledPinRed1, 0);
  ledcWrite(ledPinGreen1, 0);
  ledcWrite(ledPinBlue1, 0);

}
void ledHandler::flash(int r, int g, int b, int duration, int reps, int pause) {
  if (DEVICE == D1) {
    return;
  }
  for (int i = 0; i < reps; i++ ){
    ledcFade(ledPinRed1, 0, r, duration);
    ledcFade(ledPinGreen1, 0, g, duration);
    ledcFade(ledPinBlue1, 0, b, duration);
    ledcFade(ledPinRed2, 0, r, duration);
    ledcFade(ledPinGreen2, 0, g, duration);
    ledcFade(ledPinBlue2, 0, b, duration);
    delay(duration);
    ledcFade(ledPinRed1, r, 0, duration);
    ledcFade(ledPinGreen1, g, 0, duration);
    ledcFade(ledPinBlue1, b, 0, duration);
    ledcFade(ledPinRed2, r, 0, duration);
    ledcFade(ledPinGreen2, g, 0, duration);
    ledcFade(ledPinBlue2, b, 0, duration);
    delay(pause);
    ledsOff(); 
  }
}  

void ledHandler::ledOn(int r, int g, int b, int duration, int  frontback) {
  if (DEVICE == D1) {
    return;
  }
  if (frontback == 0 or frontback == 2) {
  ledcWrite(ledPinRed1, r);
  ledcWrite(ledPinGreen1, g);
  ledcWrite(ledPinBlue1, b);
    
  }
  if (frontback == 1 or frontback == 2) {
  ledcWrite(ledPinRed2, r);
  ledcWrite(ledPinGreen2, g);
  ledcWrite(ledPinBlue2, b);
  }
    delay(duration);
}    

/*
void ledHandler::delayLoop(int duration) {
  int start = millis();
  while (true) {
    if (millis()-start > duration) {
      break;
    }
  }
}
*/


void ledHandler::setupAnimation(message_animate *animationSetupMessage) {
  memcpy(&animationMessage, animationSetupMessage, sizeof(animationMessage));
  Serial.println("setupanimation");
  if (currentAnimation != OFF) {
    Serial.println("isn't off");
    return;
  }
  if (animationMessage.animationType == OFF) {
    Serial.println("turn off");
    ledsOff();
    return;
  }
  else if (animationMessage.animationType == SYNC_ASYNC_BLINK) {
    Serial.println("Setting up anim");
    setupSyncAsyncBlink();
  }
  else {
    return;
  }
 
}

void ledHandler::setupSyncAsyncBlink() {
  repeatCounter = 0;
  localAnimationStart = 0;
  animationNextStep = 0;
  currentAnimation = animationMessage.animationType;
  unsigned long localAnimationStartMicros = animationMessage.startTime+timerOffset;
  if (micros() > localAnimationStartMicros) {
    Serial.println("not today");
    return;
  }
  //calculate start of first round
  unsigned long microdiff = localAnimationStartMicros - micros();
  localAnimationStart = millis()+microdiff/1000;
  Serial.println("blinkStart "+String(localAnimationStart));
  animationNextStep = localAnimationStart;
  globalAnimationTimeframe = animationMessage.speed+animationMessage.pause;

  //first round of cycle is just this. speed+pause
  localAnimationTimeframe = globalAnimationTimeframe;
  /* calculate length of the entire animation. not needed for now.
  for (int i = 0; i < animationMessage.reps/2;i++) {
    cycleTotalRuntime += 2*((animationMessage.spread_time/(animationMessage.reps/2))*animationMessage.num_devices*i+animationMessage.speed+animationMessage.pause);
  }*/
}

void ledHandler::run() {
  switch (currentAnimation) {
    case OFF:
      ledsOff();
      break;
    case SYNC_ASYNC_BLINK:
      syncAsyncBlink();
      break;
    default:
      break;
  }
}

void ledHandler::syncAsyncBlink() {

  //wait until next step. if all repeats done: done. 
  if (millis() < animationNextStep) {
    return;
  }
  //if a repeat should happen...
  if (millis()  >= globalAnimationStart + globalAnimationTimeframe) {
    repeatCounter++;
    globalAnimationStart = globalAnimationStart+globalAnimationTimeframe;

    Serial.println("Repeatcounter++ "+String(repeatCounter));
    //cycle start noch timen
    //and figure out the start of next cycle
    if (repeatCounter <= animationMessage.reps/2) {
      localAnimationStart = globalAnimationStart + (animationMessage.spread_time/(animationMessage.reps/2))*position*repeatCounter;
    }
    else {
      localAnimationStart = globalAnimationStart + (animationMessage.spread_time/(animationMessage.reps/2))*position*(animationMessage.reps-repeatCounter);
    }
  }

  
  //if all repetitions have happened
  if (repeatCounter == animationMessage.reps) {
    if (animationRepeatCounter == animationMessage.animationreps) {
      //either turn off
      ledsOff();
      currentAnimation = OFF;
      return;
    }
    else {
      // or repeat 
      animationRepeatCounter++;
      repeatCounter = 0;
    }
  }
  //hier kommt die tatsächliche animation rein
  if (millis() > animationNextStep and millis() < localAnimationStart+animationMessage.speed) {
    //redsteps? and backwards?
    //cyclestart berechnen auch abhängig vom spread und ansonsten einfach runterrattern dat ding
    int elapsedTime = millis()-localAnimationStart;
    redfloat  = calculateFlash(animationMessage.rgb1[0], elapsedTime);
    greenfloat = calculateFlash(animationMessage.rgb1[1], elapsedTime);
    bluefloat = calculateFlash(animationMessage.rgb1[2], elapsedTime);  
    writeLeds();
    // how to calculate?
    animationNextStep = millis()+animationMessage.speed/256;
  }

}


float ledHandler::calculateFlash(int targetVal, unsigned long timeElapsed){
  if (timeElapsed < 0) {
    timeElapsed = 0;
  }  else if (timeElapsed > animationMessage.speed) {
    timeElapsed = animationMessage.speed;
  }
  float normalizedTime = (float)timeElapsed/(float)animationMessage.speed;
  float factor;  
  float colorValue;
  if (normalizedTime <= 0.5) {
    factor = pow(normalizedTime * 2, animationMessage.exponent);
    colorValue = (targetVal * factor);
  }
  else {
    factor = pow(abs(normalizedTime - 1) *2, animationMessage.exponent);
    colorValue = (targetVal*factor);
  }

  if (colorValue < 0) {
    return 0;
  }
  else if (colorValue > 255) {
    return 255;
  }
  else {
    return colorValue;
  }

}

void ledHandler::writeLeds() {
  ledcWrite(ledPinRed1, (int)floor(redfloat));
  ledcWrite(ledPinGreen1, (int)floor(greenfloat));
  ledcWrite(ledPinBlue1, (int)floor(bluefloat));
  ledcWrite(ledPinRed2, (int)floor(redfloat));
  ledcWrite(ledPinGreen2, (int)floor(greenfloat));
  ledcWrite(ledPinBlue2, (int)floor(bluefloat));
}

void ledHandler::candle(int duration, int reps, int pause, unsigned long startTime, unsigned long timeOffset) {
  Serial.println("should blink");
  //entfernung einbauen
  uint32_t currentTime = micros();
  uint32_t difference = currentTime-timeOffset;
  if (startTime-difference > 0) {
    Serial.println("time now: "+String(micros()));
    Serial.println("Delaying for"+String(startTime-difference+((distance/34300)*1000000*1500)));
    //todo turn this into sleep
    delayMicroseconds(startTime-difference+((distance/34300)*1000000*1500));
    Serial.println("delay done");
  }
  bool heating = true;
  float redsteps = 255.0/(duration/3);
  float greensteps = 255.0/(duration/3);
  float bluesteps = 80.0/(duration/3);
  redfloat = 0.0;
  greenfloat = 0.0;
  bluefloat = 0.0;
  
for (int i = 0; i < reps; i++ ){
  redfloat = 0.0;
  greenfloat = 0.0;
  bluefloat = 0.0;
  for (int j = 0; j <=duration; j++ ) 
    {
    if (redfloat <255) {
      redfloat += redsteps;
    }
    else if (greenfloat <255) {
      greenfloat += greensteps;
    }
    else {
      bluefloat+=bluesteps;
    }
    ledcWrite(ledPinRed2, (int)floor(redfloat));
    ledcWrite(ledPinGreen2, (int)floor(greenfloat));
    ledcWrite(ledPinBlue2, (int)floor(bluefloat));
    ledcWrite(ledPinRed1, (int)floor(redfloat));
    ledcWrite(ledPinGreen1, (int)floor(greenfloat));
    ledcWrite(ledPinBlue1, (int)floor(bluefloat));
    int start = millis();
    while (true) {
      if (millis()-start > 100) {
        break;
      }

    }
    delay(1);
  }
  for (int j = 0; j <=duration; j++ ) 
    {
    if (bluefloat > 0) {
      bluefloat -= bluesteps;
      if (bluefloat <= 0) {
        bluefloat = 0;
      }
    }
    else if (greenfloat > 0) {
      greenfloat -= greensteps;
      if (greenfloat <= 0) {
        greenfloat = 0;
      }
    }
    else {
      redfloat-=redsteps;
      if (redfloat <= 0) {
        redfloat = 0;
      }
    }
    ledcWrite(ledPinRed2, (int)floor(redfloat));
    ledcWrite(ledPinGreen2, (int)floor(greenfloat));
    ledcWrite(ledPinBlue2, (int)floor(bluefloat));
    ledcWrite(ledPinRed1, (int)floor(redfloat));
    ledcWrite(ledPinGreen1, (int)floor(greenfloat));
    ledcWrite(ledPinBlue1, (int)floor(bluefloat));
    delay(1);
  }
  ledsOff(); 
  delay(1);
  }
}





  void ledHandler::blink() {
    Serial.println("should blink");
  uint32_t currentTime = micros();
  //uint32_t difference = currentTime-timeOffset;
  /*
  if (startTime-difference > 0) {
    delayMicroseconds(startTime-difference);
    flash(animationMessage.rgb1[0], animationMessage.rgb1[1], animationMessage.rgb1[2], animationMessage.speed, animationMessage.reps, animationMessage.delay);
  }
  else {
    Serial.println("too late, need to wait to chime in");
    //find algorithm that calculates length of animation and finds later point in time to jump in
  }  */
return;

}


void ledHandler::concentric() {


}

void ledHandler::setDistance(float dist) {
  distance = dist;
}


void ledHandler::setTimerOffset(unsigned long setOffset) {
  timerOffset = setOffset;
}

void ledHandler::setPosition(int id) {
  position = id;
}

void ledHandler::setLocation(int xposition, int yposition, int zposition) {
  xPos = xposition;
  yPos = yposition;
  zPos = zposition;
}
// IF BOARD == V2

void ledHandler::printStatus() {
  return;
  Serial.println("Status of LEDHandler");
  Serial.println("Current Animation "+String(currentAnimation));
  Serial.println("Animation next step "+String(animationNextStep));
  Serial.println("Animation Spread Time"+String(animationMessage.spread_time));
  Serial.println("Current millis time "+String(millis()));
  Serial.println("Current Micros time "+String(micros()));

  Serial.println("repeatCounter "+String(repeatCounter));
  Serial.println("RepeatRuntime "+String(repeatRuntime));
  Serial.println("animationRepeatCounter "+String(animationRepeatCounter));
  Serial.println("Position"+String(position));
  
}


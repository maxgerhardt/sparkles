#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <PeakDetection.h> 
//#include <Preferences.h>
#include <LittleFS.h>
int audioPin = 5;
int freq = 5000;
int resolution = 8;

#define LEDC_TIMER_12_BIT  8
#define LEDC_BASE_FREQ     5000
#define LEDC_START_DUTY   (0)
#define LEDC_TARGET_DUTY  (4095)
#define LEDC_FADE_TIME    (3000)

const int ledPinBlue1 = 17;  // 16 corresponds to GPIO16
const int ledPinRed1 = 38; // 17 cmsgrorresponds to GPIO17
const int ledPinGreen1 = 8;  // 5 corresponds to GPIO5
const int ledPinGreen2 = 3;
const int ledPinRed2 = 9;
const int ledPinBlue2 = 37;
const int ledChannelRed1 = 0;
const int ledChannelGreen1 = 1;
const int ledChannelBlue1 = 2;
const int ledChannelRed2 = 3;
const int ledChannelGreen2 = 4;
const int ledChannelBlue2 = 5;

const int batteryPin = 4; 

struct testing {
  bool lighttest = false;
  bool lighttest_full = false;
  bool light_and_send = false;
  bool claptest = true;
  bool measure_battery = false;
  bool deep_sleep_test = false;
  bool read_write_preferences = false;
  bool read_write_struct_littlefs = false;
} testStruct;

uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
uint8_t myAddress[6];
esp_now_peer_info_t peerInfo;
esp_now_peer_num_t peerNum;
// put function declarations here:
PeakDetection peakDetection; 
//Preferences preferences;
int lastClap;
int claps = 0;
int measurements = 0;
int tick = 0;
unsigned long measurementsTime;
unsigned long measurementsTimeEnd;
unsigned long measurementsTimeRead;
struct message_announce {
  uint8_t messageType = 0;
  unsigned long sendTime;
  int light_level;
  uint8_t address[6];
  int adcVoltage;
  float voltage;
  int counter = 0;
} ;

#define NUM_CLAPS 20

struct message_send_clap_times {
  uint8_t messageType = 1;
  int clapCounter;
  unsigned long timeStamp[NUM_CLAPS]; //offsetted.
};

//4+4*NUM_CLAPS, currently 44
struct client_address {
  uint8_t address[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  int id;
  float xLoc;
  float yLoc;
  float zLoc;
  uint32_t timerOffset;
  int delay;
  message_send_clap_times clapTimes;
  float distance;
};
client_address clientAddress[200];


message_announce announceMessage;

void ledsOff() {
  ledcWrite(ledPinRed2, 0);
  ledcWrite(ledPinGreen2, 0);
  ledcWrite(ledPinBlue2, 0);
  ledcWrite(ledPinRed1, 0);
  ledcWrite(ledPinGreen1, 0);
  ledcWrite(ledPinBlue1, 0);

}

void ledOn(int r, int g, int b, int duration, int frontback) {
  //frontback: 0 front, 1 back, 2 both
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


void writeStructsToFile(const client_address* data, int count, const char* filename) {
    File file = LittleFS.open(filename, "w");
    if (!file) {
        Serial.println("Failed to open file for writing");
        return;
        
    }
    for (int i = 0; i < count; i++) {
        file.write((uint8_t*)&data[i], sizeof(client_address));
    }
    file.close();
}

// Function to read an array of structs from a file
void readStructsFromFile(client_address* data, int count, const char* filename) {
    File file = LittleFS.open(filename, "r");
    if (!file) {
        Serial.println("Failed to open file for reading");
        return;
    }
    for (int i = 0; i < count; i++) {
        file.read((uint8_t*)&data[i], sizeof(client_address));
    }
    file.close();
    
}

void setup() {
    Serial.begin(115200);
  while (!Serial) {
    ;
  }
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;

 if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }

    if (!LittleFS.begin()) {
        Serial.println("LittleFS Mount Failed");
        if (LittleFS.format()) {
            Serial.println("LittleFS formatted successfully");
        } else {
            Serial.println("LittleFS format failed");
            return;
        }
    }

  analogReadResolution(12);
  analogSetPinAttenuation(batteryPin, ADC_11db);
  WiFi.macAddress(myAddress);
  pinMode(audioPin, INPUT); 
  peakDetection.begin(30, 3, 0);  
  esp_sleep_enable_timer_wakeup(10000000); 
  ledcAttach(ledPinRed1, LEDC_BASE_FREQ, LEDC_TIMER_12_BIT);
  ledcAttach(ledPinGreen1, LEDC_BASE_FREQ, LEDC_TIMER_12_BIT);
  ledcAttach(ledPinBlue1, LEDC_BASE_FREQ, LEDC_TIMER_12_BIT);
  ledcAttach(ledPinRed2, LEDC_BASE_FREQ, LEDC_TIMER_12_BIT);
  ledcAttach(ledPinGreen2, LEDC_BASE_FREQ, LEDC_TIMER_12_BIT);
  ledcAttach(ledPinBlue2, LEDC_BASE_FREQ, LEDC_TIMER_12_BIT);
  ledsOff();
  Serial.println("Setup done ");
  //preferences.begin("sparkles", true);
  // put your setup code here, to run once:
}

void loop() {
while (true) {
  Serial.println("start "+String(tick) );
  delay(100);
  tick++;
  if (tick == 1) {
    tick = 0;
    break;
  }
}


if (testStruct.lighttest == true) {
  Serial.println("Leds individually on");
  Serial.println("Red front");
  ledOn(255, 0, 0, 5000, 0);
  ledsOff();
  Serial.println("Green front");
  ledOn(0, 255, 0, 5000, 0);
  ledsOff();
  Serial.println("Blue front");
  ledOn(0, 0, 255, 5000, 0);
  ledsOff();
  Serial.println("Red back");
  ledOn(255, 0, 0, 5000, 1);
  ledsOff();
  Serial.println("Green back");
  ledOn(0, 255, 0, 5000, 1);
  ledsOff();
  Serial.println("Blue back ");
  ledOn(0, 0, 255, 5000, 1);
  ledsOff();
  Serial.println("Leds together");
  Serial.println("Red both");
  ledOn(255, 0, 0, 5000, 2);
  ledsOff();
  Serial.println("Green both");
  ledOn(0, 255, 0, 5000, 2);
  ledsOff();
  Serial.println("Blue both ");
  ledOn(0, 0, 255, 5000, 2);
  ledsOff();
}
if (testStruct.lighttest_full == true) {
  Serial.println("all");
  ledOn(255, 255, 255, 5000, 2);
  ledsOff();
}
if (testStruct.light_and_send == true) {
  Serial.println("lamps on 64 and sending data");
  announceMessage.light_level = 64;
  announceMessage.counter++;
  ledOn(64, 64, 64, 0, 2);
  esp_now_send(broadcastAddress, (uint8_t *) &announceMessage, sizeof(announceMessage));
  delay(500);
  ledsOff();
  Serial.println("lamps on 128 and sending data");
  announceMessage.light_level = 128;
  announceMessage.counter++;
  ledOn(128, 128, 128, 0, 2);
  esp_now_send(broadcastAddress, (uint8_t *) &announceMessage, sizeof(announceMessage));
  delay(500);
  ledsOff();
  Serial.println("lamps on 192 and sending data");
  announceMessage.light_level = 192;
  announceMessage.counter++;
  ledOn(192, 192, 192, 0, 2);
  delay(500);
  ledsOff();
 esp_now_send(broadcastAddress, (uint8_t *) &announceMessage, sizeof(announceMessage));
/*
  Serial.println("lamps on 255 and sending data");
  announceMessage.light_level = 255;
  ledOn(255, 255, 255, 0, 2);

  esp_now_send(broadcastAddress, (uint8_t *) &announceMessage, sizeof(announceMessage));
  */
  delay(1000);
  ledsOff();
  announceMessage.light_level = 0;

}
if (testStruct.claptest == true) {
  Serial.println("start clapping");

  //Serial.println(sensorValue);
  while (true) {
    double data = (double)analogRead(audioPin)/512-1;
    peakDetection.add(data); 
    int peak = peakDetection.getPeak(); 
    double filtered = peakDetection.getFilt(); 
    if (peak == -1 and millis() > lastClap+1000) {
      lastClap = millis();
      Serial.println("Clap!");
      ledOn(255, 0, 0, 100, true);
      ledsOff();
      claps++;
    }
    if (claps == 3) {
      claps = 0;
      break;
    }
  }
}
if (testStruct.measure_battery == true) {
  while(true) {
    int adcValue = analogRead(batteryPin); // Read the ADC value
    float voltage = adcValue * (3.9 / 4095.0);
    Serial.print("ADC Value: ");
    Serial.print(adcValue);
    Serial.print(" | Voltage: ");
    Serial.print(voltage);
    Serial.println("V");
    announceMessage.adcVoltage = adcValue;
    announceMessage.voltage = voltage;
    announceMessage.counter++;
    esp_now_send(broadcastAddress, (uint8_t *) &announceMessage, sizeof(announceMessage));
    delay(1000);
    measurements++;
    if (measurements == 2) {
      measurements = 0;
      break;
    }
  }
}


if (testStruct.deep_sleep_test == true) {
  Serial.println("Going to sleep now"+String(micros()));
  esp_light_sleep_start();
  Serial.println("Woke up"+String(micros()));
}

if (testStruct.read_write_preferences == true) {
  while (true) {
    Serial.println("Size: "+String(sizeof(clientAddress)));
    testStruct.read_write_preferences = false;
    break;
    }
/*    measurementsTime = micros();
    measurementsTimeRead = preferences.getULong("time");
    measurementsTimeEnd = micros();
    Serial.println("Starting time reading from preferences: "+String(measurementsTime)+" Time to read "+String(measurementsTimeEnd-measurementsTime));
    measurements++;

    if (measurements == 10) {
      measurements = 0;
      break;
    }
  }
  */
}
  if (testStruct.read_write_struct_littlefs == true) {
    /*
     Serial.println("hä");
      while (true) { 
        
        for (int i = 0; i < 200; i++)
        {
        clientAddress[i].address[0] = 0x01;
        clientAddress[i].address[1] = 0x02;
        clientAddress[i].address[2] = 0x03;
        clientAddress[i].address[3] = 0x04;
        clientAddress[i].address[4] = 0x05;
        clientAddress[i].address[5] = 0x06;
        clientAddress[i].id = 1;
        clientAddress[i].xLoc = 1.0;
        clientAddress[i].yLoc = 2.0;
        clientAddress[i].zLoc = 3.0;
        clientAddress[i].timerOffset = 4;
        clientAddress[i].delay = 5;
        clientAddress[i].clapTimes.clapCounter = 6;
        clientAddress[i].clapTimes.timeStamp[0] = 7;
        clientAddress[i].clapTimes.timeStamp[1] = 8;
        clientAddress[i].clapTimes.timeStamp[2] = 9;
        clientAddress[i].clapTimes.timeStamp[3] = 10;
        clientAddress[i].clapTimes.timeStamp[4] = 11;
        clientAddress[i].clapTimes.timeStamp[5] = 12;
        clientAddress[i].clapTimes.timeStamp[6] = 13;
        }

        Serial.println("Size: "+String(sizeof(clientAddress)));
        writeStructsToFile(clientAddress, 200, "/clientAddress");        
        clientAddress[10].id = 8;
        memset(clientAddress, 0, sizeof(clientAddress));
        readStructsFromFile(clientAddress, 200,  "/clientAddress");
            Serial.print("Address: ");
        for (int i = 0; i < 6; i++) {
            Serial.print(clientAddress[10].address[i], HEX);
            if (i < 5) Serial.print(":");
        }
 
        Serial.println();
        Serial.println("ID: "+String(clientAddress[10].id));
        Serial.println("X: "+String(clientAddress[10].xLoc));
        Serial.println("Y: "+String(clientAddress[10].yLoc));
        Serial.println("Z: "+String(clientAddress[10].zLoc));
        Serial.println("TimerOffset: "+String(clientAddress[10].timerOffset));
        Serial.println("Delay: "+String(clientAddress[10].delay));
        Serial.println("ClapCounter: "+String(clientAddress[10].clapTimes.clapCounter));
        testStruct.read_write_struct_littlefs = false; 
       break;
      }
      */
  }
}


  // put your main code here, to run repeatedly:



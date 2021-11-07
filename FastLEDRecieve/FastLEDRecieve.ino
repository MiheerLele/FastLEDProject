// FastLED - Version: Latest 
#include <FastLED.h>
#include <Wire.h>

#define DEVICE_NUM 4
#define NUM_LEDS 50
#define LED_PIN 2
#define MIC_PIN A2
#define LED_TYPE WS2811
#define ORDER BRG

// Color Variables
CRGB led[NUM_LEDS];
CRGB currColor;
bool stripChanged = false;

// Fade Variables
uint8_t maxBrightness;
bool isFade = false;
uint8_t fadeCount = 1;

// Pulse Variables
bool isPulse = false;
uint8_t pulseCount = 1;

// Gamer Lights Variables
bool isGamer = false;
uint32_t rainbowCount = 1;

// Mic Variables
bool mic = false;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  delay(7000); // Let 33 iot connect to internet

//  pinMode(SDA, INPUT); // Disable built in pull up resistor
//  pinMode(SCL, INPUT);
  Wire.begin(DEVICE_NUM); // join i2c bus with address DEVICE_NUM
  Wire.onReceive(receiveEvent); // register event
  
  pinMode(MIC_PIN, INPUT);
  FastLED.addLeds<LED_TYPE, LED_PIN, ORDER>(led, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setMaxPowerInVoltsAndMilliamps(12, 2750); // Limit power draw to 36 W, my power supply is rated for 60
  Serial.println("Setup Complete");
}

void loop() {
  if (stripChanged) { updateStrip(); }
  if (isFade) { fadeColor(2); }
  if (isPulse) { pulseColor(); }
  if (isGamer) { swirlRainbow(2); }
  if (mic) { soundPulse(); }
  //Serial.println(analogRead(MIC_PIN));
  //delay(50);
  FastLED.show();
}

void receiveEvent(int howMany) {
  switch(Wire.read()) { // Read event byte
      case 0:
        handleColorChange();
        break; // Color Change
      case 1:
        isFade = Wire.read(); // 0 for false, >0 for true
        if (isFade) { fadeOn(); }
        else { fadeOff(); }
        break; // Fade Change;
      case 2:
        isPulse = Wire.read();// 0 for false, >0 for true
        if (isPulse) { pulseOn(); }
        else { pulseOff(); }
        break; // Pulse Change;
      case 3:
        isGamer = Wire.read();
        if (isGamer) { gamerOn(); }
        else { gamerOff(); }
        break; // Gamer Lights Change
      case 4:
        mic = Wire.read();
        if (mic) { micOn(); }
        else { micOff(); }
        break;
    }
}


// ------------- Color Functions -------------
void handleColorChange() {
  if (Wire.read()) { // If light on
    uint8_t r = Wire.read();
    uint8_t g = Wire.read();
    uint8_t b = Wire.read();
    currColor = CRGB(r, g, b);
    maxBrightness = Wire.read() * 2.55; // the max brightness out of alexa is 100
  } else {
    currColor = CRGB::Black;
  }
  stripChanged = true; 
}

void updateStrip() {
  FastLED.setBrightness(maxBrightness);
  for (int i = 0; i < NUM_LEDS; i++) {
    led[i] = currColor;
    FastLED.show();
    FastLED.delay(1);
  }
  stripChanged = false;
}

// ------------- Fade Functions -------------
void fadeColor(uint8_t fadeSpeed) {
  EVERY_N_MILLISECONDS(50) {
    FastLED.setBrightness(triwave8(fadeCount) * maxBrightness / 255); // Scale the brightness to the max
    fadeCount += fadeSpeed;
  }
}

void fadeOn() {
  pulseOff();
  gamerOff();
  micOff();
}

void fadeOff() {
  isFade = false;
  stripChanged = true;
}

// ------------- Pulse Functions -------------
void pulseColor() {
  EVERY_N_MILLISECONDS(50) {
    for (int i = NUM_LEDS - 1; i > 0; i--) { // Has to go back to front, otherwise chain reaction will leave whole strip as the color
      led[i] = led[i - 1];
    }
    pulseCount++;
    sendPulse(2);
  }
}

void sendPulse(int numPulses) {
  if (pulseCount % (NUM_LEDS / numPulses) == 0) {
    led[0] = currColor;
  } else {
    led[0].fadeToBlackBy(32);
  }
}

void startPulse() {
  pulseCount = 1;
  led[0] = currColor;
}

void pulseOn() {
  fadeOff();
  gamerOff();
  micOff();
  startPulse();
}

void pulseOff() {
  isPulse = false;
  stripChanged = true;
}

// ------------- Gamer Functions -------------
void swirlRainbow(uint8_t animSpeed) {
  EVERY_N_MILLISECONDS(50) {
    // IMPORTANT: As of FastLED 3.003.003, fill rainbow has a random red pixel around hue = 60.
    // This is from a compiler optimization issue.
    // The workaround is to change the lines setting hsv.sat from 240 to 255 in colorutils.ccp
    fill_rainbow(led, NUM_LEDS, rainbowCount % 255, 255 / NUM_LEDS);
    rainbowCount += animSpeed;
  }
}

void gamerOn() {
  fadeOff();
  pulseOff();
  micOff();
}

void gamerOff() {
  isGamer = false;
  stripChanged = true;
}

// ------------- Sound Functions -------------
void soundPulse() {
  EVERY_N_MILLISECONDS(50) {
    int vol = analogRead(MIC_PIN);
    fill_solid(led, NUM_LEDS, CHSV(triwave8(rainbowCount), 255, 36.8*log(vol)));
    rainbowCount++;
  }
}

void micOn() {
  fadeOff();
  pulseOff();
  gamerOff();
}

void micOff() {
  mic = false;
  stripChanged = true;
}

// FastLED - Version: Latest 
#include <FastLED.h>
#include <Wire.h>

#define DEVICE_NUM 4
#define NUM_LEDS 50
#define LED_PIN 2
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


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Wire.begin(DEVICE_NUM); // join i2c bus with address DEVICE_NUM

//  pinMode(SDA, INPUT); // Disable built in pull up resistor
//  pinMode(SCL, INPUT);
  
  Wire.onReceive(receiveEvent); // register event
  delay(2000);
  FastLED.addLeds<LED_TYPE, LED_PIN, ORDER>(led, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setMaxPowerInVoltsAndMilliamps(12, 2750); // Limit power draw to 36 W, my power supply is rated for 60
  Serial.println("Setup Complete");
}

void loop() {
  if (stripChanged) { updateStrip(); }
  if (isFade) { fadeColor(); }
  if (isPulse) { pulseColor(); }
  if (isGamer) { swirlRainbow(); }
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
void fadeColor() {
  EVERY_N_MILLISECONDS(50) {
    FastLED.setBrightness(triwave8(fadeCount) * maxBrightness / 255); // Scale the brightness to the max
    fadeCount += 2;
  }
}

void fadeOn() {
  pulseOff();
  gamerOff();
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
  startPulse();
}

void pulseOff() {
  isPulse = false;
  stripChanged = true;
}

// ------------- Gamer Functions -------------
void swirlRainbow() {
  CRGB c;
  EVERY_N_MILLISECONDS(50) {
    for(int i=0; i< NUM_LEDS; i++) {
      c = getColor(((i * 256 / NUM_LEDS) + rainbowCount) & 255);
      led[i] = c;
    }
    rainbowCount += 2; // Higher the faster 
  }
}

// From: https://www.tweaking4all.com/hardware/arduino/adruino-led-strip-effects/
CRGB getColor(byte pos) {
  CRGB c;
 
  if(pos < 85) {
   c[0]=pos * 3;
   c[1]=255 - pos * 3;
   c[2]=0;
  } else if(pos < 170) {
   pos -= 85;
   c[0]=255 - pos * 3;
   c[1]=0;
   c[2]=pos * 3;
  } else {
   pos -= 170;
   c[0]=0;
   c[1]=pos * 3;
   c[2]=255 - pos * 3;
  }

  return c;
}

void gamerOn() {
  fadeOff();
  pulseOff();
}

void gamerOff() {
  isGamer = false;
  stripChanged = true;
}

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <TimerOne.h>
#include "RTClib.h" // Library for the RTC
#include "TM1637.h"
#define ON 1
#define OFF 0

// http://henrysbench.capnfatz.com/henrys-bench/arduino-sensors-and-input/keyes-ky-040-arduino-rotary-encoder-user-manual/
// https://bigdanzblog.wordpress.com/2014/08/16/using-a-ky040-rotary-encoder-with-arduino/
// https://github.com/adafruit/Adafruit_NeoPixel/blob/master/Adafruit_NeoPixel.cpp

int PinDT  = 12;  // Connected to DT on KY-040
int PinCLK = 4;  // Connected to CLK on KY-040 // INT 0 on Teensy 2.0
int PinSW  = 8;  // Connected to SW on KY-040  // INT 1 on Teensy 2.0
int PinNeo = 15;
#define CLK 19 //pins definitions for TM1637 and can be changed to other ports
#define DIO 18

int8_t TimeDisp[] = {0x00,0x00,0x00,0x00};
unsigned char ClockPoint = 1;
unsigned char Update;

TM1637 tm1637(CLK,DIO);
RTC_DS1307 RTC;

int onState = 0;
int encoderPosNight = 10; // We will start with this for a night light
int encoderPosMax = 250;
int encoderPosMin = 5;
int encoderPosCount = encoderPosNight;
int pinCLKlast;
int aVal;
int buttonState = 0;         // variable for reading the pushbutton status
boolean bCW;

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(7, PinNeo, NEO_GRB + NEO_KHZ800);

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

void changeBrightness ()  {
 aVal = digitalRead(PinCLK);
 // if the knob is rotating, we need to determine direction
 // We do that by reading pin DT.
 if (digitalRead(PinDT) != aVal) {  // Means pin A Changed first - We're Rot ating Clockwise
   if(encoderPosCount < encoderPosMax) {
     encoderPosCount=encoderPosCount+8;
   }
   bCW = true;
 } else {// Otherwise B changed first and we're moving CCW
   bCW = false;
   if(encoderPosCount > encoderPosMin) {
     encoderPosCount=encoderPosCount-8;
   }
 }
 Serial.print ("Rotated: ");
 if (bCW){
   Serial.println ("clockwise");
 }else{
   Serial.println("counterclockwise");
 }
 Serial.print("Encoder Position: ");
 Serial.println(encoderPosCount);

 strip.setBrightness(encoderPosCount);
}

void encoderSwitch ()  {
 buttonState = digitalRead(PinSW);

 onState = 1 % onState; // Modulus Operator

 if(onState == 0) {
  strip.setBrightness(0);
 } else {
  encoderPosCount=encoderPosNight;
  strip.setBrightness(encoderPosNight);
 }

 Serial.print("buttonState: ");
 Serial.println(buttonState);
 Serial.print("onState: ");
 Serial.println(onState);

 delay(1000); // Not sure this helps. Need more testing
}

void setup() {
 RTC.begin();
 RTC.adjust(DateTime(__DATE__, __TIME__));

 tm1637.set(.5);
 tm1637.init();
 Timer1.initialize(500000);//timing for 500ms
 Timer1.attachInterrupt(TimingISR);//declare the interrupt serve routine:TimingISR

 pinMode (PinCLK,INPUT);
 pinMode (PinDT,INPUT);
 pinMode (PinSW,INPUT);
 /* Read Pin CLK
 Whatever state it's in will reflect the last position
 */
 pinCLKlast = digitalRead(PinCLK);
// Serial.begin (9600);
 Serial.begin (115200);

  attachInterrupt(0, changeBrightness, FALLING);   // interrupt 0
  attachInterrupt(3, encoderSwitch, FALLING);   // interrupt 1

  Serial.println("Start");


  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  onState = 0;
  strip.setBrightness(0);
}


void loop() {
 rainbowCycle(50);

   if(Update == ON) {
    TimeUpdate();
    tm1637.display(TimeDisp);
  }
}



// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}


// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

void TimingISR() {
  Update = ON;
 // Serial.println(second);
  ClockPoint = (~ClockPoint) & 0x01;
}
void TimeUpdate(void) {
  DateTime now = RTC.now();

  if(ClockPoint)tm1637.point(POINT_ON);
  else tm1637.point(POINT_OFF);
  TimeDisp[0] = now.hour() / 10;
  if(now.hour() > 12) {
    TimeDisp[1] = (now.hour() % 10)-12;
  } else {
    TimeDisp[1] = now.hour() % 10;
  }
  TimeDisp[2] = now.minute() / 10;
  TimeDisp[3] = now.minute() % 10;
  Update = OFF;
}

/*
  Hourglass OLED project
 */

//I2C version
//https://github.com/ThingPulse/esp8266-oled-ssd1306
#include <Wire.h>
//#include "SSD1306.h"
#include "SH1106Wire.h"

// ESP-WROOM-32 has LED on pin 2:
int led = 2;

struct Sand
{
  char x;
  char y;
};

// I2C version
//SSD1306 display(0x3c, 18, 19);
SH1106Wire display(0x3c, 18, 19);
struct Sand s1 = {12,12};

// the setup routine runs once when you press reset:
void setup() {                
  // initialize the digital pin as an output.
  pinMode(led, OUTPUT);

  
  display.init();
  display.drawString(0, 0, "Hello World");
  display.display();

  
  display.setPixel(s1.x, s1.y);
  display.display();
  
}

// the loop routine runs over and over again forever:
void loop() {
  static int nexttime = millis();
  if (millis() > nexttime) {
    display.clearPixel(s1.x, s1.y);
    s1.y += 1;
    display.setPixel(s1.x, s1.y);
    display.display();
    nexttime = millis()+500;
  }
  
  digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(100);               // wait for a second
  digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW
  delay(100);               // wait for a second
}

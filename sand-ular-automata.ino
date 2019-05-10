/*
  Hourglass OLED project

  Dependencies:
    ESP32 Arduino support https://randomnerdtutorials.com/installing-the-esp32-board-in-arduino-ide-mac-and-linux-instructions/

    Clone this forked repo into ~/Arduino/libraries/
      git@github.com:szczys/esp8266-oled-ssd1306.git
 */



//I2C version
//https://github.com/ThingPulse/esp8266-oled-ssd1306
#include <Wire.h>
//#include "SSD1306.h"
#include "SH1106Wire.h"
#include "hourglass.h"


#define GRAINSWIDE  64 //GRAINSWIDE must be divisible by 8
#define GRAINSDEEP  64

// ESP-WROOM-32 has LED on pin 2:
int led = 2;

uint8_t buff[(GRAINSWIDE/8)*GRAINSDEEP];
uint8_t lastbuff[(GRAINSWIDE/8)*GRAINSDEEP];
uint8_t toggle;
/*
uint8_t buff[32*4] = {
  0b11111111, 0b00000000, 0b00000000, 0b00000000,
  0b11111111, 0b00000000, 0b00000000, 0b00000000,
  0b11111111, 0b00000000, 0b00000000, 0b00000000,
  0b11111111, 0b00000000, 0b00000000, 0b00000000,
  0b11111111, 0b00000000, 0b00000000, 0b00000000,
  0b11111111, 0b00000000, 0b00000000, 0b00000000,
  0b11111111, 0b00000000, 0b00000000, 0b00000000,
  0b11111111, 0b00000000, 0b00000000, 0b00000000,
  0b11111111, 0b00000000, 0b00000000, 0b00000000,
  0b11111111, 0b00000000, 0b00000000, 0b00000000,
  0b11111111, 0b00000000, 0b00000000, 0b00000000,
  0b11111111, 0b00000000, 0b00000000, 0b00000000,
  0b11111111, 0b00000000, 0b00000000, 0b00000000,
  0b11111111, 0b00000000, 0b00000000, 0b00000000,
  0b11111111, 0b00000000, 0b00000000, 0b00000000,
  0b11111111, 0b00000000, 0b00000000, 0b00000000,
  0b11111111, 0b00000000, 0b00000000, 0b00000000,
  0b11111111, 0b00000000, 0b00000000, 0b00000000,
  0b11111111, 0b00000000, 0b00000000, 0b00000000,
  0b11111111, 0b00000000, 0b00000000, 0b00000000,
  0b11111111, 0b00000000, 0b00000000, 0b00000000,
  0b11111111, 0b00000000, 0b00000000, 0b00000000,
  0b11111111, 0b00000000, 0b00000000, 0b00000000,
  0b11111111, 0b00000000, 0b00000000, 0b00000000,
  0b11111111, 0b00000000, 0b00000000, 0b00000000,
  0b11111111, 0b00000000, 0b00000000, 0b00000011,
  0b11111111, 0b00000000, 0b00000000, 0b00000011,
  0b11111111, 0b00000000, 0b00000000, 0b00000011,
  0b11111111, 0b00000000, 0b00000000, 0b00000011,
  0b11111111, 0b00000000, 0b00000000, 0b00000011,
  0b11111111, 0b00000000, 0b00000000, 0b00000011,
  0b11111111, 0b00000000, 0b00000000, 0b00000011,
};
*/

struct Sand
{
  char x;
  char y;
};



// I2C version
//SSD1306 display(0x3c, 18, 19);
SH1106Wire display(0x3c, 18, 19);
struct Sand s1 = {12,12};

void clearBuff(void);
void showBuf(uint16_t xoffset, uint16_t yoffset);
uint8_t getSand(uint16_t x, uint16_t y, uint8_t framebuffer[(GRAINSWIDE/8)*GRAINSDEEP]);


void clearBuff(void) {
  for (uint16_t i=0; i<((GRAINSWIDE/8)*GRAINSDEEP); i++) {
    buff[i] = hourglass[i];
    lastbuff[i] = 0;
  }
}

void showBuf(uint16_t xoffset, uint16_t yoffset) {
  display.clear(); //drawFastImage doesn't draw black pixels so clear first
  uint16_t sandcount = 0;
  for (uint8_t i=0; i<64; i++) {
    for (uint8_t j=0; j<64; j++) {
      if (getSand(j,i,buff)) ++sandcount;
    }
  }
  char sbuf[20];
  itoa(sandcount,sbuf,10);
  display.drawString(0, 11, sbuf);
  display.drawFastImage(xoffset, yoffset, GRAINSWIDE, GRAINSDEEP, buff);
}

uint8_t getSand(uint16_t x, uint16_t y, uint8_t framebuffer[(GRAINSWIDE/8)*GRAINSDEEP]) {
  uint16_t byteIdx = y*(GRAINSDEEP/8);
  uint16_t byteOffset = x/8;
  uint16_t byteLoc = x%8;

  /*
  //Imagedraw places left to right as top to bottom, and top to bottom as left to right
  uint16_t byteIdx = x*(GRAINSWIDE/8);
  uint16_t byteOffset = (y/8);
  uint16_t byteLoc = y%8;
  */
  if (framebuffer[byteIdx+byteOffset] & (1<<byteLoc)) return 1;
  else return 0;  
}

void setSand(uint16_t x, uint16_t y, uint8_t onoff) {
  uint16_t byteIdx = y*(GRAINSDEEP/8);
  uint16_t byteOffset = (x/8);
  uint16_t byteLoc = x%8;

  /*
  //Imagedraw places left to right as top to bottom, and top to bottom as left to right
  uint16_t byteIdx = x*(GRAINSWIDE/8);
  uint16_t byteOffset = (y/8);
  uint16_t byteLoc = y%8;
  */
  if (onoff > 0) { buff[byteIdx+byteOffset] |= (1<<byteLoc); }
  else { buff[byteIdx+byteOffset] &= ~(1<<byteLoc); }
}

uint8_t notTouchingGlass(uint16_t x, uint16_t y) {
  //Sand *should* always be in the hour glass so we don't check for y-axis buffer overflows
  if (y>0) {
    if (getSand(x,y-1,hourglass)) return 0;
    if (getSand(x+1,y-1,hourglass)) return 0;
    if (getSand(x-1,y-1,hourglass)) return 0;
  }
  if (getSand(x+1,y,hourglass)) return 0;
  if (getSand(x-1,y,hourglass)) return 0;

  if (y<(GRAINSDEEP-1)) {
    if (getSand(x,y+1,hourglass)) return 0;
    if (getSand(x+1,y+1,hourglass)) return 0;
    if (getSand(x-1,y+1,hourglass)) return 0;
  }
  return 1;
}

/*
 * Cellular automata scheme:
 * 
 * if cell below is empty, drop
 * if cell below is full but below to the left is empty, fall there, otherwise fall sell below to the right
 * now that individual cells have fallen, check row issues:
 *   if row above is entirely full, and this row has empty spaces near the edges, move one grain toward that empty space
 */

void moveSand(void) {
  //preserve current state
  //for (uint16_t i=0; i<((GRAINSWIDE/8)*GRAINSDEEP); i++) { lastbuff[i] = buff[i]; }
  
  /* if cell below is empty, drop */
  for (int16_t row=GRAINSDEEP-2; row>=0; row--) {
    for (uint16_t col=0; col<GRAINSWIDE; col++) {
      //Check if we should be dropping this grain
      if (getSand(col,row, hourglass)) continue;  //Don't move cells that make up the hourglass itself
      if (getSand(col,row, buff)) {
        if ((getSand(col,row+1, buff) == 0) && (notTouchingGlass(col,row+1))) {
          setSand(col,row,0); setSand(col,row+1,1); continue;
        }
        //Toggle alternates directions checked first, otherwise operations are the same
        if (toggle) {
          toggle=0;
          if ((col > 0) && (getSand(col-1,row+1, buff) == 0) && (notTouchingGlass(col-1,row+1))) {
            setSand(col,row,0); setSand(col-1,row+1,1); continue;
          }
          
          if ((col < (GRAINSWIDE-1)) && (getSand(col+1,row+1, buff) == 0) && (notTouchingGlass(col+1,row+1))) {
            setSand(col,row,0); setSand(col+1,row+1,1);
          }
        }
        else {
          ++toggle;       
          if ((col < (GRAINSWIDE-1)) && (getSand(col+1,row+1, buff) == 0) && (notTouchingGlass(col+1,row+1))) {
            setSand(col,row,0); setSand(col+1,row+1,1);
          }
          if ((col > 0) && (getSand(col-1,row+1, buff) == 0) && (notTouchingGlass(col-1,row+1))) {
            setSand(col,row,0); setSand(col-1,row+1,1); continue;
          }
        }
      }
    }
  }
}

void reverseSand(void) {
  //preserve current state
  //for (uint16_t i=0; i<((GRAINSWIDE/8)*GRAINSDEEP); i++) { lastbuff[i] = buff[i]; }
  
  /* if cell below is empty, drop */
  for (int16_t row=1; row<GRAINSDEEP-1; row++) {
    for (int16_t col=GRAINSWIDE-1; col>=0; col--) {
      //Check if we should be dropping this grain
      if (getSand(col,row, hourglass)) continue;  //Don't move cells that make up the hourglass itself
      if (getSand(col,row, buff)) {
        if ((getSand(col,row-1, buff) == 0) && (notTouchingGlass(col,row-1))) {
          setSand(col,row,0); setSand(col,row-1,1); continue;
        }
        //Toggle alternates directions checked first, otherwise operations are the same
        if (toggle) {
          toggle = 0;
          if ((col > 0) && (getSand(col-1,row-1, buff) == 0) && (notTouchingGlass(col-1,row-1))){
            setSand(col,row,0); setSand(col-1,row-1,1); continue;
          }
          if ((col < (GRAINSWIDE-1)) && (getSand(col+1,row-1, buff) == 0) && (notTouchingGlass(col+1,row-1))) {
            setSand(col,row,0); setSand(col+1,row-1,1);
          }
        }
        else {
          ++toggle;
          if ((col < (GRAINSWIDE-1)) && (getSand(col+1,row-1, buff) == 0) && (notTouchingGlass(col+1,row-1))) {
            setSand(col,row,0); setSand(col+1,row-1,1);
          }
          if ((col > 0) && (getSand(col-1,row-1, buff) == 0) && (notTouchingGlass(col-1,row-1))){
            setSand(col,row,0); setSand(col-1,row-1,1); continue;
          }
        }
      }
    }
  }
}

// the setup routine runs once when you press reset:
void setup() {                
  // initialize the digital pin as an output.
  pinMode(led, OUTPUT);
  toggle = 0;
  clearBuff();

  /*
  //Fill with test sand
  for (uint8_t i=58; i>47; i--) {
    for (uint8_t j=24; j<39; j++) {
      setSand(j,i,1);
    }
  }
  */
  
  display.init();
  display.drawString(0, 0, "Hello Sandular");
  showBuf(64,0);
  display.display();

  
  display.setPixel(s1.x, s1.y);
  display.display();
  
}

// the loop routine runs over and over again forever:
void loop() {
  static int nexttime = millis();
  static int nextframe = millis() + 10;
  static int counter = 0;
  if (millis() > nexttime) {
    if (counter++ < 150) setSand(32,0,1);
    else setSand(32,0,0);    
    showBuf(64,0);
    
    //display.clearPixel(s1.x, s1.y);
    //s1.y += 1;
    //display.setPixel(s1.x, s1.y);
    display.display();
    nexttime = millis()+300;
    //if (digitalRead(led)) digitalWrite(led,LOW);
    //else digitalWrite(led,HIGH);
  }

  if (millis() > nextframe) {
    if ((counter < 150) || (counter > 200)) moveSand();
    else reverseSand();
    showBuf(64,0);
    display.display();
    nextframe = millis()+10;
  }

  /*
  digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(100);               // wait for a second
  digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW
  delay(100);               // wait for a second
  */
}

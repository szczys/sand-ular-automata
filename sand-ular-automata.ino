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

const int MPU_addr=0x68;  // I2C address of the MPU-6050
int16_t AcX,AcY,AcZ,Tmp,GyX,GyY,GyZ;


#define GRAINSWIDE  64 //GRAINSWIDE must be divisible by 8
#define GRAINSDEEP  64

// ESP-WROOM-32 has LED on pin 2:
int led = 2;

uint8_t botbuff  [(GRAINSWIDE/8)*GRAINSDEEP];
uint8_t topbuff [(GRAINSWIDE/8)*GRAINSDEEP];
uint8_t toggle;

// I2C version
//SSD1306 display(0x3c, 18, 19);
SH1106Wire display(0x3c, 18, 19);

void clearBuff(void);
void showBuf(void);
uint8_t getSand(uint16_t x, uint16_t y, uint8_t framebuffer[(GRAINSWIDE/8)*GRAINSDEEP]);


void clearBuff(void) {
  for (uint16_t i=0; i<((GRAINSWIDE/8)*GRAINSDEEP); i++) {
    botbuff  [i] = hourglassbot[i];
    topbuff [i] = hourglasstop[i];
  }
}

void showBuf(void) {
  display.clear(); //drawFastImage doesn't draw black pixels so clear first
  /*
  uint16_t sandcount = 0;
  for (uint8_t i=0; i<64; i++) {
    for (uint8_t j=0; j<64; j++) {
      if (getSand(j,i,botbuff  )) ++sandcount;
    }
  }
  char sbuf[20];
  itoa(sandcount,sbuf,10);
  display.drawString(0, 11, sbuf);
  */
  display.drawFastImage(64, 0, GRAINSWIDE, GRAINSDEEP, botbuff);
  display.drawFastImage(0, 0, GRAINSWIDE, GRAINSDEEP, topbuff);
}

uint8_t getSand(uint16_t x, uint16_t y, uint8_t framebuffer[(GRAINSWIDE/8)*GRAINSDEEP]) {
  uint16_t byteIdx = y*(GRAINSDEEP/8);
  uint16_t byteOffset = x/8;
  uint16_t byteLoc = x%8;

  if (framebuffer[byteIdx+byteOffset] & (1<<byteLoc)) return 1;
  else return 0;  
}

void setSand(uint16_t x, uint16_t y, uint8_t onoff, uint8_t framebuffer[(GRAINSWIDE/8)*GRAINSDEEP]) {
  uint16_t byteIdx = y*(GRAINSDEEP/8);
  uint16_t byteOffset = (x/8);
  uint16_t byteLoc = x%8;

  if (onoff > 0) { framebuffer[byteIdx+byteOffset] |= (1<<byteLoc); }
  else { framebuffer[byteIdx+byteOffset] &= ~(1<<byteLoc); }
}

uint8_t notTouchingGlass(uint16_t x, uint16_t y, uint8_t glassbuffer[(GRAINSWIDE/8)*GRAINSDEEP]) {
  //Sand *should* always be in the hour glass so we don't check for y-axis buffer overflows
  if (y>0) {
    if (getSand(x,y-1,glassbuffer)) return 0;
    if (getSand(x+1,y-1,glassbuffer)) return 0;
    if (getSand(x-1,y-1,glassbuffer)) return 0;
  }
  if (getSand(x+1,y,glassbuffer)) return 0;
  if (getSand(x-1,y,glassbuffer)) return 0;

  if (y<(GRAINSDEEP-1)) {
    if (getSand(x,y+1,glassbuffer)) return 0;
    if (getSand(x+1,y+1,glassbuffer)) return 0;
    if (getSand(x-1,y+1,glassbuffer)) return 0;
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

void moveSand(uint8_t framebuffer[(GRAINSWIDE/8)*GRAINSDEEP], uint8_t glassbuffer[(GRAINSWIDE/8)*GRAINSDEEP]) {  
  /* if cell below is empty, drop */
  for (int16_t row=GRAINSDEEP-2; row>=0; row--) {
    for (uint16_t col=0; col<GRAINSWIDE; col++) {
      //Check if we should be dropping this grain
      if (getSand(col,row, glassbuffer)) continue;  //Don't move cells that make up the hourglass itself
      if (getSand(col,row, framebuffer )) {
        if ((getSand(col,row+1, framebuffer ) == 0) && (notTouchingGlass(col,row+1,glassbuffer))) {
          setSand(col,row,0, framebuffer); setSand(col,row+1,1, framebuffer); continue;
        }
        //Toggle alternates directions checked first, otherwise operations are the same
        if (toggle) {
          toggle=0;
          if ((col > 0) && (getSand(col-1,row+1, framebuffer) == 0) && (notTouchingGlass(col-1,row+1,glassbuffer))) {
            setSand(col,row,0, framebuffer); setSand(col-1,row+1,1, framebuffer); continue;
          }
          
          if ((col < (GRAINSWIDE-1)) && (getSand(col+1,row+1, framebuffer) == 0) && (notTouchingGlass(col+1,row+1,glassbuffer))) {
            setSand(col,row,0, framebuffer); setSand(col+1,row+1,1, framebuffer);
          }
        }
        else {
          ++toggle;       
          if ((col < (GRAINSWIDE-1)) && (getSand(col+1,row+1, framebuffer) == 0) && (notTouchingGlass(col+1,row+1,glassbuffer))) {
            setSand(col,row,0, framebuffer); setSand(col+1,row+1,1, framebuffer);
          }
          if ((col > 0) && (getSand(col-1,row+1, framebuffer) == 0) && (notTouchingGlass(col-1,row+1,glassbuffer))) {
            setSand(col,row,0, framebuffer); setSand(col-1,row+1,1, framebuffer); continue;
          }
        }
      }
    }
  }
}

void reverseSand(uint8_t framebuffer[(GRAINSWIDE/8)*GRAINSDEEP], uint8_t glassbuffer[(GRAINSWIDE/8)*GRAINSDEEP]) {  
  /* if cell below is empty, drop */
  for (int16_t row=1; row<GRAINSDEEP; row++) {
    for (int16_t col=GRAINSWIDE-1; col>=0; col--) {
      //Check if we should be dropping this grain
      if (getSand(col,row, glassbuffer)) continue;  //Don't move cells that make up the hourglass itself
      if (getSand(col,row, framebuffer)) {
        if ((getSand(col,row-1, framebuffer) == 0) && (notTouchingGlass(col,row-1,glassbuffer))) {
          setSand(col,row,0, framebuffer); setSand(col,row-1,1, framebuffer); continue;
        }
        //Toggle alternates directions checked first, otherwise operations are the same
        if (toggle) {
          toggle = 0;
          if ((col > 0) && (getSand(col-1,row-1, framebuffer) == 0) && (notTouchingGlass(col-1,row-1,glassbuffer))){
            setSand(col,row,0, framebuffer); setSand(col-1,row-1,1, framebuffer); continue;
          }
          if ((col < (GRAINSWIDE-1)) && (getSand(col+1,row-1, framebuffer) == 0) && (notTouchingGlass(col+1,row-1,glassbuffer))) {
            setSand(col,row,0, framebuffer); setSand(col+1,row-1,1, framebuffer);
          }
        }
        else {
          ++toggle;
          if ((col < (GRAINSWIDE-1)) && (getSand(col+1,row-1, framebuffer) == 0) && (notTouchingGlass(col+1,row-1,glassbuffer))) {
            setSand(col,row,0, framebuffer); setSand(col+1,row-1,1, framebuffer);
          }
          if ((col > 0) && (getSand(col-1,row-1, framebuffer) == 0) && (notTouchingGlass(col-1,row-1,glassbuffer))){
            setSand(col,row,0, framebuffer); setSand(col-1,row-1,1, framebuffer); continue;
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

  Wire.begin();
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B);  // PWR_MGMT_1 register
  Wire.write(0);     // set to zero (wakes up the MPU-6050)
  Wire.endTransmission(true);
  
  //Fill with test sand
  for (uint8_t i=6; i<36; i++) {
    for (uint8_t j=24; j<39; j++) {
      setSand(j,i,1,topbuff);
      setSand(j,i+10,1,botbuff);
    }
  }
  
  display.init();
  showBuf();
  display.display();  
}

// the loop routine runs over and over again forever:
void loop() {
  static int nexttime = millis();
  static int nextframe = millis() + 10;
  static int counter = 0;
  static uint8_t gravity = 1;
  if (millis() > nexttime) {
    ++counter;
    /*
    //used to reverse gravitiy after a while for testing
    if (counter++ < 150) setSand(32,0,1,botbuff);
    else setSand(32,0,0,botbuff);
    */
    //setSand(32,6,1,topbuff);

    Wire.beginTransmission(MPU_addr);
    Wire.write(0x3D);  // starting with register 0x3D (ACCEL_YOUT_H)
    Wire.endTransmission(false);
    Wire.requestFrom(MPU_addr,2,true);  // request a total of 14 registers   
    AcY=Wire.read()<<8|Wire.read();  // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)

    if (AcY > 1000) {
      digitalWrite(led,1);
      gravity = 0;
    }
    if (AcY < -1000) {
      digitalWrite(led,0);
      gravity = 1;
    }
  
    if (gravity) {
      if (getSand(32,63,topbuff)) {
        setSand(32,63,0,topbuff);
        setSand(32,0,1,botbuff); 
      }
    }
    else {
      if (getSand(32,0,botbuff)) {
        setSand(32,0,0,botbuff);
        setSand(32,63,1,topbuff); 
      }
    }
    showBuf();

    display.display();
    nexttime = millis()+300;
    //if (digitalRead(led)) digitalWrite(led,LOW);
    //else digitalWrite(led,HIGH);
  }

  if (millis() > nextframe) {
    /*
    //used to reverse gravitiy after a while for testing
    if ((counter < 150) || (counter > 200)) moveSand(botbuff,hourglassbot);
    else reverseSand(botbuff,hourglassbot);
    */

    /*
    if (counter == 70) {
      if (gravity++) gravity=0;
      counter = 0;
    }
    */
    if (gravity) {
      moveSand(topbuff,hourglasstop);
      moveSand(botbuff,hourglassbot);
    }
    else {
      reverseSand(botbuff,hourglassbot);
      reverseSand(topbuff,hourglasstop);
    }
    showBuf();
    display.display();
    nextframe = millis()+10;
  }
}

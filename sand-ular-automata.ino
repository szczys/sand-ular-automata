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
#define BUFSIZE     (GRAINSWIDE/8)*GRAINSDEEP

// ESP-WROOM-32 has LED on pin 2:
int led = 2;

uint8_t botbuff  [BUFSIZE];
uint8_t topbuff [BUFSIZE];
uint8_t toggle;

// I2C version
//SSD1306 display(0x3c, 18, 19);
SH1106Wire display(0x3c, 21, 22);

void clearBuff(void);
void showBuf(void);
uint8_t getSand(uint16_t x, uint16_t y, uint8_t framebuffer[BUFSIZE]);


void clearBuff(void) {
  for (uint16_t i=0; i<(BUFSIZE); i++) {
    botbuff  [i] = hourglassbot[i];
    topbuff [i] = hourglasstop[i];
  }
}

void showBuf(void) {
  display.clear(); //drawFastImage doesn't draw black pixels so clear first

  /*
  //Keep track of grains of sand to make sure we're not losing or creating any by accident
  //This count includes 1466 of the hourglass frame itself
  uint16_t sandcount = 0;
  for (uint8_t i=0; i<64; i++) {
    for (uint8_t j=0; j<64; j++) {
      if (getSand(j,i,botbuff  )) ++sandcount;
      if (getSand(j,i,topbuff  )) ++sandcount;
    }
  }
  char sbuf[20];
  itoa(sandcount,sbuf,10);
  display.drawString(50, 0, sbuf);
  */
  
  display.drawFastImage(64, 0, GRAINSWIDE, GRAINSDEEP, botbuff);
  display.drawFastImage(0, 0, GRAINSWIDE, GRAINSDEEP, topbuff);
}

uint8_t getSand(uint16_t x, uint16_t y, uint8_t framebuffer[BUFSIZE]) {
  uint16_t byteIdx = y*(GRAINSDEEP/8);
  uint16_t byteOffset = x/8;
  uint16_t byteLoc = x%8;

  if (framebuffer[byteIdx+byteOffset] & (1<<byteLoc)) return 1;
  else return 0;  
}

void setSand(uint16_t x, uint16_t y, uint8_t onoff, uint8_t framebuffer[BUFSIZE]) {
  uint16_t byteIdx = y*(GRAINSDEEP/8);
  uint16_t byteOffset = (x/8);
  uint16_t byteLoc = x%8;

  if (onoff > 0) { framebuffer[byteIdx+byteOffset] |= (1<<byteLoc); }
  else { framebuffer[byteIdx+byteOffset] &= ~(1<<byteLoc); }
}

uint8_t notTouchingGlass(uint16_t x, uint16_t y, uint8_t glassbuffer[BUFSIZE]) {
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

void moveN(uint16_t x, uint16_t y, uint8_t framebuffer[BUFSIZE]) {
  setSand(x,y,0, framebuffer); setSand(x,y-1,1, framebuffer);
}

void moveNW(uint16_t x, uint16_t y, uint8_t framebuffer[BUFSIZE]) {
  setSand(x,y,0, framebuffer); setSand(x-1,y-1,1, framebuffer);
}

void moveNE(uint16_t x, uint16_t y, uint8_t framebuffer[BUFSIZE]) {
  setSand(x,y,0, framebuffer); setSand(x+1,y-1,1, framebuffer);
}

void moveS(uint16_t x, uint16_t y, uint8_t framebuffer[BUFSIZE]) {
  setSand(x,y,0, framebuffer); setSand(x,y+1,1, framebuffer);
}

void moveSW(uint16_t x, uint16_t y, uint8_t framebuffer[BUFSIZE]) {
  setSand(x,y,0, framebuffer); setSand(x-1,y+1,1, framebuffer);
}

void moveSE(uint16_t x, uint16_t y, uint8_t framebuffer[BUFSIZE]) {
  setSand(x,y,0, framebuffer); setSand(x+1,y+1,1, framebuffer);
}

void moveW(uint16_t x, uint16_t y, uint8_t framebuffer[BUFSIZE]) {
  setSand(x,y,0, framebuffer); setSand(x-1,y,1, framebuffer);
}

void moveE(uint16_t x, uint16_t y, uint8_t framebuffer[BUFSIZE]) {
  setSand(x,y,0, framebuffer); setSand(x+1,y,1, framebuffer);
}

/*
 * Cellular automata scheme:
 * 
 * if cell below is empty, drop
 * if cell below is full but below to the left is empty, fall there, otherwise fall sell below to the right
 * now that individual cells have fallen, check row issues:
 *   if row above is entirely full, and this row has empty spaces near the edges, move one grain toward that empty space
 */
void driftSouth(uint8_t framebuffer[BUFSIZE], uint8_t glassbuffer[BUFSIZE]) {  
  /* if cell below is empty, drop */
  for (int16_t row=GRAINSDEEP-2; row>=0; row--) {
    for (uint16_t col=0; col<GRAINSWIDE; col++) {
      //Check if we should be dropping this grain
      if (getSand(col,row, glassbuffer)) continue;  //Don't move cells that make up the hourglass itself
      if (getSand(col,row, framebuffer )) {
        if ((getSand(col,row+1, framebuffer ) == 0) && (notTouchingGlass(col,row+1,glassbuffer))) {
          moveS(col,row,framebuffer); continue;
        }
        //Toggle alternates directions checked first, otherwise operations are the same
        if (toggle) {
          toggle=0;
          if ((col > 0) && (getSand(col-1,row+1, framebuffer) == 0) && (notTouchingGlass(col-1,row+1,glassbuffer))) {
            moveSW(col,row,framebuffer); continue;
          }
          
          if ((col < (GRAINSWIDE-1)) && (getSand(col+1,row+1, framebuffer) == 0) && (notTouchingGlass(col+1,row+1,glassbuffer))) {
            moveSE(col,row,framebuffer); continue;
          }
        }
        else {
          ++toggle;       
          if ((col < (GRAINSWIDE-1)) && (getSand(col+1,row+1, framebuffer) == 0) && (notTouchingGlass(col+1,row+1,glassbuffer))) {
            moveSE(col,row,framebuffer); continue;
          }
          if ((col > 0) && (getSand(col-1,row+1, framebuffer) == 0) && (notTouchingGlass(col-1,row+1,glassbuffer))) {
            moveSW(col,row,framebuffer); continue;
          }
        }
      }
    }
  }
}

void driftNorth(uint8_t framebuffer[BUFSIZE], uint8_t glassbuffer[BUFSIZE]) {  
  /* if cell below is empty, drop */
  for (int16_t row=1; row<GRAINSDEEP; row++) {
    for (int16_t col=GRAINSWIDE-1; col>=0; col--) {
      //Check if we should be dropping this grain
      if (getSand(col,row, glassbuffer)) continue;  //Don't move cells that make up the hourglass itself
      if (getSand(col,row, framebuffer)) {
        if ((getSand(col,row-1, framebuffer) == 0) && (notTouchingGlass(col,row-1,glassbuffer))) {
          moveN(col,row,framebuffer); continue;
        }
        //Toggle alternates directions checked first, otherwise operations are the same
        if (toggle) {
          toggle = 0;
          if ((col > 0) && (getSand(col-1,row-1, framebuffer) == 0) && (notTouchingGlass(col-1,row-1,glassbuffer))){
            moveNW(col,row,framebuffer); continue;
          }
          if ((col < (GRAINSWIDE-1)) && (getSand(col+1,row-1, framebuffer) == 0) && (notTouchingGlass(col+1,row-1,glassbuffer))) {
            moveNE(col,row,framebuffer); continue;
          }
        }
        else {
          ++toggle;
          if ((col < (GRAINSWIDE-1)) && (getSand(col+1,row-1, framebuffer) == 0) && (notTouchingGlass(col+1,row-1,glassbuffer))) {
            moveNE(col,row,framebuffer); continue;
          }
          if ((col > 0) && (getSand(col-1,row-1, framebuffer) == 0) && (notTouchingGlass(col-1,row-1,glassbuffer))){
            moveNW(col,row,framebuffer); continue;
          }
        }
      }
    }
  }
}

void driftWest(uint8_t framebuffer[BUFSIZE], uint8_t glassbuffer[BUFSIZE]) {  
  /* if cell below is empty, drop */
  for (int16_t col=1; col<GRAINSWIDE; col++) {
    for (int16_t row=GRAINSDEEP-1; row>=0; row--) {
      //Check if we should be dropping this grain
      if (getSand(col,row, glassbuffer)) continue;  //Don't move cells that make up the hourglass itself
      if (getSand(col,row, framebuffer)) {
        if ((getSand(col-1,row, framebuffer) == 0) && (notTouchingGlass(col-1,row,glassbuffer))) {
          moveW(col,row,framebuffer); continue;
        }
        //Toggle alternates directions checked first, otherwise operations are the same
        if (toggle) {
          toggle = 0;
          if ((row > 0) && (getSand(col-1,row-1, framebuffer) == 0) && (notTouchingGlass(col-1,row-1,glassbuffer))){
            moveNW(col,row,framebuffer); continue;
          }
          if ((row < (GRAINSDEEP-1)) && (getSand(col-1,row+1, framebuffer) == 0) && (notTouchingGlass(col-1,row+1,glassbuffer))) {
            moveSW(col,row,framebuffer); continue;
          }
        }
        else {
          ++toggle;
          if ((row < (GRAINSDEEP-1)) && (getSand(col-1,row+1, framebuffer) == 0) && (notTouchingGlass(col-1,row+1,glassbuffer))) {
            moveSW(col,row,framebuffer); continue;
          }
          if ((row > 0) && (getSand(col-1,row-1, framebuffer) == 0) && (notTouchingGlass(col-1,row-1,glassbuffer))){
            moveNW(col,row,framebuffer); continue;
          }
        }
      }
    }
  }
}

void driftEast(uint8_t framebuffer[BUFSIZE], uint8_t glassbuffer[BUFSIZE]) {  
  /* if cell below is empty, drop */
  for (int16_t col=GRAINSWIDE-2; col>=0; col--) {
    for (uint16_t row=0; row<GRAINSDEEP; row++) {
      //Check if we should be dropping this grain
      if (getSand(col,row, glassbuffer)) continue;  //Don't move cells that make up the hourglass itself
      if (getSand(col,row, framebuffer )) {
        if ((getSand(col+1,row, framebuffer ) == 0) && (notTouchingGlass(col+1,row,glassbuffer))) {
          moveE(col,row,framebuffer); continue;
        }
        //Toggle alternates directions checked first, otherwise operations are the same
        if (toggle) {
          toggle=0;
          if ((row > 0) && (getSand(col+1,row-1, framebuffer) == 0) && (notTouchingGlass(col+1,row-1,glassbuffer))) {
            moveNE(col,row,framebuffer); continue;
          }
          
          if ((row < (GRAINSDEEP-1)) && (getSand(col+1,row+1, framebuffer) == 0) && (notTouchingGlass(col+1,row+1,glassbuffer))) {
            moveSE(col,row,framebuffer); continue;
          }
        }
        else {
          ++toggle;       
          if ((row < (GRAINSDEEP-1)) && (getSand(col+1,row+1, framebuffer) == 0) && (notTouchingGlass(col+1,row+1,glassbuffer))) {
            moveSE(col,row,framebuffer); continue;
          }
          if ((row > 0) && (getSand(col+1,row-1, framebuffer) == 0) && (notTouchingGlass(col+1,row-1,glassbuffer))) {
            moveNE(col,row,framebuffer); continue;
          }
        }
      }
    }
  }
}

void bathtubSand(uint16_t x, uint16_t y, int8_t dir, uint8_t framebuffer[BUFSIZE], uint8_t glassbuffer[BUFSIZE]) {
  if (dir < 0) {
    while (y>0) {
      if (getSand(x,y-1,glassbuffer)) return; //We've hit glass
      if (getSand(x,y-1,framebuffer) == 0) return; //Air above us, let normal rules sort this out
      moveS(x,y-1,framebuffer);
      --y;
    }
  }
  else {
    while (y<(GRAINSDEEP-1)) {
      if (getSand(x,y+1,glassbuffer)) return; //We've hit glass
      if (getSand(x,y+1,framebuffer) == 0) return; //Air above us, let normal rules sort this out
      moveN(x,y+1,framebuffer);
      ++y;
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
  for (uint8_t i=6; i<46; i++) {
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
  static int8_t gravity = 1;
  static int8_t weakengravity = 0;
  static int8_t tilt = 0;
  static int8_t weakentilt = 0;
  if (millis() > nexttime) {
    ++counter;
    /*
    //used to reverse gravitiy after a while for testing
    if (counter++ < 150) setSand(32,0,1,botbuff);
    else setSand(32,0,0,botbuff);
    */
    //setSand(32,6,1,topbuff);

    Wire.beginTransmission(MPU_addr);
    Wire.write(0x3B);  // starting with register 0x3B (ACCEL_XOUT_H)
    Wire.endTransmission(false);
    Wire.requestFrom(MPU_addr,4,true);  // request a total of 14 registers   
    AcX=Wire.read()<<8|Wire.read();  // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
    AcY=Wire.read()<<8|Wire.read();  // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)

    if (AcX < -2000) {
      //digitalWrite(led,1);
      gravity = -1;
    }
    else if (AcX > 2000) {
      //digitalWrite(led,0);
      gravity = 1;
    }
    else gravity = 0;

    if (AcY < -2000) {
      digitalWrite(led,1);
      tilt = -1;
    }
    else if (AcY > 2000) {
      digitalWrite(led,0);
      tilt = 1;
    }
    else tilt = 0;

    //Move one grain between top/bottom if necessary:
    if (gravity==1) {
      if (getSand(32,63,topbuff) && (getSand(32,0,botbuff) == 0)) {
        setSand(32,63,0,topbuff); //Erase grain
        bathtubSand(32,63,-1,topbuff,hourglasstop); //Drop all grains above this to simulate bathtub effect
        setSand(32,0,1,botbuff); //Spawn grain in otherside of bottleneck
      }
    }
    if (gravity==-1) {
      if (getSand(32,0,botbuff) && (getSand(32,63,topbuff) == 0)) {
        setSand(32,0,0,botbuff);
        bathtubSand(32,0,1,botbuff,hourglassbot);
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
    if ((counter < 150) || (counter > 200)) driftSouth(botbuff,hourglassbot);
    else driftNorth(botbuff,hourglassbot);
    */

    /*
    if (counter == 70) {
      if (gravity++) gravity=0;
      counter = 0;
    }
    */

        
    if (weakentilt-- == 0) {
      //Reset weakentilt to skip some frames if tilt is not very extreme
      // 0==+/-16,000 1==+/-13,000 2==+/-10,000 3==+/-7,000 4==+/-4,000  
      if ((AcY > -7000) && (AcY < 7000)) weakentilt = 3;
      else if ((AcY > -10000) && (AcY < 10000)) weakentilt = 2;
      else if ((AcY > -13000) && (AcY < 13000)) weakentilt = 1;
      else weakentilt = 0;
      
      if (tilt==1) {
        driftEast(topbuff,hourglasstop);
        driftEast(botbuff,hourglassbot);
      }
      
      if (tilt==-1) {
        driftWest(botbuff,hourglassbot);
        driftWest(topbuff,hourglasstop);
      }
    }

    if (weakengravity-- == 0) {
      //Reset weakentilt to skip some frames if tilt is not very extreme
      // 0==+/-16,000 1==+/-13,000 2==+/-10,000 3==+/-7,000  
      if ((AcX > -7000) && (AcX < 7000)) weakengravity = 3;
      else if ((AcX > -10000) && (AcX < 10000)) weakengravity = 2;
      else if ((AcX > -13000) && (AcX < 13000)) weakengravity = 1;
      else weakengravity = 0;

      if (gravity==1) {
        driftSouth(topbuff,hourglasstop);
        driftSouth(botbuff,hourglassbot);
      }
      if (gravity==-1) {
        driftNorth(botbuff,hourglassbot);
        driftNorth(topbuff,hourglasstop);
      }
    }
    
    showBuf();
    display.display();
    nextframe = millis()+10;
  }
}

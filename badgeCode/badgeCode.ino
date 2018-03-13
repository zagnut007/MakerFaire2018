/*********************************************************************
New Orleans Mini Maker Faire 2018 VIP Badge Example Sketch
https://github.com/zagnut007/MakerFaire2018

Author: Kevin Blank

The follow sketch is designed to work with the components supplied
in the 2018 VIP Badge Kit.

Learn more about the NOLA Mini Maker Faire at http://neworleans.makerfaire.com/
Requires:
https://github.com/adafruit/Adafruit-GFX-Library
https://github.com/adafruit/Adafruit_SSD1306
Thank you to Adafruit for providing these libraries!
https://www.adafruit.com/
*********************************************************************/

#include <SPI.h>
#include <Wire.h>
#include <math.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "pitches.h"
#include "bitmap_robot.h"

// If using software SPI (the default case):
#define OLED_MOSI  14  // SDA pin on OLED
#define OLED_CLK   15  // SCL pin on OLED
#define OLED_DC    10  // D/C pin on OLED
#define OLED_RESET 16  // RST pin on OLED
Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, 12);

#define LED_PIN 3

#define AUDIO_PIN 18

#define BUTTON_LEFT 7
#define BUTTON_RIGHT 6

// CHANGE ME - Put your name in the constant below.
#define MYNAME "Kevin"

#define DISPLAY_HEIGHT 64
#define DISPLAY_WIDTH 128

#define LOGO32_GLCD_HEIGHT 32 
#define LOGO32_GLCD_WIDTH  32

int rightButtonState = 0;
int leftButtonState = 0;

unsigned long lastInterrupt;
#define PROGRAM_NAME 0
#define PROGRAM_GAME 1
volatile byte programState = PROGRAM_NAME;

void setup() {
  // put your setup code here, to run once:
  pinMode(LED_PIN, OUTPUT);
  pinMode(AUDIO_PIN, OUTPUT);
  pinMode(BUTTON_LEFT, INPUT);
  pinMode(BUTTON_RIGHT, INPUT);

  Serial.begin(9600);

  // Required to set up the display
  display.begin(SSD1306_SWITCHCAPVCC);
  
  // Clear the buffer/memory
  display.clearDisplay();

  // Initial "boot" screen
  vipSplash(logo32_glcd_bmp, LOGO32_GLCD_HEIGHT, LOGO32_GLCD_WIDTH);

  // Attaching interrupt on the left button/pin 7 to switch between modes/programs
  attachInterrupt(digitalPinToInterrupt(BUTTON_LEFT), switch_program, RISING);
}

void loop() {
  // Checking for interrupt flag
  if(programState == PROGRAM_NAME){
    display.clearDisplay();
    display.display();
    digitalWrite(LED_PIN, LOW);
    splash_loop();
  }
  else{
    display.clearDisplay();
    display.display();
    digitalWrite(LED_PIN, HIGH);
    game_loop();
  }
}

unsigned long lastUpdate;
int Interval = 25; // Step interval in ms
/****************************************************************************************
 * Splash loop
 */

void splash_loop(){
  int countUntilShowName = 50;
  int currentCount = 0;
  
  while(programState == PROGRAM_NAME){
    if((millis() - lastUpdate) > Interval){
      lastUpdate = millis();

      if(currentCount < countUntilShowName){
        vipSplash(logo32_glcd_bmp, LOGO32_GLCD_HEIGHT, LOGO32_GLCD_WIDTH);
      }
      else{
        nameSplash();
      }
      currentCount = currentCount + 1;

      if(currentCount == (countUntilShowName * 2)){
        currentCount = 0;
      }

      rightButtonState = digitalRead(BUTTON_RIGHT);
      if(rightButtonState == HIGH){
        billieJean();
      }
    }
  }
  
}

/****************************************************************************************
 * Game Functions
 */
int jumpArcFactor = 2.5; // The bigger the number, the slower the jump
int spriteRobotRestingY = LOGO32_GLCD_HEIGHT; // In resting position, where is the sprite
int spriteRobotRestingX = 8;

int collisionPad = 2; // padding pixels for collision
float forwardFactor = 0.75;
int jumpCount = 0;

void game_loop(){
  // The following variables change during the loop, so reset at the top  
  int spriteRobotX = 8;
  float spriteRobotXtemp = spriteRobotX;
  int spriteRobotY = spriteRobotRestingY; // Robot starts at the bottom of the screen
  int jumpDirection = 0; // 0 means jumping up, 1 means jumping down
  int jumpStatus = 0; // 0 means not current in a jump, 1 means currently jumping
  int jumpStep = 0;

  String currentObj = "resistor";
  int spriteObjWidth = 16;
  int spriteObjHeight = 8;
  int spriteObjX = DISPLAY_WIDTH;
  int spriteObjY = DISPLAY_HEIGHT - spriteObjHeight;
  int objSpeed = 4;

  int isCollision = 0;
  int isSpriteOnField = 0;
  
  while(programState == PROGRAM_GAME){
    randomSeed(millis());
    if((millis() - lastUpdate) > Interval){
      lastUpdate = millis();

      // Check button state
      rightButtonState = digitalRead(BUTTON_RIGHT);
      if(rightButtonState == HIGH && jumpStatus == 0){
        jumpStatus = 1;
        beep(NOTE_B2, 50);
      }

      // Calculate jump position (if applicable) of Robot Sprite
      if(jumpStatus == 1){
        jumpStep = round(spriteRobotY/jumpArcFactor);
        if(jumpDirection == 0){ // Headed Up
          spriteRobotY = spriteRobotY - jumpStep;
          spriteRobotXtemp = spriteRobotXtemp + forwardFactor;
          spriteRobotX = round(spriteRobotXtemp);
          if(jumpStep < jumpArcFactor || spriteRobotY < 0){ // Let's get him going in the other direction
            jumpDirection = 1;
            spriteRobotY = jumpArcFactor;            
          }
        }
        if(jumpDirection == 1){ // Headed Down
          spriteRobotY = spriteRobotY + jumpStep;
          spriteRobotXtemp = spriteRobotXtemp + forwardFactor;
          spriteRobotX = round(spriteRobotXtemp);
          if(spriteRobotY > spriteRobotRestingY){ // Don't let him go below the bottom
            spriteRobotY = spriteRobotRestingY;
          }
        }
      }
      if(jumpStatus == 1 && spriteRobotY == spriteRobotRestingY){ // This is how we detect the end of the jump
        jumpStatus = 0;
        jumpDirection = 0;
      }
      if(jumpStatus == 0 && spriteRobotX > spriteRobotRestingX){
        spriteRobotX = spriteRobotX - 1;
        spriteRobotXtemp = spriteRobotX;
      }

      // Determine if we should show a sprite
      long randNumber;
      if(isSpriteOnField == 0){
        Serial.write("New obj\n");
        jumpCount = jumpCount + 1;
        randNumber = random(200);
        
        if(randNumber < 100){ // Use resistor sprite
          currentObj = "resistor";
          spriteObjWidth = 16;
          spriteObjHeight = 8;
        }
        else{ // Use nut sprite
          currentObj = "nut";
          spriteObjWidth = 8;
          spriteObjHeight = 8;
        }
        isSpriteOnField = 1;
        spriteObjX = DISPLAY_WIDTH;

        randNumber = random(100);
        spriteObjY = DISPLAY_HEIGHT - spriteObjHeight;
        if(randNumber < 20){ // Randomly determine if on top or bottom of screen
          spriteObjY = 1+spriteObjHeight;
        }
      }

      // Change speed based on score, this is where we make things more difficult
      if(jumpCount < 15){
        objSpeed = 3;
      }
      if(jumpCount >= 15 && jumpCount < 30){
        objSpeed = 4;
      }
      if(jumpCount > 30 && jumpCount < 45){
        objSpeed = 5;
      }
      if(jumpCount >= 45){
        objSpeed = 7;
      }

      // Calculate position of object sprite
      if(isSpriteOnField == 1){
        spriteObjX = spriteObjX - objSpeed;
        if(spriteObjX <= 0){
          isSpriteOnField = 0;
          isCollision = 0;
          
        }
      }

      // Check for collision
      int rightSideRobot = spriteRobotX + LOGO32_GLCD_WIDTH - collisionPad;
      int bottomRobot = spriteRobotY + LOGO32_GLCD_HEIGHT - collisionPad;
      int topRobot = spriteRobotY + collisionPad;
      int leftSideObj = spriteObjX + collisionPad;
      int topObj = spriteObjY + collisionPad;
      int bottomObj = spriteObjY + spriteObjHeight - collisionPad;
      if(
          (rightSideRobot >= leftSideObj) &&
            (
              (topRobot <= bottomObj && topRobot > topObj) ||
              (bottomRobot >= topObj && bottomRobot <= bottomObj)  
            ) &&
          isCollision == 0
          && leftSideObj > 16 // padding here so if we land close, doesn't trigger
        ){
          isCollision = 1;
          beep(NOTE_G3, 25);
          jumpCount = 0;
        }

      // Draw sprites
      display.clearDisplay();
      display.drawBitmap(spriteRobotX, spriteRobotY, logo32_glcd_bmp, LOGO32_GLCD_WIDTH, LOGO32_GLCD_HEIGHT, 1);
      if(isSpriteOnField == 1){
        if(currentObj == "resistor"){
          display.drawBitmap(spriteObjX, spriteObjY, sprite_resistor_bmp, spriteObjWidth, spriteObjHeight, 1);
        }
        if(currentObj == "nut"){
          display.drawBitmap(spriteObjX, spriteObjY, sprite_nut_bmp, spriteObjWidth, spriteObjHeight, 1);
        }
      }
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.setCursor(100,30);
      display.println(jumpCount);
      display.display();
      
    }    
  }
}

/****************************************************************************************
 * Utility functions
 */

void switch_program(){
  /*
   * Because when a simple hardware button is pressed, the state can change rapidly in the small amount of time during the press
   * we're adding a condition that ignores the button press for a window of time
   */
  leftButtonState = digitalRead(BUTTON_LEFT);
  if(millis() - lastInterrupt > 300 && leftButtonState == HIGH){ // we set a 300ms no-interrupts window
    lastInterrupt = millis();

    if(programState == PROGRAM_NAME){
      programState = PROGRAM_GAME;
    }
    else{
      programState = PROGRAM_NAME;
    }
    
  }
}

void billieJean(){
  beep(NOTE_FS4, 500);
  beep(NOTE_CS4, 500);
  beep(NOTE_E4, 500);
  beep(NOTE_FS4, 500);
  beep(NOTE_E4, 500);
  beep(NOTE_CS4, 500);
  beep(NOTE_B3, 500);
  beep(NOTE_CS4, 500);
}

void beep(int pitch, unsigned char delayms){
  tone(AUDIO_PIN, pitch, delayms);
  delay(delayms);          // wait for a delayms ms
  noTone(AUDIO_PIN);
  //delay(delayms);          // wait for a delayms ms   
}

void vipSplash(const uint8_t *bitmap, uint8_t w, uint8_t h){
  int margin = 50;
  
  display.clearDisplay();
  display.drawBitmap(4, 4, bitmap, w, h , 1);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(margin,4);
  display.println("NOLA Mini");
  display.setCursor(margin,12);
  display.println("Maker Faire");
  display.setTextSize(2);
  display.setCursor(margin,24);
  display.println("VIP");
  display.setTextSize(2);
  display.setCursor(margin,40);
  display.println("2018");
  display.display();
  
}

void nameSplash(){
  int margin = 10;
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(margin,10);
  display.println("My Name Is:");
  display.setTextSize(2);
  display.setCursor(margin,24);
  display.println(MYNAME);
  display.display();
  
}

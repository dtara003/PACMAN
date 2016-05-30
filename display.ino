#include <Adafruit_GFX.h>   // Core graphics library
#include <RGBmatrixPanel.h> // Hardware-specific library

#define CLK 8  // MUST be on PORTB! (Use pin 11 on Mega)
#define OE  9
#define LAT 10
#define A   A0
#define B   A1
#define C   A2
#define D   A3

RGBmatrixPanel matrix(A, B, C, D, CLK, LAT, OE, false);

#define PAC matrix.Color333(7, 7, 0)
#define GREEN matrix.Color333(1, 7, 0)
#define PURPLE matrix.Color333(7, 0, 7)
#define SKY matrix.Color333(0, 7, 7)
#define BLUE matrix.Color333(0, 0, 7)
#define BLACK matrix.Color333(0, 0, 0)
#define RED matrix.Color333(7, 0, 0)

#define CURR matrix.drawRect(xTL, yTL, 2, 2, PAC)
#define PREV matrix.drawRect(xTL, yTL, 2, 2, BLACK)

void setup() {
  matrix.begin();
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println("Setup");
  //levelOne();
  printPACMAN();
  printImage();
  
  //matrix.drawRect(15, 24, 2, 2, PAC);
  /*matrix.drawRect(15, 12, 2, 2, GREEN);
  matrix.drawRect(13, 15, 2, 2, SKY);
  matrix.drawRect(17, 15, 2, 2, PURPLE);*/
}

//signed char incomingByte = 0;
unsigned char xTL = 15;
unsigned char yTL = 24;
unsigned short SIGNAL = 0; // previously 'val'

unsigned char control = 0x00;
/*unsigned char g5 = 0x00;
unsigned char g4 = 0x00;
unsigned char g3 = 0x00;
unsigned char g2 = 0x00;
unsigned char g1 = 0x00;*/
unsigned char pacman = 0x00;
unsigned char prevControl = 0xFF;

void loop() {
 if (Serial.available() >= 2) {
                SIGNAL = (Serial.read() << 8);
                SIGNAL |= Serial.read();

                control = (SIGNAL >> 13);
                pacman = SIGNAL & 0x0007;
                //Serial.println(SIGNAL);

                if (control != prevControl) {
                        if (control == 1) {
                            matrix.fillScreen(GREEN);
                            matrix.fillRect(3, 3, 26, 26, BLACK);
                            xTL = 15; yTL = 24;
                        } else if (control == 2) {
                            matrix.fillScreen(RED);
                            matrix.fillRect(3, 3, 26, 26, BLACK);
                            xTL = 15; yTL = 24;
                        } else if (control == 3) {
                            matrix.fillScreen(GREEN);
                            matrix.fillRect(3, 3, 26, 26, BLACK);
                            xTL = 15; yTL = 24;
                        } else if (control == 4) {
                            matrix.fillScreen(BLACK);
                            levelOne();
                            matrix.drawRect(15, 24, 2, 2, PAC);
                            xTL = 15; yTL = 24;
                        } else if (control == 5) {
                            matrix.fillScreen(BLACK);
                            //levelTwo();
                            //xTL = 15; yTL = 24;
                        } else if (control == 6) {
                            matrix.fillScreen(BLACK);
                            //levelThree();
                            //xTL = 15; yTL = 24;
                        } else if (control == 7) {
                            matrix.fillScreen(BLACK);
                            printPACMAN(); printImage();
                            xTL = 15; yTL = 24;
                        }
                }
                
                
                if (control == 0x04) {
                    if (pacman == 0) {
                        // do nothing;
                        CURR;
                    } else if (pacman == 1) {
                        PREV;
                        if (xTL <= 0) {
                          xTL = 30;
                        } else {
                          xTL--;
                        }
                        CURR;
                    } else if (pacman == 2) {
                        PREV;
                        if (xTL >= 30) {
                          xTL = 0;
                        } else {
                          xTL++;
                        }
                        CURR;
                    } else if (pacman == 3) {
                        PREV; yTL--; CURR;
                    } else if (pacman == 4) {
                        PREV; yTL++; CURR;
                    }
                }

                prevControl = control;
    }
      
}

void levelOne() {
        // dots
        matrix.fillScreen(RED);
        matrix.fillRect(10, 12, 12, 8, BLACK);
        
        // border walls and inward walls
        matrix.drawRect(0, 0, 32, 32, BLUE);
        matrix.drawRect(1, 1, 30, 30, BLUE);
        matrix.drawRect(15, 2, 2, 4, BLUE);
        matrix.drawRect(15, 26, 2, 4, BLUE);
        matrix.drawRect(26, 13, 4, 2, BLUE);
        matrix.drawRect(2, 13, 4, 2, BLUE);
        matrix.drawRect(26, 17, 4, 2, BLUE);
        matrix.drawRect(2, 17, 4, 2, BLUE);
        matrix.drawRect(0, 15, 10, 2, BLACK);
        matrix.drawRect(22, 15, 10, 2, BLACK);
        //matrix.drawRect(15, 8, 2, 4, matrix.Color333(0, 7, 0));
        //matrix.drawRect(15, 19, 2, 4, matrix.Color333(0, 7, 0));
        // ghostie box
        matrix.drawRect(12, 14, 8, 4, BLUE);
        //matrix.drawRect(13, 15, 6, 2, BLACK);
        matrix.drawLine(15, 14, 16, 14, BLACK);
        // top left extras
        matrix.drawRect(4, 4, 9, 2, BLUE);
        matrix.drawRect(4, 6, 2, 5, BLUE);
        matrix.drawRect(8, 8, 2, 7, BLUE);
        matrix.fillRect(12, 8, 8, 4, BLUE);
        matrix.drawRect(15, 6, 2, 6, BLACK);
        // top right extras
        matrix.drawRect(19, 4, 9, 2, BLUE);
        matrix.drawRect(26, 6, 2, 5, BLUE);
        matrix.drawRect(22, 8, 2, 7, BLUE);
        // bottom left extras
        matrix.drawRect(4, 26, 9, 2, BLUE);
        matrix.drawRect(4, 21, 2, 5, BLUE);
        matrix.drawRect(8, 17, 2, 7, BLUE);
        matrix.fillRect(12, 20, 8, 4, BLUE);
        matrix.drawRect(15, 20, 2, 6, BLACK);
        // bottom right extras
        matrix.drawRect(19, 26, 9, 2, BLUE);
        matrix.drawRect(26, 21, 2, 5, BLUE);
        matrix.drawRect(22, 17, 2, 7, BLUE);
}

void printPACMAN() {
        matrix.drawRect(0, 2, 32, 11, matrix.Color333(2, 7, 0));
        matrix.setCursor(2, 4);
        matrix.setTextSize(1);
        matrix.setTextColor(matrix.Color333(7, 6, 6));
        matrix.print('P');
        matrix.setCursor(7, 4);
        matrix.setTextSize(1);
        matrix.setTextColor(matrix.Color333(7, 6, 6));
        matrix.print('A');
        matrix.setCursor(12, 4);
        matrix.setTextSize(1);
        matrix.setTextColor(matrix.Color333(7, 6, 6));
        matrix.print('C');
        matrix.setCursor(16, 4);
        matrix.setTextSize(1);
        matrix.setTextColor(matrix.Color333(7, 6, 6));
        matrix.print('M');
        matrix.setCursor(21, 4);
        matrix.setTextSize(1);
        matrix.setTextColor(matrix.Color333(7, 6, 6));
        matrix.print('A');
        matrix.setCursor(25, 4);
        matrix.setTextSize(1);
        matrix.setTextColor(matrix.Color333(7, 6, 6));
        matrix.print('N');
}

void printImage() {
        matrix.fillCircle(13, 22, 5, matrix.Color333(7, 7, 0));
        matrix.drawPixel(12, 20, BLUE);
        matrix.drawLine(14, 22, 17, 19, matrix.Color333(0, 0, 0));
        matrix.drawLine(15, 23, 17, 25, matrix.Color333(0, 0, 0));
        matrix.drawLine(17, 20, 15, 22, matrix.Color333(0, 0, 0));
        matrix.drawLine(16, 23, 17, 24, matrix.Color333(0, 0, 0));
        matrix.drawLine(18, 24, 16, 22, matrix.Color333(0, 0, 0));
        matrix.drawLine(15, 23, 17, 25, matrix.Color333(0, 0, 0));
        matrix.drawLine(18, 20, 18, 23, matrix.Color333(0, 0, 0));
        matrix.drawLine(17, 21, 17, 22, matrix.Color333(0, 0, 0));
        matrix.fillCircle(22, 22, 1, matrix.Color333(7, 0, 0));
}

#include <avr/io.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include "usart_1284.h"
#include "timer.h"
#include "adc.h"
#include "bit.h"
#include "task.h"
#include "levels.h"
#include "io.c"

// LEVEL & FLAG

unsigned char LEVELSCOPY[15][31];

void setupArr(unsigned char c) {
	for (unsigned char i = 0; i < 15; ++i) {
		for (unsigned char j = 0; j < 31; ++j) {
			LEVELSCOPY[i][j] = LEVELS[c][i][j];
		}
	}
}

unsigned char LEV = 0;
unsigned char FLAG = 1;

// JOYSTICK SAMPLING

enum directions {LEFT, RIGHT, UP, DOWN, NONE};

// latest stored value for next direction		// reads left and right if 0, up and down if 1
enum directions nextDirection = LEFT;			unsigned char currRead = 0;
// where reading is stored						// thresholds
unsigned short lr = 0;							unsigned short lowThresh = 0x0200;
unsigned short ud = 0;							unsigned short highThresh = 0x0300;

/*
- joystick input gets read
- stores latest reading in 'nextDirection'
- does not erase reading in 'nextDirection' when joystick is idle
*/

int ReadJoystick(int state) {
	if (FLAG) {
		nextDirection = LEFT;
	} else {
		if (!currRead) {
			lr = readLR();
		
			if (lr < lowThresh) {
				nextDirection = LEFT;
			} else if (lr > highThresh) {
				nextDirection = RIGHT;
			}

			currRead = 1;
		} else {
			ud = readUD();
		
			if (ud < lowThresh) {
				nextDirection = UP;
			} else if (ud > highThresh) {
				nextDirection = DOWN;
			}

			currRead = 0;
		}
	}

	return state;
}

// PACMAN POSITION AND ITERATION
enum PacMoveStates {goLeft, pacWait, goRight, goUp, goDown};

// hold pacman current position, defaulted to bottom center at start
unsigned char x = 11; unsigned char y = 15;
// LEFT = 1		RIGHT = 2		UP = 3		DOWN = 4
unsigned char pacSignal = 0x00;

int PacMove(int state) {
	if (FLAG) {
		x = 11; y = 15;
		pacSignal = 0x00;
	} else {
		switch (state) {
			case goLeft:
				if (nextDirection == RIGHT) {
					state = pacWait; break;
				}

				if ((LEVELSCOPY[x - 1][y] != 1) && (nextDirection == UP)) {
					state = pacWait; break;
				}

				if ((LEVELSCOPY[x + 1][y] != 1) && (nextDirection == DOWN)) {
					state = pacWait; break;
				}

				if (pacSignal == 0x01) {
				
					state = goLeft;
					break;
				}
			
				state = pacWait; break;
			case goRight:
				if (nextDirection == LEFT) {
					state = pacWait; break;
				}

				if ((LEVELSCOPY[x - 1][y] != 1) && (nextDirection == UP)) {
					state = pacWait; break;
				}

				if ((LEVELSCOPY[x + 1][y] != 1) && (nextDirection == DOWN)) {
					state = pacWait; break;
				}
			
				if (pacSignal == 0x02) {
				
					state = goRight;
					break;
				}
			
				state = pacWait; break;
			case goUp:
				if (nextDirection == DOWN) {
					state = pacWait; break;
				}

				if ((LEVELSCOPY[x][y - 1] != 1) && (nextDirection == LEFT)) {
					state = pacWait; break;
				}

				if ((LEVELSCOPY[x][y + 1] != 1) && (nextDirection == RIGHT)) {
					state = pacWait; break;
				}

				if (pacSignal == 0x03) {
				
					state = goUp;
					break;
				}
			
				state = pacWait; break;
			case goDown:
				if (nextDirection == UP) {
					state = pacWait; break;
				}

				if ((LEVELSCOPY[x][y - 1] != 1) && (nextDirection == LEFT)) {
					state = pacWait; break;
				}

				if ((LEVELSCOPY[x][y + 1] != 1) && (nextDirection == RIGHT)) {
					state = pacWait; break;
				}

				if (pacSignal == 0x04) {
				
					state = goDown;
					break;
				}

				state = pacWait; break;
			case pacWait:
				if (nextDirection == LEFT) {
					state = goLeft; break;
				} else if (nextDirection == RIGHT) {
					state = goRight; break;
				} else if (nextDirection == UP) {
					state = goUp; break;
				} else if (nextDirection == DOWN) {
					state = goDown; break;
				}
			default:
				state = pacWait; break;
		}

		switch (state) {
			case pacWait:
				pacSignal = 0x00; break;
			case goLeft:
				// if infinite, move to other side of board
				if (y <= 0) {
					y = 30;
					pacSignal = 0x01; break;
				}
				// otherwise not infinite, move left unless wall
				if ( (LEVELSCOPY[x][y - 1] != 1) ) {
					y--;
					pacSignal = 0x01; break;
				}

				if ( (LEVELSCOPY[x][y - 1] == 1) ) {
					pacSignal = 0x00; break;
				}
				break;
			case goRight:
				// if infinite, move to other side of board
				if ( (y >= 30) ) {
					y = 0;
					pacSignal = 0x02; break;
				}
				// otherwise not infinite, move right unless wall
				if ( (LEVELSCOPY[x][y + 1] != 1) ) {
					y++;
					pacSignal = 0x02; break;
				}
			
				if ( (LEVELSCOPY[x][y + 1] == 1) ) {
					pacSignal = 0x00; break;
				}
				break;
			case goUp:
				// not infinite, move up unless wall
				if ( (LEVELSCOPY[x - 1][y] != 1) ) {
					x--;
					pacSignal = 0x03; break;
				}
			
				if ( (LEVELSCOPY[x - 1][y] == 1) ) {
					pacSignal = 0x00; break;
				}
				break;
			case goDown:
				// not infinite, move down unless wall
				if ( (LEVELSCOPY[x + 1][y] != 1) ) {
					x++;
					pacSignal = 0x04; break;
				}

				if ( (LEVELSCOPY[x + 1][y] == 1) ) {
					pacSignal = 0x00; break;
				}
				break;
			default:
				break;
		}
	}

	return state;
}

// GHOST ONE

enum G1States {g1right, g1left, g1up, g1down};
unsigned char G1Signal = 0x00;

// GHOST DIRECTIONS		NONE = 0x00		UP = 0x01		DOWN = 0x02		LEFT = 0x03		RIGHT = 0x04;

unsigned char g1x = 7; unsigned char g1y = 14;
unsigned char G1Flag = 1;
unsigned char prevLocColor1 = 0;

int G1(int state) {
	if (G1Flag) {
		g1x = 7; g1y = 14;
		G1Signal = 0x00; prevLocColor1 = 0;
	} else {
		prevLocColor1 = LEVELSCOPY[g1x][g1y];
	switch (state) {
		case g1up:
			if ((LEVELSCOPY[g1x - 1][g1y] != 1) && (LEVELSCOPY[g1x][g1y - 1] != 1) && (LEVELSCOPY[g1x][g1y + 1] != 1)) state = g1left;
			else if ((LEVELSCOPY[g1x - 1][g1y] != 1) && (LEVELSCOPY[g1x][g1y - 1] != 1)) state = g1up;
			else if ((LEVELSCOPY[g1x - 1][g1y] != 1) && (LEVELSCOPY[g1x][g1y + 1] != 1)) state = g1right;
			else if ((LEVELSCOPY[g1x][g1y - 1] != 1) && (LEVELSCOPY[g1x][g1y + 1] != 1)) state = g1right;
			else if (g1y > 0 && LEVELSCOPY[g1x][g1y - 1] != 1) state = g1left;
			else if (g1y <= 0) state = g1left;
			else if (g1y < 30 && LEVELSCOPY[g1x][g1y + 1] != 1) state = g1right;
			else if (g1y >= 30) state = g1right;
			else if (LEVELSCOPY[g1x - 1][g1y] != 1) state = g1up;
			else if (LEVELSCOPY[g1x + 1][g1y] != 1) state = g1down;
			break;

		case g1down:
			if ((LEVELSCOPY[g1x + 1][g1y] != 1) && (LEVELSCOPY[g1x][g1y - 1] != 1) && (LEVELSCOPY[g1x][g1y + 1] != 1)) state = g1down;
			else if ((LEVELSCOPY[g1x + 1][g1y] != 1) && (LEVELSCOPY[g1x][g1y - 1] != 1)) state = g1down;
			else if ((LEVELSCOPY[g1x + 1][g1y] != 1) && (LEVELSCOPY[g1x][g1y + 1] != 1)) state = g1right;
			else if ((LEVELSCOPY[g1x][g1y - 1] != 1) && (LEVELSCOPY[g1x][g1y + 1] != 1)) state = g1left;
			else if (g1y > 0 && LEVELSCOPY[g1x][g1y - 1] != 1) state = g1left;
			else if (g1y <= 0) state = g1left;
			else if (g1y < 30 && LEVELSCOPY[g1x][g1y + 1] != 1) state = g1right;
			else if (g1y >= 30) state = g1right;
			else if (LEVELSCOPY[g1x + 1][g1y] != 1) state = g1down;
			else if (LEVELSCOPY[g1x - 1][g1y] != 1) state = g1up;
			break;

		case g1right:
			if ((LEVELSCOPY[g1x - 1][g1y] != 1) && (LEVELSCOPY[g1x][g1y + 1] != 1) && (LEVELSCOPY[g1x + 1][g1y] != 1)) state = g1up;
			else if ((LEVELSCOPY[g1x - 1][g1y] != 1) && (LEVELSCOPY[g1x][g1y + 1] != 1)) state = g1up;
			else if ((LEVELSCOPY[g1x + 1][g1y] != 1) && (LEVELSCOPY[g1x][g1y + 1] != 1)) state = g1down;
			else if ((LEVELSCOPY[g1x - 1][g1y] != 1) && (LEVELSCOPY[g1x + 1][g1y] != 1)) state = g1up;
			else if (LEVELSCOPY[g1x + 1][g1y] != 1) state = g1down;
			else if (g1y < 30 && LEVELSCOPY[g1x][g1y + 1] != 1) state = g1right;
			else if (g1y >= 30) state = g1right;
			else if (LEVELSCOPY[g1x - 1][g1y] != 1) state = g1up;
			else if (g1y > 0 && LEVELSCOPY[g1x][g1y - 1] != 1) state = g1left;
			else if (g1y <= 0) state = g1left;
			break;

		case g1left:
			if ((LEVELSCOPY[g1x - 1][g1y] != 1) && (LEVELSCOPY[g1x][g1y - 1] != 1) && (LEVELSCOPY[g1x + 1][g1y] != 1)) state = g1up;
			else if ((LEVELSCOPY[g1x - 1][g1y] != 1) && (LEVELSCOPY[g1x][g1y - 1] != 1)) state = g1left;
			else if ((LEVELSCOPY[g1x + 1][g1y] != 1) && (LEVELSCOPY[g1x][g1y - 1] != 1)) state = g1left;
			else if ((LEVELSCOPY[g1x - 1][g1y] != 1) && (LEVELSCOPY[g1x + 1][g1y] != 1)) state = g1down;
			else if (LEVELSCOPY[g1x + 1][g1y] != 1) state = g1down;
			else if (g1y > 0 && LEVELSCOPY[g1x][g1y - 1] != 1) state = g1left;
			else if (g1y <= 0) state = g1left;
			else if (LEVELSCOPY[g1x - 1][g1y] != 1) state = g1up;
			else if (g1y < 30 && LEVELSCOPY[g1x][g1y + 1] != 1) state = g1right;
			else if (g1y >= 30) state = g1right;
			break;

		default: state = g1up; break;
	}

	switch (state) {
		case g1up:
			g1x--; G1Signal = 0x01; break;
		case g1down:
			g1x++; G1Signal = 0x02; break;
		case g1left:
			if (g1y <= 0) { g1y = 30; G1Signal = 0x03; break;}
			g1y--; G1Signal = 0x03; break;
		case g1right:
			if (g1y >= 30) { g1y = 0; G1Signal = 0x04; break;}
			g1y++; G1Signal = 0x04; break;
		default: break;
	}}

	return state;
	
}

// ------------------------------------------------------------------------------------------------------------------------------------------------------------------

/// GHOST TWO

enum G2States {g2right, g2left, g2up, g2down};
unsigned char G2Signal = 0x00;

// GHOST DIRECTIONS		NONE = 0x00		UP = 0x01		DOWN = 0x02		LEFT = 0x03		RIGHT = 0x04;

unsigned char g2x = 7; unsigned char g2y = 15;
unsigned char G2Flag = 1;
unsigned char prevLocColor2 = 0;

int G2(int state) {
	if (G2Flag) {
		g2x = 7; g2y = 15;
		G2Signal = 0x00; prevLocColor2 = 0;
		} else {
		prevLocColor2 = LEVELSCOPY[g2x][g2y];
		switch (state) {
			case g2up:
			if ((LEVELSCOPY[g2x - 1][g2y] != 1) && (LEVELSCOPY[g2x][g2y - 1] != 1) && (LEVELSCOPY[g2x][g2y + 1] != 1)) state = g2right;
			else if ((LEVELSCOPY[g2x - 1][g2y] != 1) && (LEVELSCOPY[g2x][g2y - 1] != 1)) state = g2left;
			else if ((LEVELSCOPY[g2x - 1][g2y] != 1) && (LEVELSCOPY[g2x][g2y + 1] != 1)) state = g2up;
			else if ((LEVELSCOPY[g2x][g2y - 1] != 1) && (LEVELSCOPY[g2x][g2y + 1] != 1)) state = g2left;
			else if (g2y > 0 && LEVELSCOPY[g2x][g2y - 1] != 1) state = g2left;
			else if (g2y <= 0) state = g2left;
			else if (g2y < 30 && LEVELSCOPY[g2x][g2y + 1] != 1) state = g2right;
			else if (g2y >= 30) state = g2right;
			else if (LEVELSCOPY[g2x - 1][g2y] != 1) state = g2up;
			else if (LEVELSCOPY[g2x + 1][g2y] != 1) state = g2down;
			break;

			case g2down:
			if ((LEVELSCOPY[g2x + 1][g2y] != 1) && (LEVELSCOPY[g2x][g2y - 1] != 1) && (LEVELSCOPY[g2x][g2y + 1] != 1)) state = g2left;
			else if ((LEVELSCOPY[g2x + 1][g2y] != 1) && (LEVELSCOPY[g2x][g2y - 1] != 1)) state = g2down;
			else if ((LEVELSCOPY[g2x + 1][g2y] != 1) && (LEVELSCOPY[g2x][g2y + 1] != 1)) state = g2down;
			else if ((LEVELSCOPY[g2x][g2y - 1] != 1) && (LEVELSCOPY[g2x][g2y + 1] != 1)) state = g2right;
			else if (g2y > 0 && LEVELSCOPY[g2x][g2y - 1] != 1) state = g2left;
			else if (g2y <= 0) state = g2left;
			else if (g2y < 30 && LEVELSCOPY[g2x][g2y + 1] != 1) state = g2right;
			else if (g2y >= 30) state = g2right;
			else if (LEVELSCOPY[g2x + 1][g2y] != 1) state = g2down;
			else if (LEVELSCOPY[g2x - 1][g2y] != 1) state = g2up;
			break;

			case g2right:
			if ((LEVELSCOPY[g2x - 1][g2y] != 1) && (LEVELSCOPY[g2x][g2y + 1] != 1) && (LEVELSCOPY[g2x + 1][g2y] != 1)) state = g2down;
			else if ((LEVELSCOPY[g2x - 1][g2y] != 1) && (LEVELSCOPY[g2x][g2y + 1] != 1)) state = g2up;
			else if ((LEVELSCOPY[g2x + 1][g2y] != 1) && (LEVELSCOPY[g2x][g2y + 1] != 1)) state = g2right;
			else if ((LEVELSCOPY[g2x - 1][g2y] != 1) && (LEVELSCOPY[g2x + 1][g2y] != 1)) state = g2up;
			else if (LEVELSCOPY[g2x + 1][g2y] != 1) state = g2down;
			else if (g2y < 30 && LEVELSCOPY[g2x][g2y + 1] != 1) state = g2right;
			else if (g2y >= 30) state = g2right;
			else if (LEVELSCOPY[g2x - 1][g2y] != 1) state = g2up;
			else if (g2y > 0 && LEVELSCOPY[g2x][g2y - 1] != 1) state = g2left;
			else if (g2y <= 0) state = g2left;
			break;

			case g2left:
			if ((LEVELSCOPY[g2x - 1][g2y] != 1) && (LEVELSCOPY[g2x][g2y - 1] != 1) && (LEVELSCOPY[g2x + 1][g2y] != 1)) state = g2down;
			else if ((LEVELSCOPY[g2x - 1][g2y] != 1) && (LEVELSCOPY[g2x][g2y - 1] != 1)) state = g2up;
			else if ((LEVELSCOPY[g2x + 1][g2y] != 1) && (LEVELSCOPY[g2x][g2y - 1] != 1)) state = g2left;
			else if ((LEVELSCOPY[g2x - 1][g2y] != 1) && (LEVELSCOPY[g2x + 1][g2y] != 1)) state = g2down;
			else if (LEVELSCOPY[g2x + 1][g2y] != 1) state = g2down;
			else if (g2y > 0 && LEVELSCOPY[g2x][g2y - 1] != 1) state = g2left;
			else if (g2y <= 0) state = g2left;
			else if (LEVELSCOPY[g2x - 1][g2y] != 1) state = g2up;
			else if (g2y < 30 && LEVELSCOPY[g2x][g2y + 1] != 1) state = g2right;
			else if (g2y >= 30) state = g2right;
			break;

			default: state = g2up; break;
		}

		switch (state) {
			case g2up:
			g2x--; G2Signal = 0x01; break;
			case g2down:
			g2x++; G2Signal = 0x02; break;
			case g2left:
			if (g2y <= 0) { g2y = 30; G2Signal = 0x03; break;}
			g2y--; G2Signal = 0x03; break;
			case g2right:
			if (g2y >= 30) { g2y = 0; G2Signal = 0x04; break;}
			g2y++; G2Signal = 0x04; break;
			default: break;
		}
		}

		return state;
		
	}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------

// SCORE INCREMEMNTING

unsigned short scoreCnt = 0;
unsigned char resetScore = 1;

int Score(int state) {
	if (FLAG && resetScore) {
		scoreCnt = 0;
	} else {
		if (pacSignal == 0x01 && LEVELSCOPY[x][y] == 2) {
			scoreCnt++;
			LEVELSCOPY[x][y] = 0;
		} else if (pacSignal == 0x02 && LEVELSCOPY[x][y] == 2) {
			scoreCnt++;
			LEVELSCOPY[x][y] = 0;
		} else if (pacSignal == 0x03 && LEVELSCOPY[x][y] == 2) {
			scoreCnt++;
			LEVELSCOPY[x][y] = 0;
		} else if (pacSignal == 0x04 && LEVELSCOPY[x][y] == 2) {
			scoreCnt++;
			LEVELSCOPY[x][y] = 0;
		}
	}

	return state;
}

enum GameStates {startScreen, startRelease, firstLevel, firstLoss, firstWin};

unsigned char controlSignal = 0x07;
unsigned char scoreDisplay[] = "Score: ";
unsigned char countdown = 5;
unsigned char i = 0;

int Game(int state) {
	unsigned char D5 = GetBit(~PIND, 5);

	switch (state) {
		case startScreen:
			if (D5) {
				state = startRelease;
			} else {
				state = startScreen;
			}
			
			break;
		case startRelease:
			if (D5) {
				state = startRelease;
			} else {
				i = 0x0F;
				resetScore = 0;
				state = firstLevel;
				setupArr(0);
				countdown = 5;
			}
			
			break;
		case firstLevel:
			if (D5 || (x == g1x && y == g1y) || (x == g2x && y == g2y)) {
				i = 0x0F;
				state = firstLoss;
			} else if (!D5 && (scoreCnt < 0x0092) && !((x == g1x && y == g1y) || (x == g2x && y == g2y))) {
				if (i > 0) i--;
				state = firstLevel;
			} else if (!D5 && (scoreCnt >= 0x0092)) {
				i = 0x0F;
				state = firstWin;
			}
			
			break;
		case firstLoss:
			if (D5 || i > 0) {
				if (i > 0) i--;
				state = firstLoss;
			} else if (!D5 && i <= 0) {
				resetScore = 1;
				state = startScreen;
			}
			
			break;
		case firstWin:
			if (D5 || i > 0) {
				if (i > 0) i--;
				state = firstWin;
			} else if (!D5 && i <= 0) {
				resetScore = 1;
				state = startScreen;
			}

			break;
		default:
			state = startScreen; break;
	}

	// controlSignal		1 = LEVEL WIN		2 = LEVEL LOSE		3 = OVERALL WIN		4 = LEVEL ONE		5 = LEVEL TWO		6 = LEVEL THREE		7 = START SCREEN

	switch (state) {
		case startScreen:
			controlSignal = 0x07; FLAG = 1; G1Flag = 1; G2Flag = 1;
			
			
			LCD_ClearScreen(); LCD_DisplayString(1, " < START / RESET");
			
			break;
		case startRelease:
			controlSignal = 0x07; FLAG = 1; G1Flag = 1; G2Flag = 1;

			break;
		case firstLevel:
			controlSignal = 0x04;
			if (i > 0) {
				FLAG = 1; G1Flag = 1; G2Flag = 1;

				LCD_ClearScreen();
				LCD_DisplayString(1, "LEVEL 1 IN ");
				LCD_WriteData((countdown - 1) + '0');
				if (i % 5 == 0) countdown--;
			} else {
				FLAG = 0; G1Flag = 0; G2Flag = 0;
				
				LCD_ClearScreen();
				LCD_DisplayString(1, "LEVEL 1");
				LCD_Cursor(17);
				for (unsigned char x = 0; x < 7; ++x) {
					LCD_WriteData(scoreDisplay[x]);
				}
				LCD_WriteData(scoreCnt / 1000 + '0');
				LCD_WriteData((scoreCnt % 1000) / 100 + '0');
				LCD_WriteData((scoreCnt % 100) / 10 + '0');
				LCD_WriteData(scoreCnt % 10 + '0');
			}

			break;
		case firstLoss:
			controlSignal = 0x02; FLAG = 1; G1Flag = 1; G2Flag = 1;

			LCD_ClearScreen();
			LCD_DisplayString(1, "LEVEL 1 FAILED");
			LCD_Cursor(17);
			for (unsigned char x = 0; x < 7; ++x) {
				LCD_WriteData(scoreDisplay[x]);
			}
			LCD_WriteData(scoreCnt / 1000 + '0');
			LCD_WriteData((scoreCnt % 1000) / 100 + '0');
			LCD_WriteData((scoreCnt % 100) / 10 + '0');
			LCD_WriteData(scoreCnt % 10 + '0');
			
			break;
		case firstWin:
			controlSignal = 0x01; FLAG = 1; G1Flag = 1; G2Flag = 1;

			LCD_ClearScreen();
			LCD_DisplayString(1, "LEVEL 1 COMPLETE");
			LCD_Cursor(17);
			for (unsigned char x = 0; x < 7; ++x) {
				LCD_WriteData(scoreDisplay[x]);
			}
			LCD_WriteData(scoreCnt / 1000 + '0');
			LCD_WriteData((scoreCnt % 1000) / 100 + '0');
			LCD_WriteData((scoreCnt % 100) / 10 + '0');
			LCD_WriteData(scoreCnt % 10 + '0');

			break;
		default:
			break;
	}

	return state;
}

unsigned long SIGNAL = 0x00;

int Tinker(int state) {
	if (USART_IsSendReady(0)) {
		SIGNAL = (controlSignal);
		SIGNAL = SIGNAL << 21;
		SIGNAL |= (G1Signal << 3); //PORTB = g1y;
		SIGNAL |= (G2Signal << 6); //PORTB = (G1Signal << 4) | G2Signal;
		SIGNAL |= pacSignal;
		
		if (prevLocColor1 == 2) SIGNAL |= 0x008000; // G1 red
		if (prevLocColor2 == 2) SIGNAL |= 0x010000;
		
		USART_Send((SIGNAL >> 16) & 0x00FF, 0);
		USART_Send((SIGNAL >> 8) & 0x0000FF, 0);
		USART_Send((SIGNAL & 0x000000FF), 0);
		//USART_Flush(0);
	}
	return state;
}

int main() {
	DDRB = 0xFF; PORTB = 0x00; // used as check
	DDRC = 0xFF; PORTC = 0x00; // LCD
	DDRD = 0xDF; PORTD = 0x20; // bit 5 as start/reset input
							   // all else LCD
	
	static task joyTask, pacTask, g1Task, g2Task, scoreTask, gameTask, tinkTask;
	task *tasks[] = {&joyTask, &pacTask, &g1Task, &g2Task, &scoreTask, &gameTask, &tinkTask};
	unsigned short numTasks = 7;
	
	joyTask.state = -1; // single state function
	joyTask.period = 1; // constantly reading input
	joyTask.elapsedTime = joyTask.period;
	joyTask.TickFct = &ReadJoystick;

	pacTask.state = goLeft; // automatically start going left on start like original game
	pacTask.period = 150; // fast enough not to see the timer x 2 pause at intersections
	pacTask.elapsedTime = pacTask.period;
	pacTask.TickFct = &PacMove;
	
	scoreTask.state = -1; // calculates score during each move and displays to LCD
	scoreTask.period = 0; // must match timing of moves for correct calculations
	scoreTask.elapsedTime = scoreTask.period;
	scoreTask.TickFct = &Score;

	g1Task.state = g1right;
	g1Task.period = 150;
	g1Task.elapsedTime = g1Task.period;
	g1Task.TickFct = &G1;

	g2Task.state = g2up;
	g2Task.period = 150;
	g2Task.elapsedTime = g2Task.period;
	g2Task.TickFct = &G2;

	gameTask.state = startScreen;
	gameTask.period = 150;
	gameTask.elapsedTime = gameTask.period;
	gameTask.TickFct = &Game;
	
	tinkTask.state = -1; // sends moves via USART to display
	tinkTask.period = 150; // must match timing of moves to synchronized display
	tinkTask.elapsedTime = tinkTask.period;
	tinkTask.TickFct = &Tinker;
	
	ADC_init();
	initUSART(0);
	LCD_init();
	TimerSet(1); // set to rate of joystick sampling
				 // must have button release states in game for start/reset for compatibility
	TimerOn();
	
	while (1) {
		//cycle through tasks and reset Timerflag
		for (unsigned char i = 0; i < numTasks; i++) {
			if (tasks[i]->elapsedTime >= tasks[i]->period) {
				tasks[i]->state = tasks[i]->TickFct(tasks[i]->state);
				tasks[i]->elapsedTime = 0;
			}
			tasks[i]->elapsedTime++;
		}
		while (!TimerFlag);
		TimerFlag = 0;
	}
}

#include <avr/io.h>
#include <stdio.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include "usart_1284.h"
#include "timer.h"
#include "adc.h"
#include "bit.h"
#include "task.h"
#include "levels.h"
#include "io.c"

// LEVEL & FLAG

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
unsigned char xTL = 24; unsigned char yTL = 15; unsigned char xTR = 24; unsigned char yTR = 16;
unsigned char xBL = 25; unsigned char yBL = 15; unsigned char xBR = 25; unsigned char yBR = 16;
// LEFT = 1		RIGHT = 2		UP = 3		DOWN = 4
unsigned char pacSignal = 0x00;

/*
- keeps going in current direction (starts at LEFT)
- keeps going till either hits wall, reaches intersection, or prompted to go in opposite
	direction
	- if hits wall, waits at location until nextDirection changes to value that pacman can
		actually move in
	- if at intersection, goes back to wait state and checks to see if given a new
		nextDirection
		- if no, keeps moving in current direction
		- if yes, moves in new direction
	- prompted to go in opposite direction, IMMEDIATELY turns backwards
*/

unsigned char LEVELSCOPY[32][32];

int PacMove(int state) {
	if (FLAG) {
		xTL = 24; yTL = 15; xTR = 24; yTR = 16;
		xBL = 25; yBL = 15; xBR = 25; yBR = 16;
		
		for (unsigned char i = 0; i < 32; ++i) {
			for (unsigned char j = 0; j < 32; ++j) {
				LEVELSCOPY[i][j] = LEVELS[LEV][i][j];
			}
		}
		
		pacSignal = 0x00;
	} else {
		switch (state) {
			case goLeft:
				if (nextDirection == RIGHT) {
					state = pacWait; break;
				}

				if ((LEVELSCOPY[xTL - 1][yTL] != 1) && (LEVELSCOPY[xTR - 1][yTR] != 1) && (nextDirection == UP)) {
					state = pacWait; break;
				}

				if ((LEVELSCOPY[xBL + 1][yBL] != 1) && (LEVELSCOPY[xBR + 1][yBR] != 1) && (nextDirection == DOWN)) {
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

				if ((LEVELSCOPY[xTL - 1][yTL] != 1) && (LEVELSCOPY[xTR - 1][yTR] != 1) && (nextDirection == UP)) {
					state = pacWait; break;
				}

				if ((LEVELSCOPY[xBL + 1][yBL] != 1) && (LEVELSCOPY[xBR + 1][yBR] != 1) && (nextDirection == DOWN)) {
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

				if ((LEVELSCOPY[xTL][yTL - 1] != 1) && (LEVELSCOPY[xBL][yBL - 1] != 1) && (nextDirection == LEFT)) {
					state = pacWait; break;
				}

				if ((LEVELSCOPY[xTR][yTR + 1] != 1) && (LEVELSCOPY[xBR][yBR + 1] != 1) && (nextDirection == RIGHT)) {
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

				if ((LEVELSCOPY[xTL][yTL - 1] != 1) && (LEVELSCOPY[xBL][yBL - 1] != 1) && (nextDirection == LEFT)) {
					state = pacWait; break;
				}

				if ((LEVELSCOPY[xTR][yTR + 1] != 1) && (LEVELSCOPY[xBR][yBR + 1] != 1) && (nextDirection == RIGHT)) {
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
				if ( (yTL <= 0) && (yBL <= 0) ) {
					yTL = 30; yTR = 31;
					yBL = 30; yBR = 31;
					pacSignal = 0x01; break;
				}
				// otherwise not infinite, move left unless wall
				if ( (LEVELSCOPY[xTL][yTL - 1] != 1) && (LEVELSCOPY[xBL][yBL - 1] != 1) ) {
					yTL--; yTR--;
					yBL--; yBR--;
					pacSignal = 0x01; break;
				}

				if ( (LEVELSCOPY[xTL][yTL - 1] == 1) || (LEVELSCOPY[xBL][yBL - 1] == 1) ) {
					pacSignal = 0x00; break;
				}
				break;
			case goRight:
				// if infinite, move to other side of board
				if ( (yTR >= 31) && (yBR >= 31) ) {
					yTL = 0; yTR = 1;
					yBL = 0; yBR = 1;
					pacSignal = 0x02; break;
				}
				// otherwise not infinite, move right unless wall
				if ( (LEVELSCOPY[xTR][yTR + 1] != 1) && (LEVELSCOPY[xBR][yBR + 1] != 1) ) {
					yTL++; yTR++;
					yBL++; yBR++;
					pacSignal = 0x02; break;
				}
			
				if ( (LEVELSCOPY[xTR][yTR + 1] == 1) || (LEVELSCOPY[xBR][yBR + 1] == 1) ) {
					pacSignal = 0x00; break;
				}
				break;
			case goUp:
				// not infinite, move up unless wall
				if ( (LEVELSCOPY[xTL - 1][yTL] != 1) && (LEVELSCOPY[xTR - 1][yTR] != 1) ) {
					xTL--; xTR--;
					xBL--; xBR--;
					pacSignal = 0x03; break;
				}
			
				if ( (LEVELSCOPY[xTL - 1][yTL] == 1) || (LEVELSCOPY[xTR - 1][yTR] == 1) ) {
					pacSignal = 0x00; break;
				}
				break;
			case goDown:
				// not infinite, move down unless wall
				if ( (LEVELSCOPY[xBL + 1][yBL] != 1) && (LEVELSCOPY[xBR + 1][yBR] != 1) ) {
					xTL++; xTR++;
					xBL++; xBR++;
					pacSignal = 0x04; break;
				}

				if ( (LEVELSCOPY[xBL + 1][yBL] == 1) || (LEVELSCOPY[xBR + 1][yBR] == 1) ) {
					pacSignal = 0x00; break;
				}
				break;
			default:
				break;
		}
	}

	return state;
}

// SCORE INCREMEMNTING

unsigned short scoreCnt = 0;
unsigned char resetScore = 1;

int Score(int state) {
	if (FLAG && resetScore) {
		scoreCnt = 0;
	} else {
		if (pacSignal == 0x01 && LEVELSCOPY[xTL][yTL] == 2 && LEVELSCOPY[xBL][yBL] == 2) {
			scoreCnt++;
			LEVELSCOPY[xTL][yTL] = 0;
			LEVELSCOPY[xBL][yBL] = 0;
		} else if (pacSignal == 0x02 && LEVELSCOPY[xTR][yTR] == 2 && LEVELSCOPY[xBR][yBR] == 2) {
			scoreCnt++;
			LEVELSCOPY[xTR][yTR] = 0;
			LEVELSCOPY[xBR][yBR] = 0;
		} else if (pacSignal == 0x03 && LEVELSCOPY[xTL][yTL] == 2 && LEVELSCOPY[xTR][yTR] == 2) {
			scoreCnt++;
			LEVELSCOPY[xTL][yTL] = 0;
			LEVELSCOPY[xTR][yTR] = 0;
		} else if (pacSignal == 0x04 && LEVELSCOPY[xBL][yBL] == 2 && LEVELSCOPY[xBR][yBR] == 2) {
			scoreCnt++;
			LEVELSCOPY[xBL][yBL] = 0;
			LEVELSCOPY[xBR][yBR] = 0;
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
				countdown = 5;
			}
			
			break;
		case firstLevel:
			if (D5) {
				i = 0x0F;
				state = firstLoss;
			} else if (!D5 && (scoreCnt < 0x00B8)) {
				if (i > 0) i--;
				state = firstLevel;
			} else if (!D5 && (scoreCnt >= 0x00B8)) {
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
				if (i > 0) i --;
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
			controlSignal = 0x07; FLAG = 1;
			
			LCD_ClearScreen(); LCD_DisplayString(1, " < START / RESET");
			
			break;
		case startRelease:
			controlSignal = 0x07; FLAG = 1;

			break;
		case firstLevel:
			controlSignal = 0x04;
			if (i > 0) {
				FLAG = 1;

				LCD_ClearScreen();
				LCD_DisplayString(1, "LEVEL 1 IN ");
				if (i % 5 == 0) countdown--;
				LCD_WriteData(countdown + '0');
				LCD_Cursor(17);
				for (unsigned char x = 0; x < 7; ++x) {
					LCD_WriteData(scoreDisplay[x]);
				}
				LCD_WriteData(scoreCnt / 1000 + '0');
				LCD_WriteData((scoreCnt % 1000) / 100 + '0');
				LCD_WriteData((scoreCnt % 100) / 10 + '0');
				LCD_WriteData(scoreCnt % 10 + '0');
			} else {
				FLAG = 0;
				
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
			controlSignal = 0x02; FLAG = 1;

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
			controlSignal = 0x01; FLAG = 1;

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

unsigned short SIGNAL = 0x00;

int Tinker(int state) {
	if (USART_IsSendReady(0)) {
		SIGNAL = ((controlSignal << 13) & 0xE000);
		SIGNAL |= pacSignal;
		/*if (pacSignal == 0x01) {USART_Send(0x01, 0); } //PORTB = 0x02;}
		else if (pacSignal == 0x02) {USART_Send(0x02, 0); } //PORTB = 0x01;}
		else if (pacSignal == 0x03) {USART_Send(0x03, 0); } //PORTB = 0x04;}
		else if (pacSignal == 0x04) {USART_Send(0x04, 0); } //PORTB = 0x08;}*/
		//USART_Send(SIGNAL >> 13, 0);
		USART_Send((SIGNAL >> 8), 0);
		USART_Send((SIGNAL & 0x00FF), 0);
		//USART_Flush(0);
	}
	return state;
}

int main() {
	DDRB = 0xFF; PORTB = 0x00; // used as check
	DDRC = 0xFF; PORTC = 0x00; // LCD
	DDRD = 0xDF; PORTD = 0x20; // bit 5 as start/reset input
							   // all else LCD
	
	static task joyTask, pacTask, scoreTask, gameTask, tinkTask;
	task *tasks[] = {&joyTask, &pacTask, &scoreTask, &gameTask, &tinkTask};
	unsigned short numTasks = 5;
	
	joyTask.state = -1; // single state function
	joyTask.period = 1; // constantly reading input
	joyTask.elapsedTime = joyTask.period;
	joyTask.TickFct = &ReadJoystick;

	pacTask.state = goLeft; // automatically start going left on start like original game
	pacTask.period = 150; // fast enough not to see the timer x 2 pause at intersections
	pacTask.elapsedTime = pacTask.period;
	pacTask.TickFct = &PacMove;
	
	scoreTask.state = -1; // calculates score during each move and displays to LCD
	scoreTask.period = 150; // must match timing of moves for correct calculations
	scoreTask.elapsedTime = scoreTask.period;
	scoreTask.TickFct = &Score;

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

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

int PacMove(int state) {
	switch (state) {
		case goLeft:
			if (nextDirection == RIGHT) {
				state = pacWait; break;
			}

			if ((level1[xTL - 1][yTL] != 1) && (level1[xTR - 1][yTR] != 1) && (nextDirection == UP)) {
				state = pacWait; break;
			}

			if ((level1[xBL + 1][yBL] != 1) && (level1[xBR + 1][yBR] != 1) && (nextDirection == DOWN)) {
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

			if ((level1[xTL - 1][yTL] != 1) && (level1[xTR - 1][yTR] != 1) && (nextDirection == UP)) {
				state = pacWait; break;
			}

			if ((level1[xBL + 1][yBL] != 1) && (level1[xBR + 1][yBR] != 1) && (nextDirection == DOWN)) {
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

			if ((level1[xTL][yTL - 1] != 1) && (level1[xBL][yBL - 1] != 1) && (nextDirection == LEFT)) {
				state = pacWait; break;
			}

			if ((level1[xTR][yTR + 1] != 1) && (level1[xBR][yBR + 1] != 1) && (nextDirection == RIGHT)) {
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

			if ((level1[xTL][yTL - 1] != 1) && (level1[xBL][yBL - 1] != 1) && (nextDirection == LEFT)) {
				state = pacWait; break;
			}

			if ((level1[xTR][yTR + 1] != 1) && (level1[xBR][yBR + 1] != 1) && (nextDirection == RIGHT)) {
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
			if ( (level1[xTL][yTL - 1] != 1) && (level1[xBL][yBL - 1] != 1) ) {
				yTL--; yTR--;
				yBL--; yBR--;
				pacSignal = 0x01; break;
			}

			if ( (level1[xTL][yTL - 1] == 1) || (level1[xBL][yBL - 1] == 1) ) {
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
			if ( (level1[xTR][yTR + 1] != 1) && (level1[xBR][yBR + 1] != 1) ) {
				yTL++; yTR++;
				yBL++; yBR++;
				pacSignal = 0x02; break;
			}
			
			if ( (level1[xTR][yTR + 1] == 1) || (level1[xBR][yBR + 1] == 1) ) {
				pacSignal = 0x00; break;
			}
			break;
		case goUp:
			// not infinite, move up unless wall
			if ( (level1[xTL - 1][yTL] != 1) && (level1[xTR - 1][yTR] != 1) ) {
				xTL--; xTR--;
				xBL--; xBR--;
				pacSignal = 0x03; break;
			}
			
			if ( (level1[xTL - 1][yTL] == 1) || (level1[xTR - 1][yTR] == 1) ) {
				pacSignal = 0x00; break;
			}
			break;
		case goDown:
			// not infinite, move down unless wall
			if ( (level1[xBL + 1][yBL] != 1) && (level1[xBR + 1][yBR] != 1) ) {
				xTL++; xTR++;
				xBL++; xBR++;
				pacSignal = 0x04; break;
			}

			if ( (level1[xBL + 1][yBL] == 1) || (level1[xBR + 1][yBR] == 1) ) {
				pacSignal = 0x00; break;
			}
			break;
		default:
			break;
	}

	return state;
}

// SCORE INCREMEMNTING

unsigned short scoreCnt = 0;

int Score(int state) {
	if (pacSignal == 0x01 && level1[xTL][yTL] == 2 && level1[xBL][yBL] == 2) {
		scoreCnt++;
		level1[xTL][yTL] = 0;
		level1[xBL][yBL] = 0;
	} else if (pacSignal == 0x02 && level1[xTR][yTR] == 2 && level1[xBR][yBR] == 2) {
		scoreCnt++;
		level1[xTR][yTR] = 0;
		level1[xBR][yBR] = 0;
	} else if (pacSignal == 0x03 && level1[xTL][yTL] == 2 && level1[xTR][yTR] == 2) {
		scoreCnt++;
		level1[xTL][yTL] = 0;
		level1[xTR][yTR] = 0;
	} else if (pacSignal == 0x04 && level1[xBL][yBL] == 2 && level1[xBR][yBR] == 2) {
		scoreCnt++;
		level1[xBL][yBL] = 0;
		level1[xBR][yBR] = 0;
	}
	
	//PORTB = scoreCnt;

	LCD_ClearScreen();
	LCD_DisplayString(1, "Score: ");
	LCD_Cursor(8);
	LCD_WriteData(scoreCnt / 1000 + '0');
	LCD_WriteData((scoreCnt % 1000) / 100 + '0');
	LCD_WriteData((scoreCnt % 100) / 10 + '0');
	LCD_WriteData(scoreCnt % 10 + '0');

	return state;
}

int Tinker(int state) {
	if (USART_IsSendReady(0)) {
		if (pacSignal == 0x01) {USART_Send(0x01, 0); } //PORTB = 0x02;}
		else if (pacSignal == 0x02) {USART_Send(0x02, 0); } //PORTB = 0x01;}
		else if (pacSignal == 0x03) {USART_Send(0x03, 0); } //PORTB = 0x04;}
		else if (pacSignal == 0x04) {USART_Send(0x04, 0); } //PORTB = 0x08;}
		USART_Flush(0);
	}
	return state;
}

int main() {
	DDRB = 0xFF; PORTB = 0x00; // used as check
	DDRC = 0xFF; PORTC = 0x00; // LCD
	DDRD = 0xDF; PORTD = 0x20; // bit 5 as start/reset input
							   // all else LCD
	
	static task joyTask, pacTask, scoreTask, tinkTask;
	task *tasks[] = {&joyTask, &pacTask, &scoreTask, &tinkTask};
	unsigned short numTasks = 4;
	
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

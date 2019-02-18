/*
 Name:		keyboarduino.ino
 Created:	2019-02-14 12:47:20
 Author:	Hagbard_Celine

 keyboarduino is an implementation of W19 CS351 Assignment 2
*/

//  frequency of notes, middle C through F
#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_F4  349

//liquid crystal library
#include <LiquidCrystal.h>

//  global consts
//  state enum
enum Mode { Record = 0, Replay = 1, Reset = 2 };
//  notes to play
const int notes[] = { NOTE_C4, NOTE_D4, NOTE_E4, NOTE_F4 };
//  note duration in millis for replay
const int noteDuration = 500;
//  button pins, C,D,E,F,Replay,Reset
const int buttonPins[] = { A0,A1,A2,A3,A4,A5 };
//const int replayButtonPin = A5;
//  led pins
const int noteLedPins[] = { 9,8,7,6 };
//  speaker pin
const int speakerPin = 10;
const int numButtons = 6;

//  global variables
//  state
enum Mode mode = Record;
//
volatile int buttonDebounceStates[] = { 0,0,0,0,0,0 };
volatile bool buttonPressedStates[] = { false, false, false, false, false, false };
bool lastButtonPressedStates[] = { false, false, false, false, false, false };

int pressing;

volatile int playTime;

//liquid crystal globals
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

//  played note array

char* played = NULL;
unsigned int playedSize = 0;
unsigned int playedCap = 0;

bool rawBtnPressed(int btn)
{
	return digitalRead(buttonPins[btn]);
}

void debounceBtn(int btn)
{
	if (btn >= numButtons) { return; }
	buttonDebounceStates[btn] = (buttonDebounceStates[btn] << 1) | (!rawBtnPressed(btn));// | 0xe000; //  push 1s when off, 0s when on
	if (buttonDebounceStates[btn] == 0x8000) { buttonPressedStates[btn] = true; } //  1 1 (off) followed by 0s (ons)
	else if (buttonDebounceStates[btn] == 0x7fff) { buttonPressedStates[btn] = false; } //  0 0(on) followed by 1s (offs)
}


//  stop playing sound, turn off lights
void noPlay()
{
	noTone(speakerPin);
	digitalWrite(noteLedPins[0], LOW);
	digitalWrite(noteLedPins[1], LOW);
	digitalWrite(noteLedPins[2], LOW);
	digitalWrite(noteLedPins[3], LOW);
}

//  play a note, turn on corresponding light
void play(char note)
{
	switch (note)
	{
	case 'c':
		tone(speakerPin, notes[0]);
		digitalWrite(noteLedPins[0], HIGH);
		digitalWrite(noteLedPins[1], LOW);
		digitalWrite(noteLedPins[2], LOW);
		digitalWrite(noteLedPins[3], LOW);
		break;
	case 'd':
		tone(speakerPin, notes[1]);
		digitalWrite(noteLedPins[0], LOW);
		digitalWrite(noteLedPins[1], HIGH);
		digitalWrite(noteLedPins[2], LOW);
		digitalWrite(noteLedPins[3], LOW);
		break;
	case 'e':
		tone(speakerPin, notes[2]);
		digitalWrite(noteLedPins[0], LOW);
		digitalWrite(noteLedPins[1], LOW);
		digitalWrite(noteLedPins[2], HIGH);
		digitalWrite(noteLedPins[3], LOW);
		break;
	case 'f':
		tone(speakerPin, notes[3]);
		digitalWrite(noteLedPins[0], LOW);
		digitalWrite(noteLedPins[1], LOW);
		digitalWrite(noteLedPins[2], LOW);
		digitalWrite(noteLedPins[3], HIGH);
		break;
	default:
		noPlay();
	}
}

void record(char n)
{
	if (playedSize >= playedCap)
	{
		return;	//	do nothing for now, maybe update to expand array
	}
	played[playedSize] = n;
	++playedSize;
}

void replay()
{
	for (unsigned int i = 0; i < playedSize; ++i)
	{
		play(played[i]);
		delay(500);	//	this is garbage that must be replaced.
	}
	noPlay();
	pressing = 6;
}

void reset()
{
	for (unsigned int i = 0; i < playedSize; ++i)
	{
		played[i] = 0;
	}
	playedSize = 0;
}

void press(int btn, bool state)
{
	if (state)
	{
		switch (btn)
		{
		case 0:
		case 1:
		case 2:
		case 3:
			play('c' + btn);
			record('c' + btn);
			pressing = btn;
			break;
		case 4:
			pressing = btn;
			replay();
			break;
		case 5:
			reset();
			break;
		}
	}
	else
	{
		switch (btn)
		{
		case 0:
		case 1:
		case 2:
		case 3:
			if (pressing == btn) { noPlay(); pressing = 6; }
			break;
		case 4:
		case 5:
			break;
		}
	}
}

void readBtn(int btn)
{
	if (btn >= numButtons) { return; }
	bool tempState = buttonPressedStates[btn];
	if (lastButtonPressedStates[btn] != tempState)
	{
		lastButtonPressedStates[btn] = tempState;
		press(btn, lastButtonPressedStates[btn]);
	}
}

// the setup function runs once when you press reset or power the board
void setup() 
{
	cli();
	mode = Record;
	pressing = 6;
	//  button inputs
	pinMode(buttonPins[0], INPUT);
	pinMode(buttonPins[1], INPUT);
	pinMode(buttonPins[2], INPUT);
	pinMode(buttonPins[3], INPUT);
	pinMode(buttonPins[4], INPUT);
	pinMode(buttonPins[5], INPUT);
	//  led outputs
	pinMode(noteLedPins[0], OUTPUT);
	pinMode(noteLedPins[1], OUTPUT);
	pinMode(noteLedPins[2], OUTPUT);
	pinMode(noteLedPins[3], OUTPUT);
	//  speaker output
	pinMode(speakerPin, OUTPUT);  //  though tone should take care of this

	playTime = 0;

	//  initialize played note array
	played = (char*)calloc(256, sizeof(char));
	//	need error handling here
	playedCap = 255;
	playedSize = 0;

	//noInterrupts();
	//multithreaded madness goes here
	//OCR1A = 0xAF;
	//TIMSK1 |= _BV(OCIE0A);
	//interrupts();
	// TIMER 1 for interrupt frequency 500 Hz:
	//cli(); // stop interrupts
	TCCR1A = 0; // set entire TCCR1A register to 0
	TCCR1B = 0; // same for TCCR1B
	TCNT1 = 0; // initialize counter value to 0
	// set compare match register for 500 Hz increments
	OCR1A = 31999; // = 16000000 / (1 * 500) - 1 (must be <65536)
	// turn on CTC mode
	TCCR1B |= (1 << WGM12);
	// Set CS12, CS11 and CS10 bits for 1 prescaler
	TCCR1B |= (0 << CS12) | (0 << CS11) | (1 << CS10);
	// enable timer compare interrupt
	TIMSK1 |= (1 << OCIE1A);
	sei(); // allow interrupts
	
}

// the loop function runs over and over again until power down or reset
void loop() 
{
	
	
	for (int i = 0; i < 6; ++i)
	{
		readBtn(i);
	}
}

ISR(TIMER1_COMPA_vect)          // interrupt service routine that wraps a user defined function supplied by attachInterrupt
{
	
	++playTime;
	for (int i = 0; i < 6; ++i)
	{
		debounceBtn(i);
		
	}
	
}

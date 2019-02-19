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
//  state enum, superseded by pressing
//	enum Mode { Record = 0, Replay = 1, Reset = 2 };
//  notes to play
const int notes[] = { NOTE_C4, NOTE_D4, NOTE_E4, NOTE_F4 };
//  note duration in 2millis for replay
const int noteDuration = 125;
//  button pins, C,D,E,F,Replay,Reset
const int buttonPins[] = { A0,A1,A2,A3,A4,A5 };
//const int replayButtonPin = A5;
//  led pins
const int noteLedPins[] = { 9,8,7,6 };
//  speaker pin
const int speakerPin = 10;
const int numButtons = 6;

//  global variables
//  state, superseded by pressing
//	enum Mode mode = Record;
//
volatile int buttonDebounceStates[] = { 0,0,0,0,0,0 };	//	debounce bitstream buffers
volatile bool buttonPressedStates[] = { false, false, false, false, false, false };	//	debounced button states
bool lastButtonPressedStates[] = { false, false, false, false, false, false };	//	button states saved by loop functions

int pressing;	//	actively pressed button, consider changing to enum
int playing;	//	currently replaying recorded note

volatile int playTime;	//	time playing current replay

//liquid crystal globals
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;	//	pins
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);	//	constructed object

//  played note array

char* played = NULL;	//	char array of played notes
unsigned int playedSize = 0;	//	number of currently held notes
unsigned int playedCap = 0;	//	max number of held notes

//	wrapper for digitalreads
bool rawBtnPressed(int btn)
{
	return digitalRead(buttonPins[btn]);
}

//	update debounce state of button by shifting left and bitwise oring !digitalread, will only change pressedState on a debounceState of 1000... or 0111...
void debounceBtn(int btn)
{
	if (btn >= numButtons) { return; }	//	if button doesn't exist
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

//  play a note, turn on corresponding light and turn off others, this could be streamlined for extensibility
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

//	add playing note to note array
void record(char n)
{
	if (playedSize >= playedCap)	//	full array
	{
		return;	//	do nothing for now, maybe update to expand array
	}
	played[playedSize] = n;	//	append note to array
	++playedSize;	//	increment size
	//	lcd writing code, could be generalized based on values sent in lcd.begin(col, row) in setup
	if (playedSize > 32) { return; }	//	full LCD screen
	if (playedSize == 17) { lcd.setCursor(0, 1); }	//	full LCD first row, change to second row
	lcd.print(n);	//	append note to screen
	
}

//	play next note/stop playing after current note has played for noteDuration
void replay()
{
	/*
	for (unsigned int i = 0; i < playedSize; ++i)
	{
		play(played[i]);
		delay(500);	//	this is garbage that must be replaced.
	}
	noPlay();
	pressing = 6;
	*/
	if (playTime > noteDuration)	//	note has played for noteDuration Timer1 ticks
	{
		
		if (playing < playedSize)	//	if there's another note
		{
			play(played[playing]);	//	play the next note
			playTime = 0;	//	reset tick timer
			++playing;	//	increment index
		}
		else
		{
			noPlay();	//	stop playing
			playing = 0;	//	reset index
			pressing = numButtons;	//	reset button state
		}
	}
}

void reset()
{
	cli();	//	stop interrupts, maybe not necessary but this is code i don't wan't interrupted
	for (unsigned int i = 0; i < playedSize; ++i)	//	set array to 0
	{
		played[i] = 0;
	}
	pressing = numButtons;	//	rest button state
	playedSize = 0;	//	set number of elements to 0
	playing = 0;	//	set replay index to zero
	lcd.clear();	//	clear lcd
	sei();	//	restart interrupts
}

//	do a thing based on a change of a button state
void press(int btn, bool state)
{
	if (state)
	{
		switch (btn)
		{
		case 0:
		case 1:
		case 2:
		case 3:	//	note buttons, play a note, record that note, change active button state, reset replay index
			play('c' + btn);
			record('c' + btn);
			pressing = btn;
			playing = 0;
			break;
		case 4:	//	replay button, change button state, reset replay index, make sure playTime>noteDuration (so start playing first note immediately), call replay
			pressing = btn;
			playing = 0;
			playTime = noteDuration+1;
			replay();
			break;
		case 5:	//	reset button (clears array)
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
			if (pressing == btn) { noPlay(); pressing = numButtons; }	//	stop playing if active button released
			break;
		case 4:
		case 5:
			break;	//	reset and replay buttons do nothing on release
		}
	}
}

//	compare held last pressed state to volatile pressedstate, do stuff if there's a change
void readBtn(int btn)
{
	if (btn >= numButtons) { return; }	//	button doesn't exist sanity check
	bool tempState = buttonPressedStates[btn];	//	save debounced press state (to potentially avoid comparing and assigning two different values, though i dunno if the compiler will optimize it away)
	if (lastButtonPressedStates[btn] != tempState)	//	consider replacing with if lastButtonPressedStates!=buttonPressedStates { lastButtonPressedStates=!lastButtonPressedStates ?
	{
		lastButtonPressedStates[btn] = tempState;	//	assign new state
		press(btn, lastButtonPressedStates[btn]);	//	do stuff based on state change
	}
}

// the setup function runs once when you press reset or power the board
void setup() 
{
	cli();	//	stop interrupts
	//mode = Record;
	pressing = numButtons;	//	empty buttonpress state
	playing = 0;	//	replay index
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

	playTime = 0;	//	note duration timer

	//  initialize played note array
	played = (char*)calloc(256, sizeof(char));
	//	need error handling here
	playedCap = 255;
	playedSize = 0;

	lcd.begin(16, 2);	//	initialize lcd with screen size, consider defining values to use with record()

	//	setting magic numbers to make Timer1 operate at 500 Hz
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
	
	
	for (int i = 0; i < 6; ++i)	//	compare currently held button states to debounced button states, do stuff on change
	{
		readBtn(i);
	}
	if (pressing == 4)	//	if active button is replay
	{
		replay();
	}
}

ISR(TIMER1_COMPA_vect)          // interrupt service routine keyed to TIMER1_COMPA
{
	
	++playTime;	//	increment duration timer
	for (int i = 0; i < numButtons; ++i)	//	debounce button states
	{
		debounceBtn(i);
		
	}
	
}

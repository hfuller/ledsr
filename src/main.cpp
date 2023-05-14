#include "Arduino.h"

#define DEBUG false

//This is where we set how many LEDs we have.
#define NUM_LEDS 40

//These are the possible LED behaviors.
#define ON 0
#define OFF 1
#define TOGGLE_EVERY_FRAME 2
#define ON_PCT_20 3
#define ON_PCT_20_OTHER 4
#define OFF_PCT_20 5
#define OFF_PCT_20_OTHER 6
#define ON_PCT_20_RANDOM 7
#define OFF_PCT_20_RANDOM 8

////Pin connect to Latch
const int latchPin = 3;
////Pin connected to Clock
const int clockPin = 4;
////Pin connected to Data
const int dataPin = 2;

const int troubleshootingButtonPin = 11;

bool states[NUM_LEDS];
byte thing;

//i is the "main Counter" for any app that is running.
int i = 0;
//This defines 1/framerate (aka the duration per frame in ms).
int frameDuration = 100;
//This is the light program that we are going to run. 0 is bootup, others defined elsewhere.
int mode = 0;

int config[NUM_LEDS];

void buttonInterrupt() {
	if ( mode == 250 ) {
		Serial.println(i++);
	} else {
		mode = 250;
	}
}

void setup() {
 
 Serial.begin(9600);
 Serial.println("henlo");
 Serial.print(sizeof(states)); Serial.println(" leds");

 Serial.println("Setting up debug button");
 pinMode(troubleshootingButtonPin, INPUT_PULLUP);
 attachInterrupt(digitalPinToInterrupt(troubleshootingButtonPin), buttonInterrupt, FALLING);

 Serial.println("Setting up sr output");
 pinMode(latchPin, OUTPUT);
 pinMode(dataPin, OUTPUT);  
 pinMode(clockPin, OUTPUT);

 Serial.println("Seeding rng");
 randomSeed(analogRead(0));

 Serial.println("!!! FOR TROUBLESHOOTING MODE PRESS d AT ANY TIME !!!");

 //This is where the behavior for the LEDs is defined.
 //Any LED that is not defined here will be ON.
 //NOTE! You want to start counting LEDs at zero. The first LED config will be set at config[0].
 config[0] = ON;
 config[1] = OFF;
 //config[2] = TOGGLE_EVERY_FRAME;
 config[3] = ON_PCT_20;
 config[4] = ON_PCT_20_OTHER;
 config[5] = OFF_PCT_20;
 config[6] = OFF_PCT_20_OTHER;
 config[7] = ON_PCT_20_RANDOM;
 config[8] = OFF_PCT_20_RANDOM;
 config[9] = OFF_PCT_20_RANDOM;
}


void loop() {
 if ( DEBUG ) { Serial.print("!!! i == "); Serial.println(i); }
 //EFFECTS CODE
 if ( mode == 0 ) {
	 //Blank All
	 for ( int i = 0; i < sizeof(states); i++ ) {
		 states[i] = false;
	 }
	 mode++;
 } else if ( mode == 1 ) {
	 //Bootup
	 frameDuration = 25;
	 states[random(0,NUM_LEDS)] = true;
	 if ( i * frameDuration > 5000 ) {
		 mode++;
		 i = 0;
	 }
 } else if ( mode == 2 ) {
	 //Run Mode
	 frameDuration = 100;
	 for ( int j=0; j<sizeof(states); j++ ) {
		 if ( config[j] == TOGGLE_EVERY_FRAME ) {
			 states[j] = ! states[j];
		 } else if ( config[j] == ON_PCT_20 ) {
			 states[j] = ( i % 10 <= 2 );
		 } else if ( config[j] == ON_PCT_20_OTHER ) {
			 states[j] = ( (i+4) % 10 <= 2 );
		 } else if ( config[j] == OFF_PCT_20 ) {
			 states[j] = !( i % 10 <= 2 );
		 } else if ( config[j] == OFF_PCT_20_OTHER ) {
			 states[j] = !( (i+4) % 10 <= 2 );
		 } else if ( config[j] == ON_PCT_20_RANDOM ) {
			 states[j] = ( random(0,10) <= 2 );
		 } else if ( config[j] == OFF_PCT_20_RANDOM ) {
			 states[j] = !( random(0,10) <= 2 );
		 } else if ( config[j] == OFF ) {
			 states[j] = false;
		 } else {
			 //just turn it on
			 states[j] = true;
		 }
	 }
	 if ( i > 50 ) {
		 i = 0;
	 }
 } else if ( mode == 250 ) {
	 //Troubleshooting Mode
	 frameDuration = 500;
	 if ( i >= NUM_LEDS ) i=0;
	 Serial.print("!!! Troubleshooting Mode. Keys: i=next LED. x=exit. 0=start over. This is LED #"); Serial.println(i);
	 for ( int j = 0; j < sizeof(states); j++ ) {
		 if ( j == i ) {
			 states[j] = ! states[j];
		 } else states[j] = false;
	 }
 }

 //END EFFECTS CODE

 if ( DEBUG ) { Serial.print("Blitting "); Serial.print(ceil(sizeof(states)/8)); Serial.println(" bytes"); }

 digitalWrite(latchPin, LOW);
 for ( int j = 0; j < ceil(sizeof(states)/8); j++ ) {
	 //memcpy(&thing, &states + j, 1);

	 //Oh my god this is such a HACK but whatever... I'm too rusty at C++ to do it another way.
	 //We copy the states into a byte, one bit at a time. But who cares, the uC is fast.
	 thing = B00000000;
	 for ( int k = 0; k < 8; k++ ) {
		 if ( DEBUG ) Serial.print(states[j*8 + k]);
		 if ( states[j*8 + k] ) {
			 bitSet(thing, k);
		 }
	 }
	 if ( DEBUG ) Serial.println();

	 if ( DEBUG ) { Serial.print(j); Serial.print(" "); Serial.print(states[j*8]); Serial.print(" "); Serial.println(thing, BIN); }
	 shiftOut(dataPin, clockPin, MSBFIRST, thing);
 }
 digitalWrite(latchPin, HIGH);
 delay(frameDuration);

 if ( mode != 250 ) i++;
 Serial.print('.'); //heartbeat
 if ( DEBUG ) Serial.println();

 //Serial debug support
 if ( Serial.available() > 0 ) {
	 char c = Serial.read();
	 if ( c == 'd' ) {
		 mode = 250;
	 } else if ( c == 'i' ) {
		 i++;
	 } else if ( c == '0' ) {
		 i=0;
	 } else { 
		 mode = 2;
	 }
	 Serial.read(); //dump it
 }
}


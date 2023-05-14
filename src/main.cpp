#include "Arduino.h"

#define DEBUG false

////Pin connect to Latch
const int latchPin = 3;
////Pin connected to Clock
const int clockPin = 4;
////Pin connected to Data
const int dataPin = 2;

bool states[40];
byte thing;

int i;

void setup() {
 //set pins to output because they are addressed in the main loop
 pinMode(latchPin, OUTPUT);
 pinMode(dataPin, OUTPUT);  
 pinMode(clockPin, OUTPUT);
 
 Serial.begin(9600);
 Serial.println("henlo");
 Serial.print(sizeof(states)); Serial.println(" leds");

 for ( int i = 0; i < sizeof(states); i++ ) {
	 states[i] = false;
 }
 for ( int i = 0; i < 4; i++ ) {
	 states[i] = true;
 }

 i = 0;
 
}


void loop() {
 states[i] = true;
 Serial.println(i);

 Serial.print("Blitting "); Serial.print(ceil(sizeof(states)/8)); Serial.println(" bytes");

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
 if ( DEBUG ) Serial.println();
 digitalWrite(latchPin, HIGH);
 delay(200);

 states[i] = false;
 i++;
 if ( i >= sizeof(states) ) {
	 i = 0;
 }
}

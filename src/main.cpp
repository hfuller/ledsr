#include "Arduino.h"

//https://github.com/RobTillaart/PCF8575.git
#include <PCF8575.h>
//only for debug stuff
#include <Wire.h>

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
const int dataPin = 5;

const int troubleshootingButtonPin = 2;

PCF8575 xp[8];
uint16_t xpvals[8];

bool states[NUM_LEDS];
byte thing;
bool toBeIncremented;

//i is the "main Counter" for any app that is running.
int i = 0;
//This defines 1/framerate (aka the duration per frame in ms).
int frameDuration = 100;
//This is the light program that we are going to run. 0 is bootup, others defined elsewhere.
int mode = 0;

int config[NUM_LEDS];

void buttonInterrupt() {
	if ( mode == 250 ) {
		toBeIncremented = true;
	} else {
		mode=250;
		i=0;
		toBeIncremented = false;
	}
}

void setup() {
 
 Serial.begin(9600);
 Serial.setTimeout(100); //More responsive/less hangy serial rx
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

 Serial.println("Setting up gpio expanders");
 for ( int j=0; j<8; j++ ) {
	 xp[j] = PCF8575(0x20 + j);
	 xp[j].begin(); 
	 Serial.print("XP"); Serial.print(j); Serial.print(" at addr "); Serial.print(xp[j].getAddress());
	 if ( ! xp[j].isConnected() ) Serial.print(" NOT");
	 Serial.print(" connected - "); Serial.println(xp[j].read16(), HEX);
 }

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
	 frameDuration = 250;
	 if ( i >= NUM_LEDS ) i=0;
	 Serial.print("!!! Troubleshooting Mode. Keys: i=next LED. x=exit. 0=start over. This is LED #"); Serial.println(i);
	 if ( DEBUG ) Serial.println(digitalRead(troubleshootingButtonPin));
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

 if ( mode != 250 ) {
	 i++;
 } else {
	 if ( toBeIncremented ) {
		delay(500);
		if ( !digitalRead(troubleshootingButtonPin) ) {
			mode = 2;
		}
	 	i++;
	 	toBeIncremented = false;
	 }
 }

 //GPIO Expander handling.
 for ( int j=0; j<8; j++ ) {
	 uint16_t value = xp[j].read16();
	 if ( value != xpvals[j] ) {
		 for ( int k=0, mask=1; k < 16; k++, mask = mask << 1 ) {
			 if ( ( value & mask ) != ( xpvals[j] & mask ) ) {
				 Serial.print("FLIP "); Serial.print(j); Serial.print(" "); Serial.print(k); Serial.print(" "); Serial.println( ( value & mask ) == 0 );
			 }
		 }
		 xpvals[j] = value;
	 }
 }

 //Serial debug support
 if ( Serial.available() > 0 ) {
	 char c = Serial.read();
	 if ( c == 'd' ) {
		 mode = 250;
	 } else if ( c == 'i' ) {
		 i++;
	 } else if ( c == '0' ) {
		 i=0;
	 } else if ( c == 's' ) {
		 //Set mode of specific LED. Syntax: 's 42 2' sets LED number 42 to be TOGGLE_EVERY_FRAME.
		 int ledtoset = Serial.parseInt(SKIP_WHITESPACE);
		 if ( ledtoset >= 0 && ledtoset < NUM_LEDS ) {
			 config[ledtoset] = Serial.parseInt(SKIP_WHITESPACE);
		 } else {
			 Serial.print("set command parse error - "); Serial.println(ledtoset);
		 }
	 } else if ( c == 'r' ) {
		 //Read GPIO expanders.
		 for ( int j=0; j<8; j++ ) {
			 Serial.print(xp[j].read16());
		 }
	 } else if ( c == '2' ) {
		 //I2C Test
		 Wire.begin();
		 Serial.println("I2C bus scanning...");
		 byte error, address;
		 int nDevices;
		
		 Serial.println("Scanning...");
		
		 nDevices = 0;
		 for(address = 1; address < 127; address++ ) 
		 {
		   // The i2c_scanner uses the return value of
		   // the Write.endTransmisstion to see if
		   // a device did acknowledge to the address.
		   Wire.beginTransmission(address);
		   error = Wire.endTransmission();
		
		   if (error == 0)
		   {
		     Serial.print("I2C device found at address 0x");
		     if (address<16) 
		       Serial.print("0");
		     Serial.print(address,HEX);
		     Serial.println("  !");
		
		     nDevices++;
		   }
		   else if (error==4) 
		   {
		     Serial.print("Unknown error at address 0x");
		     if (address<16) 
		       Serial.print("0");
		     Serial.println(address,HEX);
		   }    
		 }
		 if (nDevices == 0)
		   Serial.println("No I2C devices found\n");
		 else
		   Serial.println("done\n");
		 Serial.println("!!! YOU NEED to reboot the device now !!!");
	 } else { 
		 mode = 2;
	 }
	 Serial.read(); //dump it
 }

 Serial.println(); //heartbeat
}


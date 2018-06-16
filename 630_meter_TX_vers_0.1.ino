/*
 * 630 meter CW transmitter, Ver 0.1 Copyright 2018 Kevin Loughin, KB9RLW
 * 
 * This program is free software, you can redistribute in and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation.
 * http://www.gnu.org/licenses
 * 
 * This software is provided free of charge but without any warranty.
 * 
 */

// Include the etherkit si5351 library.  You may have to add it using the library
// manager under the sketch/Include Library menu.

#include "si5351.h"
#include "Wire.h"
Si5351 si5351;

// Declare variables we will use in the program

int tail = 0;  // The counter for the keying tail delay
int taildefault = 1000; // Default to 7/10ths second tail, change if you want.
int keyed = 1; //flag to indicate keyed state
int keyedprev = 1; //flag to indicate previous key state after a change
int unkeyflag = 0; //flag for end of timeout switch back to RX
long freq = 47500000; //current frequency.
int rawfreq = 0;
int addvalue = 0;
long prevfreq = 47500000;
long freqread = 47400000; // temp variable for freq calculation
int spot = 1; // spot switch state
int spotprev = 1; //flag to indicate previous spot state

void setup() {
  // Setup the I/O pins and turn the relays off
  pinMode (2, OUTPUT); //RX relay control line
  pinMode (3, OUTPUT); //TR relay control line
  digitalWrite(2, LOW); // turn off relay
  digitalWrite(3, LOW); // turn off relay
  pinMode (6, INPUT); // Key input line
  digitalWrite(6, HIGH); // turn on internal pullup resistor
  pinMode (8, INPUT); // spot switch input line
  digitalWrite(8, HIGH); // turn on internal pullup resistor
  Serial.begin(9600); // enable serial for debugging output

  si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0); //initialize the VFO
  si5351.set_freq(freq, SI5351_CLK0);  //set initial freq
  si5351.set_freq(freq, SI5351_CLK1);
  si5351.output_enable(SI5351_CLK0, 0); //turn off outputs
  si5351.output_enable(SI5351_CLK1, 0);
  
  // Setup the interrupt for the keying  tail timer interrupt routine
   OCR0A = 0xAF;
  TIMSK0 |= _BV(OCIE0A);

}

// interrupt handler routine.  Handles the keying, runs 100 times per second
SIGNAL(TIMER0_COMPA_vect) 
{
  if ( (tail > 0) && keyed ) // If we're in the delay period, count down
   {
    tail--;
   }
   
 
}

// main loop here.  handles spotting, tuning

void loop() {
  
  
  if (!spot) //we're in spotting mode changing frequency
    {
       readtuning(); //call function to read the tuning and set variabl freq.
       si5351.set_freq(freq, SI5351_CLK0);  //set VFOs freq
       si5351.set_freq(freq, SI5351_CLK1);
      
    }
    
  spot = digitalRead(8); // Read the spot switch

  if ( !spot && spotprev ) // did we just press the spot switch?
    {
      si5351.output_enable(SI5351_CLK1, 1); //turn on the spotting vfo
      spotprev = 0; //remember that we turned it on
    }
  if ( spot && !spotprev ) // did we just release the spot switch?
    {
      si5351.output_enable(SI5351_CLK1, 0); //turn off the spotting vf0
      spotprev = 1; //remember that we turned it off
    }

 if ( !tail && unkeyflag ) // timeout counted down, switch back to RX mode
    {
      digitalWrite (3, LOW); // turn off TR relay
      digitalWrite (2, LOW); // reconnect RX output
      unkeyflag = 0; //reset the flag
    }
    
  keyed = digitalRead (6); // Read the state of the key line

  if ( keyed && !keyedprev) // did we just unkey? set the timeout value
    {
      tail = taildefault;
      keyedprev = 1;
     si5351.output_enable(SI5351_CLK0, 0); // turn off VFO
      unkeyflag = 1; // note for timeout that we just unkeyed
    }
    
  if (!keyed && keyedprev) // did we just key down?
    {
      digitalWrite(2, HIGH); // ground RX line
      delay (1); // wait 1 millisecond
      digitalWrite(3, HIGH); // switch TR relay
      delay (1); // wait 1 millisecond
      si5351.output_enable(SI5351_CLK0, 1); //turn on VFO
      keyedprev = 0; //set the flag so we know we keyed last time through.
    }
}

void readtuning() {
  rawfreq = analogRead(A2);
addvalue = (rawfreq * 6.8359375);
freqread = (472000 + addvalue);
if ((freqread - freq) > 50)
  {
    freq = freqread;
  }
if ((freq - freqread) > 50)
  {
    freq = freqread;
  }
freq = freq * 100;
}


/*----------------------------------------------------------------------*
 * Sleep demo for ATtinyX4.                                             *
 * Wire a button from pin D2 (INT0, PB2, DIP pin 5) to ground.          *
 * Wire an LED with an appropriate dropping resistor from pin           *
 * D0 (PB0, DIP pin 2) to ground.                                       *
 * Pushing the button wakes the MCU.                                    *
 * After waking, the MCU flashes the LED, then waits 10 seconds before  *
 * going back to sleep.                                                 *
 *                                                                      *
 * Jack Christensen 04Nov2013                                           *
 *                                                                      *
 * Developed with Arduino 1.0.5.                                        *
 * Test conditions for all results below:                               *
 *   5V or 3.3V regulated power supply                                  *
 *   8MHz system clock (internal RC oscillator)                         *
 *   Fuse bytes (L/H/E): 0xE2 / 0xD5 / 0xFF                             *
 *   Arduino-Tiny core, http://code.google.com/p/arduino-tiny/          *
 *                                                                      *
 * ATtiny84A-PU                                                         *
 *   Vcc=5V:   4.9mA active, 0.1µA power-down.                          *
 *   Vcc=3.3V: 3.1mA active, 0.1µA power-down.                          *
 *                                                                      *
 * This work is licensed under the Creative Commons Attribution-        *
 * ShareAlike 3.0 Unported License. To view a copy of this license,     *
 * visit http://creativecommons.org/licenses/by-sa/3.0/ or send a       *
 * letter to Creative Commons, 171 Second Street, Suite 300,            *
 * San Francisco, California, 94105, USA.                               *
 *----------------------------------------------------------------------*/ 
 
#include <avr/sleep.h>

const int LED_PIN = 0;
const unsigned long KEEP_RUNNING = 10000;    //milliseconds

void setup(void)
{
    //to minimize power consumption while sleeping, output pins must not source
    //or sink any current. input pins must have a defined level; a good way to
    //ensure this is to enable the internal pullup resistors.

    for (byte i=0; i<11; i++) {    //make all pins inputs with pullups enabled
        pinMode(i, INPUT_PULLUP);
    }
    pinMode(LED_PIN, OUTPUT);      //make the led pin an output
    digitalWrite(LED_PIN, LOW);    //drive it low so it doesn't source current
}

void loop(void)
{
    for (byte i=0; i<5; i++) {     //wake up, flash the LED
        digitalWrite(LED_PIN, HIGH);
        delay(100);
        digitalWrite(LED_PIN, LOW);
        delay(100);
    }
    delay(KEEP_RUNNING);           //opportunity to measure active supply current 
    digitalWrite(LED_PIN, HIGH);   //blink LED once before sleeping
    delay(100);
    digitalWrite(LED_PIN, LOW);
    goToSleep();
}

void goToSleep(void)
{
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();
    MCUCR &= ~(_BV(ISC01) | _BV(ISC00));      //INT0 on low level
    GIMSK |= _BV(INT0);                       //enable INT0
    byte adcsra = ADCSRA;                     //save ADCSRA
    ADCSRA &= ~_BV(ADEN);                     //disable ADC
    cli();                                    //stop interrupts to ensure the BOD timed sequence executes as required
    byte mcucr1 = MCUCR | _BV(BODS) | _BV(BODSE);  //turn off the brown-out detector
    byte mcucr2 = mcucr1 & ~_BV(BODSE);
    MCUCR = mcucr1;
    MCUCR = mcucr2;
    sei();                                    //ensure interrupts enabled so we can wake up again
    sleep_cpu();                              //go to sleep
    sleep_disable();                          //wake up here
    ADCSRA = adcsra;                          //restore ADCSRA
}

//external interrupt 0 wakes the MCU
ISR(INT0_vect)
{
    GIMSK = 0;                     //disable external interrupts (only need one to wake up)
}

/*----------------------------------------------------------------------*
 * Sleep demo for ATmega328P.                                           *
 * Wire a button from digital pin 2 (INT0) to ground.                   *
 * Wire an LED with an appropriate dropping resistor from pin D13 to    *
 * ground.                                                              *
 * Pushing the button wakes the MCU.                                    *
 * After waking, the MCU flashes the LED, then waits 10 seconds before  *
 * going back to sleep.                                                 *
 *                                                                      *
 * Jack Christensen 07May2013                                           *
 *                                                                      *
 * Tested with Arduino 1.0.5 and an Arduino Uno.                        *
 * Test conditions for all results below:                               *
 *   5V regulated power supply, fed to the Vin pin                      *
 *   16MHz system clock                                                 *
 *   Fuse bytes (L/H/E): 0xFF / 0xDE / 0x05                             *
 *   Optiboot bootloader                                                *
 *                                                                      *
 * Uno R1                                                               *
 *   38mA active, 26mA with MCU in power-down mode.                     *
 *                                                                      *
 * Uno SMD edition                                                      *
 *   42mA active, 31mA power-down.                                      *
 *                                                                      *
 * Adafruit Boarduino                                                   *
 *   Power select jumper set to "USB", USB (FTDI) not connected.        *
 *   15mA active, 3mA power-down.                                       *
 *                                                                      *
 * Adafruit Boarduino without power LED                                 *
 *   12mA active, 0.1µA power-down.                                     *
 *                                                                      *
 * Breadboarded ATmega328P-PU                                           *
 *   12mA active, 0.1µA power-down.                                     *
 *                                                                      *
 * This work is licensed under the Creative Commons Attribution-        *
 * ShareAlike 3.0 Unported License. To view a copy of this license,     *
 * visit http://creativecommons.org/licenses/by-sa/3.0/ or send a       *
 * letter to Creative Commons, 171 Second Street, Suite 300,            *
 * San Francisco, California, 94105, USA.                               *
 *----------------------------------------------------------------------*/ 
 
#include <avr/sleep.h>

const int LED = 13;                          //LED on pin 13
const unsigned long KEEP_RUNNING = 10000;    //milliseconds

void setup(void)
{
    //to minimize power consumption while sleeping, output pins must not source
    //or sink any current. input pins must have a defined level; a good way to
    //ensure this is to enable the internal pullup resistors.

    for (byte i=0; i<20; i++) {    //make all pins inputs with pullups enabled
        pinMode(i, INPUT_PULLUP);
    }

    pinMode(LED, OUTPUT);          //make the led pin an output
    digitalWrite(LED, LOW);        //drive it low so it doesn't source current
}

void loop(void)
{
    for (byte i=0; i<5; i++) {     //flash the LED
        digitalWrite(LED, HIGH);
        delay(100);
        digitalWrite(LED, LOW);
        delay(100);
    }
    delay(KEEP_RUNNING);           //opportunity to measure active supply current 
    digitalWrite(LED, HIGH);       //one blink before sleeping
    delay(100);
    digitalWrite(LED, LOW);
    goToSleep();
}

void goToSleep(void)
{
    byte adcsra = ADCSRA;          //save the ADC Control and Status Register A
    ADCSRA = 0;                    //disable the ADC
    EICRA = _BV(ISC01);            //configure INT0 to trigger on falling edge
    EIMSK = _BV(INT0);             //enable INT0
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    cli();                         //stop interrupts to ensure the BOD timed sequence executes as required
    sleep_enable();
    //disable brown-out detection while sleeping (20-25µA)
    uint8_t mcucr1 = MCUCR | _BV(BODS) | _BV(BODSE);
    uint8_t mcucr2 = mcucr1 & ~_BV(BODSE);
    MCUCR = mcucr1;
    MCUCR = mcucr2;
    //sleep_bod_disable();           //for AVR-GCC 4.3.3 and later, this is equivalent to the previous 4 lines of code
    sei();                         //ensure interrupts enabled so we can wake up again
    sleep_cpu();                   //go to sleep
    sleep_disable();               //wake up here
    ADCSRA = adcsra;               //restore ADCSRA
}

//external interrupt 0 wakes the MCU
ISR(INT0_vect)
{
    EIMSK = 0;                     //disable external interrupts (only need one to wake up)
}
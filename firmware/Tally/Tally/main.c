/*
 * Tally.c
 *
 * Created: 24/04/2020 13.50.55
 * Author : Mohamad Sigit Dude
 * http://github.com/dudelectric
 */ 
# define F_CPU 1000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include <avr/sleep.h>


#define A3_DDR    DDRA
#define A3_PORT   PORTA
#define A3_PIN    0
#define A3_pin_init()			A3_DDR |= _BV(A3_PIN)
#define A3_pin_low()			A3_PORT &= ~_BV(A3_PIN)
#define A3_pin_high()			A3_PORT |= _BV(A3_PIN)

#define A2_DDR    DDRA
#define A2_PORT   PORTA
#define A2_PIN    1
#define A2_pin_init()			A2_DDR |= _BV(A2_PIN)
#define A2_pin_low()			A2_PORT &= ~_BV(A2_PIN)
#define A2_pin_high()			A2_PORT |= _BV(A2_PIN)

#define A1_DDR    DDRD
#define A1_PORT   PORTD
#define A1_PIN    6
#define A1_pin_init()			A1_DDR |= _BV(A1_PIN)
#define A1_pin_low()			A1_PORT &= ~_BV(A1_PIN)
#define A1_pin_high()			A1_PORT |= _BV(A1_PIN)

//count btn
#define BT1_DDR    DDRD
#define BT1_PORT   PORTD
#define BT1_PIN    PIND
#define BT1_BIT    2
#define bt1_pin_init()					BT1_DDR &= ~_BV(BT1_BIT)
#define bt1_pin_is_low()				(bit_is_clear(BT1_PIN, BT1_BIT))
#define bt1_pin_pullup()				BT1_PORT |= _BV(BT1_BIT)
//reset btn
#define BT2_DDR    DDRD
#define BT2_PORT   PORTD
#define BT2_PIN    PIND
#define BT2_BIT    3
#define bt2_pin_init()					BT2_DDR &= ~_BV(BT2_BIT)
#define bt2_pin_is_low()				(bit_is_clear(BT2_PIN, BT2_BIT))
#define bt2_pin_pullup()				BT2_PORT |= _BV(BT2_BIT)



#define SSegPortInit()			DDRB = 0xFF;
			//			.GFEDCBA
#define num0		  0b10111111//01000000
#define num1		  0b10000110//01111001
#define num2		  0b11011011//00100100
#define num3		  0b11001111//00110000
#define num4		  0b11100110//00011001
#define num5		  0b11101101//00010010
#define num6		  0b11111101//00000010
#define num7		  0b10000111//01111000
#define num8		  0b11111111//00000000
#define num9		  0b11101111//00010000
#define numx		  0b10000000//01111111//clear

uint8_t numArray[] = {num0, num1, num2, num3, num4, num5, num6, num7, num8, num9};	
uint8_t numPtr;
bool fl;
uint16_t ctr;
uint8_t digit = 3;
#define TMOUT_RELOAD_IN_SEC	5
uint8_t tmout_ctr = TMOUT_RELOAD_IN_SEC;

void goToSleep(void)
{
	A1_pin_low();
	A2_pin_low();
	A3_pin_low();
	PORTB = 0x00;
	tmout_ctr = TMOUT_RELOAD_IN_SEC;
	ctr -= 1; 
	/************************/
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	sleep_enable();
	MCUCR &= ~(_BV(ISC11) | _BV(ISC10));      //INT1 on low level
	GIMSK |= _BV(INT1);                       //enable INT1
	sei();                                    //ensure interrupts enabled so we can wake up again
	sleep_cpu();                              //go to sleep
	sleep_disable();                          //wake up here
}

//external interrupt 0 wakes the MCU
ISR(INT1_vect)
{
	GIMSK = 0;                     //disable external interrupts (only need one to wake up)
}
	
//F_T0 = 1MHz/8 = 125khz => 	Periode = 1/125khz = 8us
//untuk overflow dalam 1ms counter harus mendapat clock sebanyak 1ms/8us = 125	
#define TOVO_PRELOAD_1MS  (0xff + 1 - 125) 

void timer0_1ms_ov_init(){
	TCNT0 = TOVO_PRELOAD_1MS;
	TCCR0B = _BV(CS01); //F_T0 = F_CPU/8
	TIMSK |= _BV(TOIE0);//enable int
	sei();
}	

ISR(TIMER0_OVF_vect){
	static uint16_t cnt0 = 0;
	static uint16_t cnt1 = 0;
	
	TCNT0 = TOVO_PRELOAD_1MS;
	
	if(++cnt0 > 2){
		cnt0 = 0;
		A1_pin_high(); //removing shadow fx
		A2_pin_high(); //removing shadow fx
		A3_pin_high(); //removing shadow fx
		if (digit == 1)
		{
			uint8_t na = ((ctr%1000)%100)%10;
			PORTB = numArray[na];
			A2_pin_high();
			A3_pin_high();
			A1_pin_low();//digit1
			digit = 2;
		}
		else if (digit == 2)
		{
			uint8_t na = ((ctr%1000)%100)/10; 
			PORTB = numArray[na];
			A1_pin_high();
			A3_pin_high();
			if (na != 0 || ctr > 10)
			{
				A2_pin_low();//digit2
			}
			digit = 3;
		}
		else if (digit == 3)
		{
			uint8_t na = (ctr%1000)/100;
			PORTB = numArray[na];
			A1_pin_high();
			A2_pin_high();
			if (na != 0 || ctr > 100)
			{
				A3_pin_low();//digit3
			}
			digit = 1;
		}
	}
	
	if(++cnt1 > 1000){
		cnt1 = 0;
		tmout_ctr--;
		if (tmout_ctr == 0){
			goToSleep();
		}
	}
}

int main(void)
{
	bt1_pin_init();
	bt2_pin_init();
	bt1_pin_pullup();
	bt2_pin_pullup();
	A1_pin_init();
	A2_pin_init();
	A3_pin_init();
	A1_pin_low();
	A2_pin_low();
	A3_pin_low();
	SSegPortInit();
	PORTB = numx;
	timer0_1ms_ov_init();

    while (1) 
    {
		if (bt1_pin_is_low())
		{
			ctr = 0;
			_delay_ms(200);
			while (bt1_pin_is_low());
		}
		if (bt2_pin_is_low())
		{
			ctr++;
			if (ctr == 1000){
				ctr = 0;
			}
			tmout_ctr = TMOUT_RELOAD_IN_SEC;
			_delay_ms(200);
			while (bt2_pin_is_low());
		}
		
    }
}


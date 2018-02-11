#ifndef F_CPU
#define F_CPU 16000000UL // or whatever may be your frequency
#endif
 
/* File: main.c */
#include <avr/io.h>
#include <util/delay.h>


int main(void)
{
    DDRB |= (1<<PB7);
    while(1){
            PORTB |= (1<<PB7);
            _delay_ms(500);
            PORTB &= ~(1<<PB7);
            _delay_ms(500);
            }
    return 1;
}

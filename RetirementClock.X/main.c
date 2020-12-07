/*
*
*
*
*/


#define F_CPU 3333333
#define _BV(bit) (1 << (bit))

#include <stdlib.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "lcd.h"

int main(void)
{
    // Set LCD backlight as output
    PORTB.DIRSET = PIN5_bm;   
    // Set buzzer as output
    PORTA.DIRSET = PIN7_bm;
    // Set buttons as input
    PORTA.DIRCLR = PIN4_bm;
    PORTA.DIRCLR = PIN5_bm; 
    
    // Enable pull-up resistor for both buttons
    PORTA.PIN4CTRL = PORT_PULLUPEN_bm;
    PORTA.PIN5CTRL = PORT_PULLUPEN_bm;  
    
    // Left btn triggers an interrupt
    PORTA.PIN4CTRL = PORT_ISC_RISING_gc; 
    
    // Set sleep mode
    set_sleep_mode(SLPCTRL_SMODE_IDLE_gc);
    
    // Initialize LCD
    lcd_init(LCD_DISP_ON);
    PORTB.OUTSET = PIN5_bm;
    lcd_puts("Starting...");
    _delay_ms(1000);
    PORTB.OUTCLR = PIN5_bm;
            
    lcd_clrscr();
    // Enable interrupts
    sei();
    while(1)
    {
        sleep_mode();
    }
}

ISR(PORTA_PORT_vect)
{
    // Clear the interrupt flag
    PORTA.INTFLAGS = PORTA.INTFLAGS;
    // Toggle the LCD backlight
    if (!(PORTA.IN & PIN5_bm))
    {
        PORTB.OUTTGL = PIN5_bm;
    }  
}
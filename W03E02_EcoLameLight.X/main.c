/*
 * File:   main.c
 * 
 * Turns on the LED whenever the button is pressed down.
 * CPU stays in sleep mode unless it's interrupted by a button press.
 * 
 * Author: Santeri Hiltunen
 *
 * Created on 15 November 2020, 20:47
 */


#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

int main(void) 
{
    // Set LED as output
    PORTF.DIRSET = PIN5_bm;
    // Set btn as input
    PORTF.DIRCLR = PIN6_bm; 
    
    // Btn is configured to trigger an interrupt when pressed
    PORTF.PIN6CTRL = PORT_ISC_FALLING_gc;
    // Enable interrupts
    sei();
    
    while (1) 
    {
        // LED will stay on for as long as the btn is pressed
        // after returning from the ISR
        while (!(PORTF.IN & PIN6_bm)) 
        {
            PORTF.OUTCLR = PIN5_bm;
        }
        // Turn LED off as soon as the btn is released
        PORTF.OUTSET = PIN5_bm;
        // Enter sleep mode after each interrupt
        sleep_mode();
    }
}

ISR(PORTF_PORT_vect)
{
    // Clear all interrupt flags
    PORTF.INTFLAGS = 0xFF;
}

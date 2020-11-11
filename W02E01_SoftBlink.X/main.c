/*
 * File:   main.c
 * 
 * A softly blinking LED implementation with a period of around 1 second
 * 
 * Author: Santeri Hiltunen
 *
 * Created on 10 November 2020, 22:03
 */

#define F_CPU   3333333

#include <avr/io.h>
#include <util/delay.h>

// Implements a single PWM cycle
void pwm_period(unsigned int duty) 
{
    // Keeps the LED on for duty value times with a delay
    for (unsigned int i = duty; i > 0x00; --i ) 
    {
        // Turn LED on
        PORTF.OUTCLR = PIN5_bm;
        _delay_us(7);

    }
    // Keeps the LED off for the remaining period (0xFF-duty) with a delay
    for (unsigned int j = (0xFF-duty); j > 0x00; --j )
    {
        // Turn LED off
        PORTF.OUTSET = PIN5_bm;
        _delay_us(7);
    }
    

}

int main(void) 
{
    // Set LED as output
    PORTF.DIRSET = PIN5_bm;
    
    // Duty value determines the LED's brightness level
    unsigned int duty = 0x00;
    
    while (1) 
    {   
        // Increases duty value to max while calling PWM-function
        while (duty < 0xFF)
        {
            ++duty;
            pwm_period(duty);
        }   
        // Decreases duty value to min while calling PWM-function     
        while (duty > 0x00)
        {
            --duty;
            pwm_period(duty);
        }   
                
    }
}



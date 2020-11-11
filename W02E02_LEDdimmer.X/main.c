/*
 * File:   main.c
 * Author: dtek0068
 *
 * Created on 11 November 2020, 14:53
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
    
    // Set button as input
    PORTF.DIRCLR = PIN6_bm;
    
    // Controls LED brightness: 0x00 == MIN, 0xFF == MAX
    unsigned int duty = 0x00;
    
    // Shows whether the LED should be dimming or brightening 
    unsigned int btnDIM = 0;

    while (1) 
    {   
        // Checks if the btn is pressed
        if (!(PORTF.IN & PIN6_bm)) 
        {
            // Brightening sequence keeps checking if the btn is held down
            while (!(PORTF.IN & PIN6_bm) && !btnDIM)
            {
                // Increases brightness to MAX without going over
                if (duty < 0xFF)
                {
                    ++duty;
                }
                pwm_period(duty);
            }
            // Dimming sequence keeps checking if the btn is held down
            while (!(PORTF.IN & PIN6_bm) && btnDIM)
            {
                // Decreases brightness to MIN without going under
                if (duty > 0x00)
                {
                    --duty;
                }
                pwm_period(duty);
            }
            // Flips the state variable for the next btn press sequence
            btnDIM = !btnDIM;
        }
        
        // Keeps the current brightness level when btn is not pressed
        else 
        {
            pwm_period(duty);
        }
        
    }

}

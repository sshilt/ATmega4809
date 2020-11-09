/*
 * Simple LED blinking application for testing that the ATmega4809 PCB
 * can be programmed with the course specific virtual machine.
 */

#define F_CPU   3333333

#include <avr/io.h>
#include <util/delay.h>

int main(void)
{
    // Set PF5 (LED) as out
    PORTF.DIRSET = PIN5_bm;

    // The superloop
    while (1)
    {
        // Toggle LED ON/OFF every 1/2 seconds
<<<<<<< HEAD
        _delay_ms(200);
=======
        _delay_ms(500);
>>>>>>> b9ebdf7edb8848fe2353ff1c8f22cc5547c3984f
        PORTF.OUTTGL = PIN5_bm;
    }
}

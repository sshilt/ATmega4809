/*
 * File:   main.c
 * Author: Santeri Hiltunen
 *
 * Created on 09 November 2020, 14:22
 */


#include <avr/io.h>

int main(void) {
    PORTF.DIRSET = PIN5_bm; // Set PF5 (LED) as output
    PORTF.DIRCLR = PIN6_bm; // Set PF6 (switch) as input
    while (1) {
        if (PORTF.IN & PIN6_bm) {
            PORTF.OUTSET = PIN5_bm; // If button is pressed turn on LED
        }
        else {
            PORTF.OUTCLR = PIN5_bm; // Else keep LED turned off
        }
    }
}

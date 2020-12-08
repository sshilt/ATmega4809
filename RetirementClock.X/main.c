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

void RTC_init(void);
void display_clock(void);
void display_countdown(void);
void display_runtime(void);
void increment_time(void);
void increment_minute(void);
void increment_hour(void);
void increment_day(void);
void increment_month(void);
void increment_year(void);
void retire(void);

// Time keeping variables
volatile uint16_t year = 2020;
volatile uint8_t month = 12;
volatile uint8_t day = 8;
volatile uint8_t hour = 0;
volatile uint8_t minute = 42;
volatile uint8_t second = 0;

// Keeps track of the system runtime (in seconds)
volatile uint32_t runtime = 0;

// Keeps track of the current lcd mode (3 possible ones)
volatile uint8_t lcd_mode = 0;

// Used when turning integer values into displayable arrays
char buffer[7];
// Used to hold a padding integer for the LCD
static char padding[2];

int main(void)
{
    // Initialize the padding array with a 0
    itoa(0, padding, 10);
    
    // Set LCD backlight as output
    PORTB.DIRSET = PIN5_bm;   
    // Set buzzer as output
    PORTA.DIRSET = PIN7_bm;
    // Set button as input
    PORTF.DIRCLR = PIN6_bm;
    
    // Enable pull-up resistor for both buttons
    PORTA.PIN4CTRL = PORT_PULLUPEN_bm;
    PORTA.PIN5CTRL = PORT_PULLUPEN_bm;  
    
    // Btn triggers an interrupt on falling edge
    PORTF.PIN6CTRL = PORT_ISC_FALLING_gc; 
    
    // Set sleep mode
    set_sleep_mode(SLPCTRL_SMODE_IDLE_gc);
    
    // Initialize RTC
    RTC_init();
    
    // Initialize LCD
    lcd_init(LCD_DISP_ON);
    
    // Turn on LCD backlight
    PORTB.OUTSET = PIN5_bm;
            
    lcd_clrscr();
    // Enable interrupts
    sei();
    while(1)
    {
        sleep_mode();
    }
}

ISR(PORTF_PORT_vect)
{
    // Clear the interrupt flag
    PORTF.INTFLAGS = PORTF.INTFLAGS;
    // Change the LCD view
    lcd_mode = ((lcd_mode + 1) % 3);
    
}

ISR(RTC_PIT_vect)
{
    // Clear the interrupt flag
    RTC.PITINTFLAGS = RTC_PI_bm;
    // Increment the system runtime
    runtime++;
    // Increment the time and date variables
    increment_time();
    
    switch (lcd_mode)
    {
        case 0:
            display_clock();
            break;
        case 1:
            display_countdown();
            break;
        case 2:
            display_runtime();
            break;
            
    }
}

void RTC_init(void)
{
    uint8_t temp;

    /* Initialize 32.768kHz Oscillator: */
    /* Disable oscillator: */
    temp = CLKCTRL.XOSC32KCTRLA;
    temp &= ~CLKCTRL_ENABLE_bm;
    /* Enable writing to protected register */
    CPU_CCP = CCP_IOREG_gc;
    CLKCTRL.XOSC32KCTRLA = temp;

    while(CLKCTRL.MCLKSTATUS & CLKCTRL_XOSC32KS_bm)
    {  
        ; /* Wait until XOSC32KS becomes 0 */
    }

    /* SEL = 0 (Use External Crystal): */
    temp = CLKCTRL.XOSC32KCTRLA;
    temp &= ~CLKCTRL_SEL_bm;
    /* Enable writing to protected register */
    CPU_CCP = CCP_IOREG_gc;
    CLKCTRL.XOSC32KCTRLA = temp;

    /* Enable oscillator: */
    temp = CLKCTRL.XOSC32KCTRLA;
    temp |= CLKCTRL_ENABLE_bm;
    /* Enable writing to protected register */
    CPU_CCP = CCP_IOREG_gc;
     CLKCTRL.XOSC32KCTRLA = temp;

     /* Initialize RTC: */
    while (RTC.STATUS > 0)
    {
        ; /* Wait for all register to be synchronized */
    }
    /* 32.768kHz External Crystal Oscillator (XOSC32K) */
    RTC.CLKSEL = RTC_CLKSEL_TOSC32K_gc;
    /* Run in debug: enabled */
    RTC.DBGCTRL = RTC_DBGRUN_bm;

    RTC.PITINTCTRL = RTC_PI_bm; /* Periodic Interrupt: enabled */

    RTC.PITCTRLA = RTC_PERIOD_CYC32768_gc /* RTC Clock Cycles 32768 */
        | RTC_PITEN_bm; /* Enable: enabled */
}

void display_clock(void)
{
    // Clear LCD
    lcd_clrscr();
    // Padding with a 0 if the value takes a single slot
    if (hour < 10)
    {
        lcd_puts(padding);
    }
    itoa(hour, buffer, 10); 
    lcd_puts(buffer);
    lcd_puts(":");
    
    if (minute < 10)
    {
        lcd_puts(padding);
    }
    itoa(minute, buffer, 10);
    lcd_puts(buffer);
    lcd_puts(":");
    
    if (second < 10)
    {
        lcd_puts(padding);
    }    
    itoa(second, buffer, 10);
    lcd_puts(buffer);
    lcd_puts("\n");
    
    itoa(day, buffer, 10);
    lcd_puts(buffer);
    lcd_puts(".");
    
    itoa(month, buffer, 10);
    lcd_puts(buffer);
    lcd_puts(".");   
    
    itoa(year, buffer, 10);
    lcd_puts(buffer);
}
void display_countdown()
{
    // Clear LCD
    lcd_clrscr();
}
void display_runtime()
{
    // Clear LCD
    lcd_clrscr();
    
    itoa(runtime, buffer, 10);
    lcd_puts(buffer);    
    
}

void increment_time(void)
{
    if (second == 59)
    {
        second = 0;
        increment_minute();
    }
    else
    {
        second++;
    }
}

void increment_minute(void)
{
    if ((minute == 59) && (second == 0))
    {
        minute = 00;
        increment_hour();
    }
    else
    {
        minute++;
    }
}

void increment_hour(void)
{
    if ((hour == 23) && (minute == 0) && (second == 0))
    {
        hour = 0;
        increment_day();
    }
    else
    {
        hour++;
    }
}

void increment_day(void)
{
    if ((hour == 0) && (minute == 0) && (second == 0))
    {
        day++;
    }
    else
    {
        
    } 
}

void increment_month(void)
{
    if (second == 59)
    {
        second = 0;
    }
    else
    {
        second++;
    }
}

void increment_year(void)
{
    if (second == 59)
    {
        second = 0;
    }
    else
    {
        second++;
    }
}

void retire(void)
{
    lcd_puts("Go home,\n");
    lcd_puts("old timer!");
    //PORTA.OUTSET = PIN7_bm;
}

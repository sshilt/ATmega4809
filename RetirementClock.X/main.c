/*
* File: main.c
*
*
*/


#define F_CPU 3333333
#define _BV(bit) (1 << (bit))
#define MAX_COMMAND_LEN 32

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "lcd.h"
#include "serial.h"

// Function prototypes
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
void execute_command(char *command);

// Time keeping variables
volatile uint16_t year = 2020;
volatile uint8_t month = 12;
volatile uint8_t day = 31;
volatile uint8_t hour = 23;
volatile uint8_t minute = 59;
volatile uint8_t second = 55;

// Birthday variables
volatile uint16_t birth_year = 1965;
volatile uint8_t birth_month = 12;
volatile uint8_t birth_day = 31;

// Holds the amount of days in each month
static int days_in_month[] = {31,28,31,30,31,30,31,31,30,31,30,31};

// Keeps track of the system runtime (in seconds)
volatile uint32_t runtime = 0;

// Keeps track of the current lcd mode (3 possible ones)
volatile uint8_t lcd_mode = 0;

// Used when turning integer values into displayable arrays
char buffer[32];

// Used to hold a padding value for the LCD
static char padding[2];

// Holds position of the 
volatile uint8_t pos = 0;

int main(void)
{
    // Initialize the padding array with a 0
    sprintf(padding, "%d", 0);
    
    // Set LCD backlight as output
    PORTB.DIRSET = PIN5_bm;   
    // Set buzzer as output
    PORTA.DIRSET = PIN7_bm;
    // Set button as input
    PORTF.DIRCLR = PIN6_bm;
     
    // Btn triggers an interrupt on falling edge
    PORTF.PIN6CTRL = PORT_ISC_FALLING_gc; 
    
    // USART0 triggers an interrupt on receive complete
    USART0.CTRLA = USART_RXCIE_bm;
    
    // Set sleep mode
    set_sleep_mode(SLPCTRL_SMODE_IDLE_gc);
    
    //Initialize USART0
    USART0_init();
    
    // Initialize RTC
    RTC_init();
    
    // Initialize LCD
    lcd_init(LCD_DISP_ON);
    // Clear LCD
    lcd_clrscr();
    // Turn on LCD backlight
    PORTB.OUTSET = PIN5_bm;
   

    // Enable interrupts
    sei();
    // Superloop just enters sleep mode. Everything works on interrupts
    while(1)
    {
        sleep_mode();
    }
}

// Triggered when a byte is received through USART0 (serial console input)
ISR(USART0_RXC_vect)
{
    // Clear the interrupt flag
    USART0.RXDATAH = USART_RXCIF_bm;
    
    char command[MAX_COMMAND_LEN];
    
    char c;
    c = USART0_readChar();
        
    if((c != '\n') && (c != '\r'))
    {
        command[pos++] = c;
        if(pos > MAX_COMMAND_LEN)
        {
            pos = 0;
        }
    }
    // PuTTY console ends lines with '\r' when enter is pressed
    if(c == '\r')
    {
            
        command[pos] = '\0';
        pos = 0;
        execute_command(command);
    }
}

// Triggered on a button press
ISR(PORTF_PORT_vect)
{
    // Clear the interrupt flag
    PORTF.INTFLAGS = PORTF.INTFLAGS;
    // Change the LCD view
    lcd_mode = ((lcd_mode + 1) % 3);
    
}

// Triggered by RTC once a second
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

// Displays a clock and date view
void display_clock(void)
{
    // Clear LCD
    lcd_clrscr();
    // Pad with a 0 if the value has only a single digit
    if (hour < 10)
    {
        lcd_puts(padding);
    }
    // Display hours on top row
    sprintf(buffer, "%d:", hour); 
    lcd_puts(buffer);
    
    if (minute < 10)
    {
        lcd_puts(padding);
    }
    // Display minutes on top row
    sprintf(buffer, "%d:", minute);
    lcd_puts(buffer);
    
    if (second < 10)
    {
        lcd_puts(padding);
    }    
    // Display seconds on top row. Move cursor to next row
    sprintf(buffer, "%d\n", second);
    lcd_puts(buffer);
    
    // Display date on bottom row
    sprintf(buffer, "%d.%d.%d", day, month, year);
    lcd_puts(buffer);
}

// Displays a countdown to retirement
void display_countdown(void)
{
    // Clear LCD
    lcd_clrscr();
    sprintf(buffer, "%d:%d:%d",hour,minute,second);
    lcd_puts(buffer);
}

// Displays how long the system has been running
void display_runtime(void)
{
    // Placeholder for conversions
    uint32_t num = runtime;
    
    // Clear LCD
    lcd_clrscr();
    
    // Calculate and display days. (86400 seconds in a day)
    sprintf(buffer, "%d:", (int)floor(num / 86400));
    lcd_puts(buffer); 
    
    // Calculate and display hours
    num = num % 86400;
    sprintf(buffer, "%d:", (int)floor(num / 3600));
    lcd_puts(buffer);
    
    // Calculate and display minutes
    num %= 3600;
    sprintf(buffer, "%d:", (int)floor(num / 60));
    lcd_puts(buffer);

    // Calculate and display seconds
    num %= 60;
    sprintf(buffer, "%d", (int)floor(num));
    lcd_puts(buffer);  
    
}

/* Time and date incrementation functions.
 * As the seconds are about to "overflow" to 60,
 * they are reset to 0 and increment_minute() is called. 
 * This method is repeated up to year increments
 */
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
    if (minute == 59)
    {
        minute = 0;
        increment_hour();
    }
    else
    {
        minute++;
    }
}

void increment_hour(void)
{
    if (hour == 23)
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
    // Check if it's the last day of the month. > handles February 29th
    if (day >= (days_in_month[month - 1]))
    {
        // If it's leap year and it's February 28th, increment date to 29
        if ((((month == 2) && (day == 28)) && ((((year % 400) == 0)
                || ((year % 100) != 0)) && ((year % 4) == 0))))
        {
            day++;
        }
        else
        {
            day = 1;
            increment_month();
        }

    }
    else
    {
        day++;
    } 
}

void increment_month(void)
{
    if (month == 12)
    {
        month = 1;
        increment_year();
    }
    else
    {
        month++;
    }
}

void increment_year(void)
{
    year++;
}

void retire(void)
{
    lcd_puts("Go home,\n");
    lcd_puts("old timer!");
    //PORTA.OUTSET = PIN7_bm;
}

void execute_command(char *command)
{
    if(strncmp(command, "SET DATETIME", 12) == 0)
    {
        char delim[] = " ";

        char *ptr = strtok(command, delim);

        while(ptr != NULL)
        {
            uint16_t num;
            sscanf(ptr, "%d", &num);
            year = num;
            ptr = strtok(NULL, delim);
        }
    }
    // Print date and time in the serial console
    else if (strcmp(command, "GET DATETIME") == 0)
    {
        char datetime[33];
        
        sprintf(datetime, "%d.%d.%d %d:%d:%d\r\n",
                day,month,year,hour,minute,second);
        
        USART0_sendString(datetime);
    } 
    else if (strcmp(command, "TGL BACKLIGHT") == 0)
    {
        PORTB.OUTTGL = PIN5_bm;
        USART0_sendString("BACKLIGHT TOGGLED.\r\n");
    }     
    else 
    {
        USART0_sendString("Incorrect command.\r\n");
    }
}

/*
 * File: main.c
 * 
 * Author: Santeri Hiltunen <sshilt@utu.fi>
 * 
 * Runs a 16x2 LCD, an active buzzer and a button.
 * Uses RTC to generate an interrupt every second that changes the 
 * time and date variables displayed on the screen. LCD has 3 modes:
 * clock and date view, retirement date view, and system runtime view.
 * Button changes between the modes. 
 * Implements accurate time keeping including leap year calculations.
 * 
 * When retirement age is reached, buzzer will sound and a message is displayed.
 * System is reset by changing the time in the console or disconnecting the
 * device.
 * 
 * Commands have been configured to be used by PuTTY with default settings.
 * Implements serial commands:
 *   GET DATETIME
 *   SET DATETIME dd mm yyyy hh mm ss
 *   GET BIRTHDAY
 *   SET BIRTDAY dd mm yyyy
 *   TGL BACKLIGHT
 * 
 * 7.12.2020: Basic LCD functionality.
 * 9.12.2020: Complete time keeping.
 * 13.12.2020: Serial interface functionality.
 * 16.12.2020: Optimizations. Retirement alert functional.
 */

#define F_CPU 3333333
#define MAX_COMMAND_LEN 32 // Max serial command length
#define RETIREMENT_AGE 65

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
static inline void increment_time(void);
static inline void increment_minute(void);
static inline void increment_hour(void);
static inline void increment_day(void);
static inline void increment_month(void);
static inline void increment_year(void);
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

// Holds the number of days in each month
static int days_in_month[] = {31,28,31,30,31,30,31,31,30,31,30,31};

// Keeps track of the system runtime (in seconds)
volatile uint32_t runtime = 0;

// Keeps track of the current lcd mode (3 possible ones)
volatile uint8_t lcd_mode = 0;

// Holds position of the array index when building command strings
volatile uint8_t pos = 0;

// Used to hold a padding value for the LCD
static char padding[2];

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
    
    // Initialize LCD
    lcd_init(LCD_DISP_ON);
    // Clear LCD
    lcd_clrscr();
    // Turn on LCD backlight
    PORTB.OUTSET = PIN5_bm;
    
    //Initialize USART0
    USART0_init();
    
    // Initialize RTC
    RTC_init();
           
    // Enable interrupts
    sei();

    // Superloop enters sleep mode
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
    
    // Build the command array
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
    
    // Check if it's time to retire. Exit the ISR if it is
    if (year >= (birth_year + RETIREMENT_AGE))
    {
        if (year > (birth_year + RETIREMENT_AGE))
        {
            retire();
            return;
        }
        else if (month >= birth_month)
        {
            if (day >= birth_day)
            {
                retire();
                return;
            }
        }
    }
    // Turn buzzer off
    PORTA.OUTCLR = PIN7_bm;
    // Enter the appropriate time showing function
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
// RTC initialization. Example code from Microchip's repo
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

// Displays a time and date view
void display_clock(void)
{
    // Holds time and date variables
    char buffer[16];
    
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

// Displays retirement date. TODO: countdown to retirement
void display_countdown(void)
{
    // Holds time and date variables
    char buffer[16];
    // Clear LCD
    lcd_clrscr();
    sprintf(buffer, "%d.%d.%d",
            birth_day, birth_month, birth_year + RETIREMENT_AGE);
    lcd_puts(buffer);
    lcd_puts("\nRetirement date");
}

// Display how long the system has been running
void display_runtime(void)
{
    // Holds time and date variables
    char buffer[8];
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
    lcd_puts("\nSystem runtime");
}

/* Time and date incrementation functions.
 * As the seconds are about to "overflow" to 60,
 * they are reset to 0 and increment_minute() is called. 
 * This method is repeated up to year increments
 */
static inline void increment_time(void)
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

static inline void increment_minute(void)
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

static inline void increment_hour(void)
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

static inline void increment_day(void)
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

static inline void increment_month(void)
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

static inline void increment_year(void)
{
    year++;
}

void retire(void)
{
    lcd_clrscr();
    
    lcd_gotoxy(4,0);
    lcd_puts("Go home,");
    
    lcd_gotoxy(3,1);
    lcd_puts("old timer!");
    PORTA.OUTSET = PIN7_bm;
}

// Execute serial terminal commands
void execute_command(char *command)
{
    /*
     * Set date and time. Compare first 12 symbols of the command.
     * Syntax is "SET DATETIME dd mm yyyy hh mm ss".
     * Incorrect syntax will print garbage values to LCD.
     */
    if (strncmp(command, "SET DATETIME", 12) == 0)
    {
        char delim[] = " ";
        char *saveptr;
        char *ptr = strtok_r(command, delim, &saveptr);
        
        uint8_t count = 0;
        
        while(ptr != NULL)
        {
            /*
             * Values of the split words are saved as integers.
             * First 2 words (SET and DATETIME) are saved but overwritten
             * and never used.
             */
            uint16_t num;
            sscanf(ptr, "%d", &num);
            
            // Set values from 3rd word onwards (time and date values)
            switch (count)
            {
                case 2:
                    day = num;
                    break;
                case 3:
                    month = num;
                    break;
                case 4:
                    year = num;
                    break;
                case 5:
                    hour = num;
                    break;
                case 6:
                    minute = num;
                    break;
                case 7:
                    second = num;
                    break;
            }
            // Move onto next case
            count++;
            // Save next token to ptr
            ptr = strtok_r(NULL, delim, &saveptr);
        }
    }
    // Print date and time in the serial console
    else if (strcmp(command, "GET DATETIME") == 0)
    {
        char buffer[33];
        sprintf(buffer, "%d.%d.%d %d:%d:%d\r\n",
                day,month,year,hour,minute,second);
        
        USART0_sendString(buffer);
    }
    /*
     * Set birthday. Compare first 12 symbols of the command.
     * Syntax is "SET BIRTHDAY dd mm yyyy".
     * Incorrect syntax will print garbage values to LCD.
     * Works identically to the SET DATETIME -command with fewer arguments
     */    
    else if(strncmp(command, "SET BIRTHDAY", 12) == 0)
    {
        char delim[] = " ";
        char *saveptr;
        char *ptr = strtok_r(command, delim, &saveptr);
        
        uint8_t count = 0;
        
        while(ptr != NULL)
        {
            uint16_t num;
            sscanf(ptr, "%d", &num);
            
            switch (count)
            {
                case 2:
                    birth_day = num;
                    break;
                case 3:
                    birth_month = num;
                    break;
                case 4:
                    birth_year = num;
                    break;
            }
            count++;
            ptr = strtok_r(NULL, delim, &saveptr);
        }
    }
    // Print birthday to the serial console
    else if (strcmp(command, "GET BIRTHDAY") == 0)
    {
        char buffer[33];
        sprintf(buffer, "%d.%d.%d\r\n",
                birth_day, birth_month, birth_year);
        
        USART0_sendString(buffer);
    }    
    // Toggle the LED backlight bits
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
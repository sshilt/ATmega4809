/* 
 * File: serial.h    
 * Header file for serial.c functions
 */

void USART0_init(void);
void USART0_sendChar(char c);
void USART0_sendString(char *str);
char USART0_readChar(void);
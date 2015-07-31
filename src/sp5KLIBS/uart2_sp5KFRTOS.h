/*------------------------------------------------------------------------------------
 * uart2_sp5KFRTOS.h
 * Autor: Pablo Peluffo @ 2015
 * Basado en Proycon AVRLIB de Pascal Stang.
 *
 * Son funciones que impelementan los drivers de las 2 uarts del ATmega1284P, adaptado al sistema SP5K
 * y al FRTOS.
 * Por ser drivers, no deberian accederse a estas funciones directamente sino que se hace por medio de
 * las API de rprintf.
 * El unico acceso que se debe hacer es para inicializarlo por medio de las funciones de init.
 *
*/

#include <avr/interrupt.h>
#include <avr/io.h>

#include "avrlibdefs.h"
#include "avrlibtypes.h"
#include "global.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"


#ifndef UART2_FRTOS_H_
#define UART2_FRTOS_H_

//------------------------------------------------------------------------------------
// UARTs

#define UART0	0
#define UART1	1

#define GPRS_UART	0
#define CMD_UART 	1

#define UARTCTL_PORT	PORTD
#define UARTCTL_PIN		PIND
#define UARTCTL			4
#define UARTCTL_DDR		DDRD
#define UARTCTL_MASK	0x10

#define serBAUD_DIV_CONSTANT			( ( unsigned portLONG ) 16 )

/* Tamao de las colas de la UART */
#define uart0RXBUFFER_LEN	( ( u16 ) ( 640 ))
#define uart0TXBUFFER_LEN	( ( u08 ) ( 128 ))
#define uart1RXBUFFER_LEN	( ( u08 ) ( 128 ))
#define uart1TXBUFFER_LEN	( ( u08 ) ( 128 ))

/* Constants for writing to UCSRB. */
#define serRX_INT_ENABLE				( ( unsigned portCHAR ) 0x80 )
#define serRX_ENABLE					( ( unsigned portCHAR ) 0x10 )
#define serTX_INT_ENABLE				( ( unsigned portCHAR ) 0x20 )
#define serTX_ENABLE					( ( unsigned portCHAR ) 0x08 )

/* Constants for writing to UCSRC. */
#define serUCSRC_SELECT					( ( unsigned portCHAR ) 0x00 )
#define serEIGHT_DATA_BITS				( ( unsigned portCHAR ) 0x06 )

#define UART_DEFAULT_BAUD_RATE		9600	///< default baud rate for UART0

typedef struct {
	u08 buffer[uart0RXBUFFER_LEN];
	u16 head;
	u16 tail;
	u16 count;
} gprsRXbuffer_struct;

/*------------------------------------------------------------------------------------*/

#define vUart0InterruptOn()									\
{															\
	unsigned portCHAR ucByte;								\
															\
	ucByte = UCSR0B;										\
	ucByte |= serTX_INT_ENABLE;								\
	UCSR0B = ucByte;										\
}
/*------------------------------------------------------------------------------------*/

#define vUart1InterruptOn()									\
{															\
	unsigned portCHAR ucByte;								\
															\
	ucByte = UCSR1B;										\
	ucByte |= serTX_INT_ENABLE;								\
	UCSR1B = ucByte;										\
}

/*------------------------------------------------------------------------------------*/

#define vUart0InterruptOff()								\
{															\
	unsigned portCHAR ucInByte;								\
															\
	ucInByte = UCSR0B;										\
	ucInByte &= ~serTX_INT_ENABLE;							\
	UCSR0B = ucInByte;										\
}
/*------------------------------------------------------------------------------------*/

#define vUart1InterruptOff()								\
{															\
	unsigned portCHAR ucInByte;								\
															\
	ucInByte = UCSR1B;										\
	ucInByte &= ~serTX_INT_ENABLE;							\
	UCSR1B = ucInByte;										\
}
/*------------------------------------------------------------------------------------*/

void uartsHWEnable(void);
void uartsHWDisable(void);

void xUartEnable(u08 nUart);
void xUart0Enable(void);
void xUart1Enable(void);

void xUartDisable(u08 nUart);
void xUart0Disable(void);
void xUart1Disable(void);

void xUartInit( u08 nUart, u32 baudrate);
void xUart0Init( u32 baudrate);
void xUart1Init( u32 baudrate);

s08 xUartPutChar( u08 nUart, u08 cOutChar, u08 xBlockTime );
s08 xUart0PutChar( u08 cOutChar, u08 xBlockTime );
s08 xUart1PutChar( u08 cOutChar, u08 xBlockTime );

s08 xUartGetChar(u08 nUart, u08 *pcRxedChar,u08 xBlockTime );
s08 xUart0GetChar(u08 *pcRxedChar,u08 xBlockTime );
s08 xUart1GetChar(u08 *pcRxedChar,u08 xBlockTime );

s08 uartQueueIsEmpty(u08 nUart);
s08 uart0QueueIsEmpty(void);
s08 uart1QueueIsEmpty(void);

void xUartQueueFlush(u08 nUart);
void xUart0QueueFlush(void);
void xUart1QueueFlush(void);

char *getGprsRxBufferPtr(void);
void DEBUG_getParams( u16 *head, u16 *tail, u16 *count);

#endif /* UART2_FRTOS_H_ */

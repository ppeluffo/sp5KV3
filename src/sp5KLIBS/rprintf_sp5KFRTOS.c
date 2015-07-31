

#include "rprintf_sp5KFRTOS.h"

/*------------------------------------------------------------------------------------*/
void rprintfInit(u08 nUart)
{
	if ( nUart == 0 ) {
		GPRSrprintfInit();
		return;
	}

	if ( nUart == 1 ) {
		TERMrprintfInit();
		return;
	}
}
/*------------------------------------------------------------------------------------*/
void GPRSrprintfInit(void)
{

	// Creo el semaforo
	semph_GPRS = xSemaphoreCreateMutex();
}
/*------------------------------------------------------------------------------------*/
void TERMrprintfInit(void)
{

	// Creo el semaforo
	semph_TERM = xSemaphoreCreateMutex();
}
/*------------------------------------------------------------------------------------*/
void rprintfChar(u08 nUart, unsigned char c)
{
	// send a character/byte to the current output device
	// do LF -> CR/LF translation
	if ( nUart == 0 ) {
		GPRSrprintfChar( c );
		return;
	}

	if ( nUart == 1 ) {
		TERMrprintfChar( c );
		return;
	}

	return;
}
/*------------------------------------------------------------------------------------*/
void GPRSrprintfChar(unsigned char c)
{
	// send a character/byte to the current output device
	// do LF -> CR/LF translation
	while ( xSemaphoreTake( semph_GPRS, ( TickType_t ) 10 ) != pdTRUE )
		taskYIELD();

	if(c == '\n')
		xUart0PutChar('\r', ( TickType_t ) 10);

	// send character
	xUart0PutChar( c, ( TickType_t ) 10);

	xSemaphoreGive( semph_GPRS );
	return;
}
/*------------------------------------------------------------------------------------*/
void pvGPRSrprintfChar(unsigned char c)
{
	// send a character/byte to the current output device
	// do LF -> CR/LF translation
	if(c == '\n')
		xUart0PutChar('\r', ( TickType_t ) 10);

	// send character
	xUart0PutChar( c, ( TickType_t ) 10);
	return;
}
/*------------------------------------------------------------------------------------*/
void pvGPRSoutChar(unsigned char c)
{
	// send character
	xUart0PutChar( c, ( TickType_t ) 10);
}
/*------------------------------------------------------------------------------------*/
void TERMrprintfChar(unsigned char c)
{
	// send a character/byte to the current output device
	// do LF -> CR/LF translation

	while ( xSemaphoreTake( semph_TERM, ( TickType_t ) 10 ) != pdTRUE )
		taskYIELD();

	if(c == '\n')
		xUart1PutChar('\r', ( TickType_t ) 10);

	// send character
	xUart1PutChar( c, ( TickType_t ) 10);

	xSemaphoreGive( semph_TERM );
	return;
}
/*------------------------------------------------------------------------------------*/
void pvTERMrprintfChar(unsigned char c)
{
	// send a character/byte to the current output device
	// do LF -> CR/LF translation
	if(c == '\n')
		xUart1PutChar('\r', ( TickType_t ) 10);

	// send character
	xUart1PutChar( c, ( TickType_t ) 10);
	return;
}
/*------------------------------------------------------------------------------------*/
void pvTERMoutChar(unsigned char c)
{
	// send character
	xUart1PutChar( c, ( TickType_t ) 10);
}
/*------------------------------------------------------------------------------------*/
void rprintfStr(u08 nUart, char str[])
{
	// prints a null-terminated string stored in RAM
	// send a string stored in RAM
	// check to make sure we have a good pointer
	if (!str) return;

	if ( nUart == 0 ) {
		GPRSrprintfStr( str );
		return;
	}

	if ( nUart == 1 ) {
		TERMrprintfStr( str );
		return;
	}

	return;
}
/*------------------------------------------------------------------------------------*/
void GPRSrprintfStr( char str[])
{
	// prints a null-terminated string stored in RAM
	// send a string stored in RAM
	// check to make sure we have a good pointer
	if (!str) return;

	while ( xSemaphoreTake( semph_GPRS, ( TickType_t ) 10 ) != pdTRUE )
		taskYIELD();

	// print the string until a null-terminator
	while (*str)
		pvGPRSrprintfChar( *str++);

	xSemaphoreGive( semph_GPRS );

}
/*------------------------------------------------------------------------------------*/
void TERMrprintfStr( char str[])
{
	// prints a null-terminated string stored in RAM
	// send a string stored in RAM
	// check to make sure we have a good pointer
	if (!str) return;

	while ( xSemaphoreTake( semph_TERM, ( TickType_t ) 10 ) != pdTRUE )
		taskYIELD();

	// print the string until a null-terminator
	while (*str)
		pvTERMrprintfChar( *str++);

	xSemaphoreGive( semph_TERM );
}
/*------------------------------------------------------------------------------------*/
void rprintfProgStr(u08 nUart, const prog_char str[])
{
	// prints a null-terminated string stored in program ROM
	// print a string stored in program memory

	if ( nUart == 0 ) {
		GPRSrprintfProgStr(str);
	}

	if ( nUart == 1 ) {
		TERMrprintfProgStr(str);
	}

}
/*------------------------------------------------------------------------------------*/
void GPRSrprintfProgStr( const prog_char str[])
{
	// prints a null-terminated string stored in program ROM
	// print a string stored in program memory
	register char c;

	// check to make sure we have a good pointer
	if (!str) return;

	while ( xSemaphoreTake( semph_GPRS, ( TickType_t ) 10 ) != pdTRUE )
		taskYIELD();

	// print the string until the null-terminator
	while((c = pgm_read_byte(str++)))
		pvGPRSrprintfChar( c );

	xSemaphoreGive( semph_GPRS );
}
/*------------------------------------------------------------------------------------*/
void TERMrprintfProgStr( const prog_char str[])
{
	// prints a null-terminated string stored in program ROM
	// print a string stored in program memory
	register char c;

	// check to make sure we have a good pointer
	if (!str) return;

	while ( xSemaphoreTake( semph_TERM, ( TickType_t ) 10 ) != pdTRUE )
		taskYIELD();

	// print the string until the null-terminator
	while((c = pgm_read_byte(str++)))
		pvTERMrprintfChar( c );

	xSemaphoreGive( semph_TERM );
}
/*------------------------------------------------------------------------------------*/
void rprintfCRLF(u08 nUart)
{
	// prints carriage return and line feed
	// print CR/LF
	// LF -> CR/LF translation built-in to rprintfChar()
	if ( nUart == 0 ) {
		GPRSrprintfCRLF();
	}

	if ( nUart == 1 ) {
		TERMrprintfCRLF();
	}
}
/*------------------------------------------------------------------------------------*/
void GPRSrprintfCRLF(void)
{
	// prints carriage return and line feed
	// print CR/LF
	// LF -> CR/LF translation built-in to rprintfChar()
	GPRSrprintfChar('\n');
}
/*------------------------------------------------------------------------------------*/
void TERMrprintfCRLF(void)
{
	// prints carriage return and line feed
	// print CR/LF
	// LF -> CR/LF translation built-in to rprintfChar()
	TERMrprintfChar( '\n');
}
/*------------------------------------------------------------------------------------*/


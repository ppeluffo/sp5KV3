/*
 *
 *  Created on: 26/3/2015
 *      Author: pablo
 *
 *  Funciones de manejo de las 2 uarts en ATmega1284 con FRTOS8.
 *  - Las funciones son reentrantes.
 *  - TRASMISION:
 *    Cuando se quiere trasmitir, por medio de la funcion xUartPutChar se escribe el
 *    byte a trasmitir en la cola de trasmision y se inicia la misma prendiendo la
 *    interrupcion de trasmision. Esta saca el byte de la cola y lo pone en la uart.
 *  - RECEPCION:
 *    Cuando la UART recibe un dato, la rutina de interrupcion lo escribe en la cola
 *    de recepcion.
 *    Cuando queremos leer, accedemos a la cola con xUartGetChar.
 *
 *
 *        xUartGetChar    ----------   ISR RX  -----------
 *    <------------------| RXqueue | <---------| UART RX |
 *                       -----------           -----------
 *        xUartPutChar   -----------   ISR TX  -----------
 *    ------------------>| TXqueue | --------->| UART TX |
 *                       -----------           -----------
 *
 *  En el caso de la uart del GPRS, en vez de usar una queue vamos a usar un buffer lineal
 *  propio. Esto es porque el tamano es bastante grande y por un lado no quiero repetir los
 *  datos a otro buffer propio del tkGprs, y por otro lado por lo que demora la copia.
 *  La operariva es simple: Cuando se llena, no escribo nada mas. Debo resetearlo.
 *  No devulevo 1 valor sino todos el buffer.
 *
 */
/*------------------------------------------------------------------------------------*/

#include "uart2_sp5KFRTOS.h"

/*------------------------------------------------------------------------------------*/

xQueueHandle xUart0CharsForTxQueue;
gprsRXbuffer_struct xUart0RxedCharsBuffer;

xQueueHandle xUart1CharsForTxQueue, xUart1RxedCharsQueue;

/*------------------------------------------------------------------------------------*/
void fifo_init ( gprsRXbuffer_struct *rb);
void fifo_put ( gprsRXbuffer_struct *rb, const unsigned char c);
void fifo_flush ( gprsRXbuffer_struct *rb, const s08 clearBuffer);
s08 fifo_full ( gprsRXbuffer_struct *rb);
s08 fifo_empty ( gprsRXbuffer_struct *rb);

/*------------------------------------------------------------------------------------*/

void fifo_init ( gprsRXbuffer_struct *rb)
{
	memset (rb, 0, sizeof(*rb) );
	rb->head = 0;
	rb->tail = 0;
	rb->count = 0;
}
/*------------------------------------------------------------------------------------*/

s08 fifo_empty ( gprsRXbuffer_struct *rb)
{
	return ( 0 == rb->count );
}
/*------------------------------------------------------------------------------------*/

s08 fifo_full ( gprsRXbuffer_struct *rb)
{
	return ( rb->count >= uart0RXBUFFER_LEN );
}
/*------------------------------------------------------------------------------------*/

void fifo_put ( gprsRXbuffer_struct *rb, const unsigned char c )
{
	// Si el buffer esta lleno, descarto el valor recibido
	// Guardo en el lugar apuntado por head y avanzo.

	if ( rb->count < uart0RXBUFFER_LEN )
    {
		rb->buffer[rb->head] = c;
		++rb->count;
		++rb->head;
    }
}
/*------------------------------------------------------------------------------------*/

void fifo_flush ( gprsRXbuffer_struct *rb, const s08 clearBuffer)
{
	rb->head = 0;
	rb->tail = 0;
	rb->count = 0;

	if (clearBuffer) {
		memset (rb->buffer, '\0', sizeof (rb->buffer));
	}
}
/*------------------------------------------------------------------------------------*/

char *getGprsRxBufferPtr(void)
{
	xUart0RxedCharsBuffer.buffer[uart0RXBUFFER_LEN - 1] = '\0';
	return( xUart0RxedCharsBuffer.buffer );
}
/*------------------------------------------------------------------------------------*/
void uartsHWEnable(void)
{
	// Habilita el pin que activa el buffer L365
	// Pin UARTCTL -> OUT
	sbi(UARTCTL_DDR, UARTCTL);
	cbi(UARTCTL_PORT, UARTCTL);

}
//------------------------------------------------------------------------------------
void uartsHWDisable(void)
{
	// Deshabilita el pin que activa el buffer L365
	sbi(UARTCTL_PORT, UARTCTL);

}
//------------------------------------------------------------------------------------
void xUartEnable(u08 nUart)
{
	if (nUart == 0) {
		xUart0Enable();
		return;
	}

	if (nUart == 1) {
		xUart1Enable();
		return;
	}
}
/*------------------------------------------------------------------------------------*/
void xUart0Enable(void)
{
	UCSR0B |= BV(RXCIE0)| BV(RXEN0)| BV(TXEN0);
	return;
}
/*------------------------------------------------------------------------------------*/
void xUart1Enable(void)
{
	UCSR1B |= BV(RXCIE1)| BV(RXEN1)| BV(TXEN1);
	return;
}
/*------------------------------------------------------------------------------------*/
void xUartDisable(u08 nUart)
{
	if (nUart == 0) {
		xUart0Disable();
		return;
	}

	if (nUart == 1) {
		xUart1Disable();
		return;
	}
}
/*------------------------------------------------------------------------------------*/
void xUart0Disable(void)
{

	UCSR0B &= ~BV(RXCIE0) & ~BV(RXEN0) & ~BV(TXEN0);
	return;

}
/*------------------------------------------------------------------------------------*/
void xUart1Disable(void)
{

	UCSR1B &= ~BV(RXCIE1) & ~BV(RXEN1) & ~BV(TXEN1);
	return;

}
/*------------------------------------------------------------------------------------*/
void xUartInit( u08 nUart, u32 baudrate)
{
	if (nUart == 0) {
		xUart0Init( baudrate );
		return;
	}
	if (nUart == 1) {
		xUart1Init( baudrate );
		return;
	}
}
/*------------------------------------------------------------------------------------*/
void xUart0Init( u32 baudrate)
{
/* Inicializo el UART para 9600N81.
 * y creo las colas de TX y RX las cuales van a ser usadas por los
 * handlers correspondientes.
 *
 * El valor del divisor se calcula para un clock de 8Mhz.
 * Usamos siempre el doblador de frecuencia por lo que U2Xn = 1;
 *
*/
u16 bauddiv;
s08 doubleSpeed = FALSE;

	uartsHWEnable();	// Habilito el buffer L365.

	portENTER_CRITICAL();

	switch (baudrate) {
	case 9600:
		bauddiv = 103;
		doubleSpeed = TRUE;
		break;
	case 115200:
		bauddiv = 8;
		doubleSpeed = TRUE;
		break;
	}

	// Creo las colas de TX y RX
	fifo_init(&xUart0RxedCharsBuffer);
	xUart0CharsForTxQueue = xQueueCreate( uart0TXBUFFER_LEN, ( unsigned portBASE_TYPE ) sizeof( signed portCHAR ) );
	//
	outb(UBRR0L, bauddiv);
	outb(UBRR0H, bauddiv>>8);
	// Enable the Rx interrupt.  The Tx interrupt will get enabled
	// later. Also enable the Rx and Tx.
	// enable RxD/TxD and interrupts
	outb(UCSR0B, BV(RXCIE0)| BV(RXEN0)|BV(TXEN0));

	// Set the data bits to 8N1.
	UCSR0C = ( serUCSRC_SELECT | serEIGHT_DATA_BITS );

	// 	DOUBLE SPEED
	if (doubleSpeed) {
		outb(UCSR1A, BV(U2X0));
	}

	portEXIT_CRITICAL();

	return;
}
/*------------------------------------------------------------------------------------*/
void xUart1Init( u32 baudrate)
{

u16 bauddiv;
s08 doubleSpeed = FALSE;

	uartsHWEnable();	// Habilito el buffer L365.

	portENTER_CRITICAL();

	switch (baudrate) {
	case 9600:
		bauddiv = 103;
		doubleSpeed = TRUE;
		break;
	case 115200:
		bauddiv = 8;
		doubleSpeed = TRUE;
		break;
	}

	xUart1RxedCharsQueue = xQueueCreate( uart1RXBUFFER_LEN, ( unsigned portBASE_TYPE ) sizeof( signed portCHAR ) );
	xUart1CharsForTxQueue = xQueueCreate( uart1TXBUFFER_LEN, ( unsigned portBASE_TYPE ) sizeof( signed portCHAR ) );
	//
	outb(UBRR1L, bauddiv);
	outb(UBRR1H, bauddiv>>8);
	// Enable the Rx interrupt.  The Tx interrupt will get enabled
	// later. Also enable the Rx and Tx.
	// enable RxD/TxD and interrupts
	outb(UCSR1B, BV(RXCIE1)| BV(RXEN1)|BV(TXEN1));
	// Set the data bits to 8N1.
	UCSR1C = ( serUCSRC_SELECT | serEIGHT_DATA_BITS );

	if (doubleSpeed) {
		outb(UCSR1A, BV(U2X1));
	}

	portEXIT_CRITICAL();

	return;
}
/*------------------------------------------------------------------------------------*/
ISR( USART0_UDRE_vect )
{
/* Handler (ISR) de TX0.
 * El handler maneja la trasmision de datos por la uart0.
 * Para trasmitir, usando la funcion xUart0PutChar se encolan los datos y prende la
 * flag de interrupcion.
 * La rutina de interrupcion ejecuta este handler (SIGNAL) en el cual si hay
 * datos en la cola los extrae y los trasmite.
 * Si la cola esta vacia (luego del ultimo) apaga la interrupcion.
*/

s08 cTaskWoken;
u08 cChar;

	if( xQueueReceiveFromISR( xUart0CharsForTxQueue, &cChar, &cTaskWoken ) == pdTRUE )
	{
		/* Send the next character queued for Tx. */
		outb (UDR0, cChar);
	} else {
		/* Queue empty, nothing to send. */
		vUart0InterruptOff();
	}
}
/*------------------------------------------------------------------------------------*/
ISR( USART1_UDRE_vect )
{
	// Handler (ISR) de TX.
s08 cTaskWoken;
u08 cChar;

	if( xQueueReceiveFromISR( xUart1CharsForTxQueue, &cChar, &cTaskWoken ) == pdTRUE )
	{
		/* Send the next character queued for Tx. */
		outb (UDR1, cChar);
	} else {
		/* Queue empty, nothing to send. */
		vUart1InterruptOff();
	}
}
/*------------------------------------------------------------------------------------*/
s08 xUartPutChar( u08 nUart, u08 cOutChar, u08 xBlockTime )
{
/* Esta funcion se usa para trasmitir por el puerto serial.
 * Si puede encolarlos lo hace y luego prende la flag de TXporInterrupcion para que
 * sea el handler de TX quien vacie la cola y saque los datos por el puerto UART0
*/
s08 res = pdFAIL;

	if (nUart == 0) {
		res = xUart0PutChar( cOutChar, xBlockTime );
		return(res);
	}

	if (nUart == 1) {
		res = xUart1PutChar( cOutChar, xBlockTime );
		return(res);
	}

	return(res);
}
/*------------------------------------------------------------------------------------*/
s08 xUart0PutChar( u08 cOutChar, u08 xBlockTime )
{
/* Esta funcion se usa para trasmitir por el puerto serial 0.
 * Si puede encolarlos lo hace y luego prende la flag de TXporInterrupcion para que
 * sea el handler de TX quien vacie la cola y saque los datos por el puerto UART0

*/
	// Controlo que halla espacio en la cola.
	while  ( uxQueueSpacesAvailable( xUart0CharsForTxQueue ) < 2 )
		taskYIELD();

	//if ( uxQueueMessagesWaiting( xUart0CharsForTxQueue ) > ( uart0TXBUFFER_LEN - 2) )
	//	taskYIELD();

	if( xQueueSend( xUart0CharsForTxQueue, &cOutChar, xBlockTime ) != pdPASS ) {
		return pdFAIL;
	}

	vUart0InterruptOn();
	return pdPASS;
}
/*------------------------------------------------------------------------------------*/
s08 xUart1PutChar( u08 cOutChar, u08 xBlockTime )
{
/* Esta funcion se usa para trasmitir por el puerto serial 0.
 * Si puede encolarlos lo hace y luego prende la flag de TXporInterrupcion para que
 * sea el handler de TX quien vacie la cola y saque los datos por el puerto UART0

*/
	// Controlo que halla espacio en la cola.
	if ( uxQueueSpacesAvailable( xUart1CharsForTxQueue ) < 2 )
		taskYIELD();

	if( xQueueSend( xUart1CharsForTxQueue, &cOutChar, xBlockTime ) != pdPASS ) {
		return pdFAIL;
	}

	vUart1InterruptOn();
	return pdPASS;
}
/*------------------------------------------------------------------------------------*/
ISR( USART0_RX_vect )
{
/* Handler (ISR) de RX0.
 * Este handler se encarga de la recepcion de datos.
 * Cuando llega algun datos por el puerto serial lo recibe este handler y lo va
 * guardando en la cola de recepcion
*/

s08 cChar;

	/* Get the character and post it on the queue of Rxed characters.
	 * If the post causes a task to wake force a context switch as the woken task
	 * may have a higher priority than the task we have interrupted.
    */
	cChar = inb(UDR0);
	fifo_put ( &xUart0RxedCharsBuffer, cChar);

}
/*------------------------------------------------------------------------------------*/
ISR( USART1_RX_vect )
{
/* Handler (ISR) de RX1.
 * Este handler se encarga de la recepcion de datos.
 * Cuando llega algun datos por el puerto serial lo recibe este handler y lo va
 * guardando en la cola de recepcion
*/

s08 cChar;

	/* Get the character and post it on the queue of Rxed characters.
	 * If the post causes a task to wake force a context switch as the woken task
	 * may have a higher priority than the task we have interrupted.
    */
	cChar = inb(UDR1);

	if( xQueueSendFromISR( xUart1RxedCharsQueue, &cChar, pdFALSE ) )
	{
		taskYIELD();
	}
}
/*------------------------------------------------------------------------------------*/
s08 xUartGetChar(u08 nUart, u08 *pcRxedChar,u08 xBlockTime )
{
/* Es la funcion que usamos para leer los caracteres recibidos.
 * Los programas la usan bloqueandose hasta recibir un dato.
*/

	/* Get the next character from the buffer.  Return false if no characters
	are available, or arrive before xBlockTime expires. */
s08 res = FALSE;

	if (nUart == 0) {
		res = xUart0GetChar( pcRxedChar, xBlockTime);
		return (res);
	}

	if (nUart == 1) {
		res = xUart1GetChar( pcRxedChar, xBlockTime);
		return (res);
	}

	return(res);
}
/*------------------------------------------------------------------------------------*/
s08 xUart0GetChar(u08 *pcRxedChar,u08 xBlockTime )
{

s08 res = FALSE;

	if ( fifo_empty ( &xUart0RxedCharsBuffer )) {
		res = FALSE;
	} else {
		//*pcRxedChar = fifo_get(&xUart0RxedCharsBuffer);
		//res = TRUE;
		res = FALSE;
	}

}
/*------------------------------------------------------------------------------------*/
s08 xUart1GetChar(u08 *pcRxedChar,u08 xBlockTime )
{

	if( xQueueReceive( xUart1RxedCharsQueue, pcRxedChar, xBlockTime ) ) {
		return pdTRUE;
	} else	{
		return pdFALSE;
	}
}
/*------------------------------------------------------------------------------------*/
s08 uartQueueIsEmpty(u08 nUart)
{

	if (nUart == 0) {
		return( uart0QueueIsEmpty() );
	}
	if (nUart == 1) {
		return( uart1QueueIsEmpty() );
	}
	return(FALSE);
}
/*------------------------------------------------------------------------------------*/
s08 uart0QueueIsEmpty(void)
{

	if ( uxQueueMessagesWaiting(xUart0CharsForTxQueue) == 0) {
		return (TRUE);
	}
	return(FALSE);
}
/*------------------------------------------------------------------------------------*/
s08 uart1QueueIsEmpty(void)
{
	if ( uxQueueMessagesWaiting(xUart1CharsForTxQueue) == 0) {
		return (TRUE);
	}
	return(FALSE);
}
/*------------------------------------------------------------------------------------*/
void xUartQueueFlush(u08 nUart)
{

	/* Vacio la cola de recepcion */
	if (nUart == 0) {
		xUart0QueueFlush();
		return;
	}
	if (nUart == 1) {
		xUart1QueueFlush();
		return;
	}
}
/*------------------------------------------------------------------------------------*/
void xUart0QueueFlush(void)
{
	fifo_flush ( &xUart0RxedCharsBuffer, TRUE);
	//while (xUart0GetChar( &cChar, ( TickType_t ) 10 ) );
	return;
}
/*------------------------------------------------------------------------------------*/
void xUart1QueueFlush(void)
{
	xQueueReset(xUart1RxedCharsQueue);
	//while (xUart1GetChar( &cChar, ( TickType_t ) 10 ) );
	return;
}
/*------------------------------------------------------------------------------------*/

void DEBUG_getParams( u16 *head, u16 *tail, u16 *count)
{
	*head = xUart0RxedCharsBuffer.head;
	*tail = xUart0RxedCharsBuffer.tail;
	*count = xUart0RxedCharsBuffer.count;

}

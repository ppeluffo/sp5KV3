/*
 * sp5KFRTOS_i2c.c
 *
 *  Created on: 28/10/2013
 *      Author: root
 *
 *  Funciones de base de atencion al bus I2E modificadas para usarse con FRTOS.
 *  1- Las esperas las hago con taskYIELD() o xTaskDelay()
 *  2- El acceso lo hago por medio de un semaforo.
 *
 */

#include "i2c_sp5KFRTOS.h"

// Funciones privadas
static void pvi2C_setBitRate(int bitrateKHz);
static inline s08 pvi2c_sendStart(void);
static inline void pvi2c_sendStop(void);
static inline s08 pvi2c_writeByte(u08 data);
static inline s08 pvi2c_readByte(u08 ackFlag);
static inline void pvi2c_disable(void);
//static inline u08 i2c_status(void);
//------------------------------------------------------------------------------------
void i2c_init(void)
{
	// set pull-up resistors on I2C bus pins
	//sbi(PORTC, 0);  // i2c SCL
	//sbi(PORTC, 1);  // i2c SDA
	PORTC |= 1 << SCL;
	PORTC |= 1 << SDA;
	// set i2c bit rate to 100KHz
	pvi2C_setBitRate(100);

	// Creo el semaforo
	semph_I2C = xSemaphoreCreateMutex();

}
//------------------------------------------------------------------------------------
static void pvi2C_setBitRate(int bitrateKHz)
{
int bitrate_div;

	// SCL freq = F_CPU/(16+2*TWBR*4^TWPS)

	// set TWPS to zero
	//cbi(TWSR, TWPS0);
	//cbi(TWSR, TWPS1);
	((TWSR) &= ~(1 << (TWPS0)));
	((TWSR) &= ~(1 << (TWPS1)));
	// SCL freq = F_CPU/(16+2*TWBR))

	// calculate bitrate division
	bitrate_div = ((F_CPU/1000l)/bitrateKHz);
	if(bitrate_div >= 16) {
		bitrate_div = (bitrate_div-16)/2;
	}

	TWBR = bitrate_div;
}
//------------------------------------------------------------------------------------
static inline s08 pvi2c_sendStart(void)
{
u08 timeout;

	// Genera la condicion de START en el bus I2C.
	// TWCR.TWEN = 1 : Habilita la interface TWI
	// TWCR.TWSTA = 1: Genera un START
	// TWCR.TWINT = 1: Borra la flag e inicia la operacion del TWI
	//
	// Cuando el bus termino el START, prende la flag TWCR.TWINT y el codigo
	// de status debe ser 0x08 o 0x10.
	//
	// Controlo por timeout la salida para no quedar bloqueado.
	//
	TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);

	// Espero que la interface I2C termine su operacion hasta 10 ticks.
	timeout = 10;
	while ( (timeout-- > 0) && !(TWCR & (1<<TWINT) ) ) {
		//_delay_ms(1);
		taskYIELD();
	}

	if (timeout == 0) {
		return (FALSE);
	} else {
		// retieve current i2c status from i2c TWSR
		return( TWSR & 0xF8 );
	}

}
//------------------------------------------------------------------------------------
static inline s08 pvi2c_writeByte(u08 data)
{
u08 timeout;

	// Envia el data por el bus I2C.
	TWDR = data;
	// Habilita la trasmision y el envio de un ACK.
	TWCR = (1 << TWINT) | (1 << TWEN);

	// Espero que la interface I2C termine su operacion.
	timeout = 10;
	while ( (timeout-- > 0) && !(TWCR & (1<<TWINT) ) ) {
		//_delay_ms(1);
		taskYIELD();
	}

	if (timeout == 0) {
		return (FALSE);
	} else {
		// retieve current i2c status from i2c TWSR
		return( TWSR & 0xF8 );
	}

}
//------------------------------------------------------------------------------------
static inline s08 pvi2c_readByte(u08 ackFlag)
{
u08 timeout;

	// begin receive over i2c
	if( ackFlag )
	{
		// ackFlag = TRUE: ACK the recevied data
		TWCR = (1 << TWEA) | (1 << TWINT) | (1 << TWEN);
	}
	else
	{
		// ackFlag = FALSE: NACK the recevied data
		TWCR = (1 << TWINT) | (1 << TWEN);
	}

	// Espero que la interface I2C termine su operacion.
	timeout = 10;
	while ( (timeout-- > 0) && !(TWCR & (1<<TWINT) ) ) {
		//_delay_ms(1);
		taskYIELD();
	}

	if (timeout == 0) {
		return (FALSE);
	} else {
		// retieve current i2c status from i2c TWSR
		return( TWSR & 0xF8 );
	}

}
//------------------------------------------------------------------------------------
static inline void pvi2c_sendStop(void)
{
	// Genera la condicion STOP en el bus I2C.
	// !!! El TWINT NO ES SETEADO LUEGO DE UN STOP
	// por lo tanto no debo esperar que termine.

	TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);

}
//-----------------------------------------------------------------------------------
//static inline u08 i2c_status(void)
//{
	// Retorna el status del modulo I2C por medio del registro TWSR
//	return( TWSR & 0xF8 );
//}
//------------------------------------------------------------------------------------
static inline void pvi2c_disable(void)
{
	// Deshabilito la interfase TWI
	TWCR &= ~(1 << TWEN);

}
//------------------------------------------------------------------------------------
s08 i2c_masterWrite (u08 dev_type, u16 i2c_address, u08 dev_id, u08 dev_addr,u08 length, char *data)
{
/* Escribe en el dispositivo I2C 'length' byes, apuntados por *data
 * Todos los dispositivos I2C permite hacer esta escritura de bloque
 * que es mas rapida que ir escribiendo de a byte.
 * i2c_address: direccion del primer byte a escribir
 * dev_id:      identificador del dispositivo I2C ( eeprom: 0xA0, ds1340: 0xD0, buscontroller: 0x40, etc)
 * dev_addr:    direccion individual del dispositivo dentro del bus ( 0,1,2,...)
 * length:      cantidad de bytes a escribir
 * * data:      puntero a la posicion del primer byte a escribir.
 * Retorna -1 en caso de error o 1 en caso OK ( TRUE, FALSE).
 * En caso de error, imprime el mismo con su codigo en la salida de DEBUG.
 *
 */

u08 i2c_status;
char txbyte;
u08 n = 0;
s08 r_val = FALSE;
u08 i;

	// Control de tamanio de buffer
	if (length > EERDBUFLENGHT)
		length = EERDBUFLENGHT;

	i2c_retry:

		if (n++ >= I2C_MAXTRIES) goto i2c_quit;

		// Pass1) START: Debe retornar 0x08 (I2C_START) o 0x10 (I2C_REP_START) en condiciones normales
		i2c_status = pvi2c_sendStart();
		if ( i2c_status == TW_MT_ARB_LOST) goto i2c_retry;
		if ( (i2c_status != TW_START) && (i2c_status != TW_REP_START) ) goto i2c_quit;

		// Pass2) (SLA_W) Send slave address. Debo recibir 0x18 ( SLA_ACK )
		txbyte = (dev_id & 0xF0) | ((dev_addr & 0x07) << 1) | TW_WRITE;
		i2c_status = pvi2c_writeByte(txbyte);
		// Check the TWSR status
		if ((i2c_status == TW_MT_SLA_NACK) || (i2c_status == TW_MT_ARB_LOST)) goto i2c_retry;
		if (i2c_status != TW_MT_SLA_ACK) goto i2c_quit;

		// Pass3) Envio la direccion fisica donde comenzar a escribir.
		// En las memorias es una direccion de 16 bytes.
		// En el DS1344 o el BusController es de 1 byte
		switch (dev_type) {
		case EE24X:
			// La direccion fisica son 2 byte.
			// Envio primero el High 8 bit i2c address. Debo recibir 0x28 ( DATA_ACK)
			txbyte = i2c_address >> 8;
			i2c_status = pvi2c_writeByte(txbyte);
			if (i2c_status != TW_MT_DATA_ACK) goto i2c_quit;

			// Envio el Low byte.
			txbyte = i2c_address;
			i2c_status = pvi2c_writeByte(txbyte);
			if (i2c_status != TW_MT_DATA_ACK) goto i2c_quit;
			break;
		case DS1340:
			// Envio una direccion de 1 byte.
			// Send the High 8-bit of I2C Address. Debo recibir 0x28 (DATA_ACK)
			txbyte = i2c_address;
			i2c_status = pvi2c_writeByte(txbyte);
			if (i2c_status != TW_MT_DATA_ACK) goto i2c_quit;
			break;
		case MPC23008:
			// Envio una direccion de 1 byte.
			// Send the High 8-bit of I2C Address. Debo recibir 0x28 (DATA_ACK)
			txbyte = i2c_address;
			i2c_status = pvi2c_writeByte(txbyte);
			if (i2c_status != TW_MT_DATA_ACK) goto i2c_quit;
			break;
		case ADS7828:
			// No hay address.
			break;
		default:
			break;
		}

		// Pass4) Trasmito todos los bytes del buffer. Debo recibir 0x28 (DATA_ACK)
		i = 0;
		while(length)
		{
			txbyte = *data++;
			i2c_status = pvi2c_writeByte( txbyte );
			if (i2c_status != TW_MT_DATA_ACK) goto i2c_quit;
			length--;
			i++;
		}

		// I2C transmit OK.
		r_val = TRUE;

	i2c_quit:

		// Pass5) STOP
		pvi2c_sendStop();
		vTaskDelay( ( TickType_t)( 20 / portTICK_RATE_MS ) );

		// En caso de error libero la interface.
		if (r_val == FALSE) pvi2c_disable();

	return(r_val);

}
//------------------------------------------------------------------------------------
s08 i2c_masterRead (u08 dev_type, u16 i2c_address, u08 dev_id, u08 dev_addr,u08 length, char *data)
{
/* Lee del dispositivo I2C 'length' byes y los retorna en *data
 * Todos los dispositivos I2C permite hacer esta lectura de bloque
 * que es mas rapida que ir escribiendo de a byte.
 * i2c_address: direccion del primer byte a escribir
 * dev_id:      identificador del dispositivo I2C ( eeprom: 0xA0, ds1340: 0xD0, buscontroller: 0x40, etc)
 * dev_addr:    direccion individual del dispositivo dentro del bus ( 0,1,2,...)
 * length:      cantidad de bytes a escribir
 * * data:      puntero a la posicion del primer byte a escribir.
 * Retorna -1 en caso de error o 1 en caso OK ( TRUE, FALSE).
 * En caso de error, imprime el mismo con su codigo en la salida de DEBUG.*
*/

u08 i2c_status;
char txbyte;
u08 n = 0;
s08 r_val = FALSE;
u08 i;

	// Control de tamanio de buffer
	if (length > EERDBUFLENGHT)
		length = EERDBUFLENGHT;

	i2c_retry:

		if (n++ >= I2C_MAXTRIES) goto i2c_quit;

		// Pass1) START: Debe retornar 0x08 (I2C_START) o 0x10 (I2C_REP_START) en condiciones normales
		i2c_status = pvi2c_sendStart();
		if (i2c_status == TW_MT_ARB_LOST) goto i2c_retry;
		if ( (i2c_status != TW_START) && (i2c_status != TW_REP_START) ) goto i2c_quit;

		// Pass2) (SLA_W) Send slave address + WRITE
		txbyte = (dev_id & 0xF0) | ((dev_addr & 0x07) << 1) | TW_WRITE;
		i2c_status = pvi2c_writeByte(txbyte);
		// Check the TWSR status
		if ((i2c_status == TW_MT_SLA_NACK) || (i2c_status == TW_MT_ARB_LOST)) goto i2c_retry;
		if (i2c_status != TW_MT_SLA_ACK) goto i2c_quit;

		// Pass3) Envio la direccion fisica donde comenzar a leer.
		// En las memorias es una direccion de 16 bytes.
		// En el DS1344 o el BusController es de 1 byte
		switch (dev_type) {
		case EE24X:
			// La direccion fisica son 2 byte.
			// Envio primero el High 8 bit i2c address
			i2c_status = pvi2c_writeByte(i2c_address >> 8);
			if (i2c_status != TW_MT_DATA_ACK) goto i2c_quit;
			break;
		case DS1340:
			break;
		case MPC23008:
			break;
		default:
			break;
		}
		// En todos los otros casos envio una direccion de 1 byte.
		// Send the Low 8-bit of I2C Address
		i2c_status = pvi2c_writeByte(i2c_address);
		if (i2c_status != TW_MT_DATA_ACK) goto i2c_quit;

		// Pass4) REPEATED START
		i2c_status = pvi2c_sendStart();
		if (i2c_status == TW_MT_ARB_LOST) goto i2c_retry;
		if ( (i2c_status != TW_START) && (i2c_status != TW_REP_START) ) goto i2c_quit;

		// Pass5) (SLA_R) Send slave address + READ. Debo recibir un 0x40 ( SLA_R ACK)
		txbyte = (dev_id & 0xF0) | ((dev_addr << 1) & 0x0E) | TW_READ;
		i2c_status = pvi2c_writeByte(txbyte);
		// Check the TWSR status
		if ((i2c_status == TW_MR_SLA_NACK) || (i2c_status == TW_MR_ARB_LOST)) goto i2c_retry;
		if (i2c_status != TW_MR_SLA_ACK) goto i2c_quit;

		// Pass6.1) Leo todos los bytes requeridos y respondo con ACK.
		i = 0;
		while(length > 1)
		{
			// accept receive data and ack it
			// begin receive over i2c
			i2c_status = pvi2c_readByte(ACK);
			if (i2c_status != TW_MR_DATA_ACK) goto i2c_quit;
			*data++ = TWDR;
			// decrement length
			length--;
			i++;
		}

		// Pass6.2) Acepto el ultimo byte y respondo con NACK
		pvi2c_readByte(NACK);
		*data++ = TWDR;

		// I2C read OK.
		r_val = TRUE;

	i2c_quit:

	// Pass5) STOP
	pvi2c_sendStop();

	// En caso de error libero la interface.
	if (r_val == FALSE) pvi2c_disable();

	return(r_val);
}
//------------------------------------------------------------------------------------
s08 adc_masterRead (u08 dev_type, u16 i2c_address, u08 dev_id, u08 dev_addr,u08 length, u08 *data)
{
/* Tomado de i2c_masterRead pero modificado para el ADC7828 en el cual el read no manda la SLA+W.
 *
 */

u08 i2c_status;
u08 txbyte;
u08 n = 0;
s08 r_val = FALSE;

	// Control de tamanio de buffer
	if (length > EERDBUFLENGHT)
		length = EERDBUFLENGHT;

	i2c_retry:

		if (n++ >= I2C_MAXTRIES) goto i2c_quit;

		// Pass1) START: Debe retornar 0x08 (I2C_START) o 0x10 (I2C_REP_START) en condiciones normales
		i2c_status = pvi2c_sendStart();
		if (i2c_status == TW_MT_ARB_LOST) goto i2c_retry;
		if ( (i2c_status != TW_START) && (i2c_status != TW_REP_START) ) goto i2c_quit;

		// Pass5) (SLA_R) Send slave address + READ. Debo recibir un 0x40 ( SLA_R ACK)
		txbyte = (dev_id & 0xF0) | ((dev_addr << 1) & 0x0E) | TW_READ;
		i2c_status = pvi2c_writeByte(txbyte);
		// Check the TWSR status
		if ((i2c_status == TW_MR_SLA_NACK) || (i2c_status == TW_MR_ARB_LOST)) goto i2c_retry;

		if (i2c_status != TW_MR_SLA_ACK) goto i2c_quit;

		// Pass6.1) Leo todos los bytes requeridos y respondo con ACK.
		while(length > 1)
		{
			// accept receive data and ack it
			// begin receive over i2c
			i2c_status = pvi2c_readByte(ACK);
			if (i2c_status != TW_MR_DATA_ACK) goto i2c_quit;
			*data++ = TWDR;
			// decrement length
			length--;
		}

		// Pass6.2) Acepto el ultimo byte y respondo con NACK
		pvi2c_readByte(NACK);
		*data++ = TWDR;

		// I2C read OK.
		r_val = TRUE;

	i2c_quit:

		// Pass5) STOP
	pvi2c_sendStop();

	// En caso de error libero la interface.
	if (r_val == FALSE) pvi2c_disable();

	return(r_val);
}
//------------------------------------------------------------------------------------


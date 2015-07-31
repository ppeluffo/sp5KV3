/*
 * sp5KFRTOS_rtc.c
 *
 *  Created on: 01/11/2013
 *      Author: root
 *
 * Funciones del RTC DS1340-33 modificadas para usarse con FRTOS.
 *
 *
 */
//------------------------------------------------------------------------------------

#include "rtc_sp5KFRTOS.h"

static char pv_bcd2dec(char num);
static char pv_dec2bcd(char num);
static s08 pvRTC_read( char *data);
static s08 pvRTC_write( char *data);

//------------------------------------------------------------------------------------
s08 rtcGetTime(RtcTimeType *rtc)
{
	// Retorna la hora formateada en la estructura RtcTimeType
	// No retornamos el valor de EOSC ni los bytes adicionales.

u08 oscStatus;
s08 retStatus;
u08 data[8];

	retStatus = pvRTC_read( &data );
	if (!retStatus)
		return(-1);

	// Decodifico los resultados.
	oscStatus = 0;
	if ( (data[0] & 0x80) == 0x80 ) {		// EOSC
		oscStatus = 1;
	}
	rtc->sec = pv_bcd2dec(data[0] & 0x7F);
	rtc->min = pv_bcd2dec(data[1]);
	rtc->hour = pv_bcd2dec(data[2] & 0x3F);
	rtc->day = pv_bcd2dec(data[4] & 0x3F);
	rtc->month = pv_bcd2dec(data[5] & 0x1F);
	rtc->year = pv_bcd2dec(data[6]) + 2000;

	return(1);
}
//------------------------------------------------------------------------------------
s08 rtcSetTime(RtcTimeType *rtc)
{
	// Setea el RTC con la hora pasada en la estructura RtcTimeType

s08 retStatus;
u08 data[8];

	data[0] = 0;	// EOSC = 0 ( rtc running)
	data[1] = pv_dec2bcd(rtc->min);
	data[2] = pv_dec2bcd(rtc->hour);
	data[3] = 0;
	data[4] = pv_dec2bcd(rtc->day);
	data[5] = pv_dec2bcd(rtc->month);
	data[6] = pv_dec2bcd(rtc->year);

//	for ( i=0; i<7; i++) {
//		rprintf(CMD_UART, "I=%d, v=%x\r\n\0", i, data[i]);
//	}
	retStatus = pvRTC_write( &data );
	return(retStatus);
}
//------------------------------------------------------------------------------------
static s08 pvRTC_read( char *data)
{
	// Funcion generica que lee los 8 primeros bytes de datos del RTC
	// Hay que atender que *data tenga al menos los 8 bytes.

s08 ret = FALSE;

	while ( xSemaphoreTake( semph_I2C, ( TickType_t ) 10 ) != pdTRUE )
		taskYIELD();

	ret = i2c_masterRead (DS1340, 0, DS1340_ID, DS1340_ADDR, 8, data);

	xSemaphoreGive( semph_I2C );
	return(ret);
}
//------------------------------------------------------------------------------------
static s08 pvRTC_write( char *data)
{
s08 ret = FALSE;

	while ( xSemaphoreTake( semph_I2C, ( TickType_t ) 10 ) != pdTRUE )
		taskYIELD();

	// Funcion generica que escribe los 7 primeros bytes del RTC
	ret = i2c_masterWrite (DS1340, 0, DS1340_ID, DS1340_ADDR, 7, data);

	xSemaphoreGive( semph_I2C );
	return(ret);
}
//------------------------------------------------------------------------------------
static char pv_dec2bcd(char num)
{
	// Convert Decimal to Binary Coded Decimal (BCD)
	return ((num/10 * 16) + (num % 10));
}
//------------------------------------------------------------------------------------
static char pv_bcd2dec(char num)
{
	// Convert Binary Coded Decimal (BCD) to Decimal
	return ((num/16 * 10) + (num % 16));
}
//------------------------------------------------------------------------------------

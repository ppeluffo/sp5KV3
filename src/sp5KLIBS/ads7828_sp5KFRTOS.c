/*
 * ads7828_sp5KFRTOS.c
 *
 *  Created on: 15/4/2015
 *      Author: pablo
 */


#include "ads7828_sp5KFRTOS.h"

s08 pvADS7828_write( u08 *data);
s08 pvADS7828_read( u08 *data);

//------------------------------------------------------------------------------------
s08 pvADS7828_write( u08 *data)
{
s08 ret = FALSE;

	while ( xSemaphoreTake( semph_I2C, ( TickType_t ) 10 ) != pdTRUE )
		taskYIELD();

	ret = i2c_masterWrite (ADS7828, 0 , ADS7828_ID, ADS7828_ADDR, 1, data);

	xSemaphoreGive( semph_I2C );
	return(ret);

}
//------------------------------------------------------------------------------------
s08 pvADS7828_read( u08 *data)
{
s08 ret = FALSE;

	while ( xSemaphoreTake( semph_I2C, ( TickType_t ) 10 ) != pdTRUE )
		taskYIELD();

	ret = adc_masterRead (ADS7828, 0 , ADS7828_ID, ADS7828_ADDR, 2, data);

	xSemaphoreGive( semph_I2C );
	return(ret);

}
//------------------------------------------------------------------------------------
void ADS7828_init(void)
{
	return;
}
//------------------------------------------------------------------------------------
s08 ADS7828_convert(u08 channel, u16 *value)
{

u08 ads7828Channel;
u08 ads7828CmdByte;
u08 buffer[2];
u08 status;
u16 retValue;

	if ( channel > 7) {
		return(FALSE);
	}
	// Convierto el canal 0-7 al C2/C1/C0 requerido por el conversor.
	ads7828Channel = (((channel>>1) | (channel&0x01)<<2)<<4) | ADS7828_CMD_SD;
	// do conversion
	// Armo el COMMAND BYTE
	ads7828CmdByte = ads7828Channel & 0xF0;	// SD=1 ( single end inputs )
	ads7828CmdByte |= ADS7828_CMD_PDMODE2;	// Internal reference ON, A/D converter ON
	// start conversion on requested channel

	status = pvADS7828_write( &ads7828CmdByte);
	if (!status) {
		return(status);
	}

	// Espero el settle time
	//vTaskDelay(1);

	// retrieve conversion result
	status = pvADS7828_read( &buffer);
	retValue = (buffer[0]<<8) | buffer[1];
//	rprintf(CMD_UART, "ADC chD[%d][%d][%x][%d][%x]\r\n",retValue, buffer[0],buffer[0], buffer[1],buffer[1]);
	// pack bytes and return result
	*value = retValue;

	// Apago el conversor
//	ads7828CmdByte = ads7828Channel & 0xF0;
//	ads7828CmdByte |= ADS7828_CMD_PDMODE0;	// Internal reference OFF, A/D converter OFF
//	status = pvADS7828_write( &ads7828CmdByte);

	return (status);

}
//------------------------------------------------------------------------------------



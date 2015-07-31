/*
 * ee_sp5KFRTOS.c
 *
 *
 *  Created on: 01/11/2013
 *      Author: root
 *
 *  Funciones de EE modificadas para usarse con FRTOS.
 */
//------------------------------------------------------------------------------------

#include "ee_sp5KFRTOS.h"

//------------------------------------------------------------------------------------

s08 EE_read( u16 i2c_address,u08 length, char *data)
{
s08 ret = FALSE;

	while ( xSemaphoreTake( semph_I2C, ( TickType_t ) 10 ) != pdTRUE )
		taskYIELD();

	ret = i2c_masterRead (EE24X, i2c_address, EE_ID, EE_ADDR, length, data);

	xSemaphoreGive( semph_I2C );
	return(ret);
}
//------------------------------------------------------------------------------------

s08 EE_write( u16 i2c_address,u08 length, char *data)
{
	// Esta funcion escribe a partir de una posicion dada de la eeprom que no tiene
	// porque coincidir con una pagina.
	// Esto puede traer problemas ya que muchas veces la eeprom se graba por paginas

s08 ret = FALSE;

	while ( xSemaphoreTake( semph_I2C, ( TickType_t ) 10 ) != pdTRUE )
		taskYIELD();

	ret = i2c_masterWrite (EE24X, i2c_address, EE_ID, EE_ADDR, length, data);

	xSemaphoreGive( semph_I2C );
	return(ret);

}
//------------------------------------------------------------------------------------

s08 EE_pageWrite( u16 i2c_address,u08 length, char *data)
{
	// Esta funcion controla que se escriba una pagina, controlando que la direccion
	// de escritura sea una frontera de pagina.

s08 ret = FALSE;

	// Controlo que el tamano no exceda al de una pagina
	if (length > EERDBUFLENGHT) {
		length = EERDBUFLENGHT;
	}

	// Ajusto la direccion de escritura a la frontera de pagina mas proxima
	if (  (i2c_address % EEPAGESIZE) != 0 ) {
		i2c_address = (i2c_address / EEPAGESIZE ) * EEPAGESIZE;
	}

	while ( xSemaphoreTake( semph_I2C, ( TickType_t ) 10 ) != pdTRUE )
		taskYIELD();

	ret = i2c_masterWrite (EE24X, i2c_address, EE_ID, EE_ADDR, length, data);

	xSemaphoreGive( semph_I2C );
	return(ret);

}
//------------------------------------------------------------------------------------




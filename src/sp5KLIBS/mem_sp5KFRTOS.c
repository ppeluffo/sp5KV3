/*
 * sp5KV3_mem.c
 *
 *  Created on: 26/5/2015
 *      Author: pablo
 *
 *  MEM_write()
 *  MEM_read()
 *  MEM_delete()
 *  MEM_deleteBlock()
 *  MEM_init()
 *  MEM_drop()
 *  MEM_status()
 *

 *
 *  Todas las funciones retorna TRUE / FALSE.
 *  Se les pasa un puntero *result donde indican el codigo de salida.
 *  ( MEM_FULL, MEM_EMPTY, RD_ERROR, WR_ERROR, etc).
 *  Estas funciones intercambian con el resto del programa que las invoca datos del tipo frameDataType.
 *  Estos datos los conviertes hacia/desde un buffer lineal al que le agregan datos ( id, checksum, etc)
 *  para poder leer/grabar/borrar en la memoria fisica.
 *
 *  Luego utilizamos una serie de funciones privadas que si deben tener en cuenta los detalles de
 *  implementacion de la memoria.
 *  pv_MEMpush(): 		graba en la primer posicion disponible de la memoria
 *  pv_MEMshift4Rd(): 	lee desde el principio de la memoria.
 *  pv_MEMshift4Del(): 	borra desde el ppio.de la memoria.
 *  pv_MEMisFull()
 *  pv_MEMisEmpty()
 *
 *  Al incializar la memoria, WRptr = DELptr = 0.
 *  WR: Escribo y avanzo el puntero.
 *  DEL: Borro y avanzo
 *  Memoria vacia: DELptr = WRptr
 *  Memoria llena:
 */
//------------------------------------------------------------------------------------

#include "mem_sp5KFRTOS.h"


struct {			// Memory Control Block
	u16 WRptr;		// Estructura de control de archivo
	u16 RDptr;
	u16 DELptr;
	u16 rcds4rd;
	u16 rcds4del;
	char buffer[MEMRECDSIZE];
} MCB;

static u08 pv_MEMchecksum( char *buff, u08 limit );
//static char mem_printfBuff[64];

//------------------------------------------------------------------------------------
s08 MEM_init( void )
{
	/* Inicializa el sistema de la memoria ( punteros )
	 * Recorro la memoria buscando transiciones VACIO->DATO y DATO->VACIO.
	 * La transicion VACIO->DATO determina el puntero DELptr
	 * La transicion DATO->VACIO determina el puntero WRptr.
	 * Si no hay transiciones y todos son datos, la memoria esta llena.
	 * Si no hay transicions y todos son blancos, la memoria esta vacia.
	 */

char mark_Z, mark;
u16 i2c_address, i;
s08 transicion = FALSE;

	// Creo el semaforo
	//semph_MEM = xSemaphoreCreateMutex();
	// Lo debo crear en MAIN antes de arrancar el RTOS

	while ( xSemaphoreTake( semph_MEM, ( TickType_t ) 10 ) != pdTRUE )
		taskYIELD();

	i2c_address = EEADDR_START + ( MAXMEMRCS - 1) * MEMRECDSIZE;
	EE_read( i2c_address,sizeof(MCB.buffer), MCB.buffer);
	mark_Z = MCB.buffer[0];

	for ( i=0; i < MAXMEMRCS; i++) {
		// Calculo la direccion del registro a borrar
		i2c_address = EEADDR_START + i * MEMRECDSIZE;
		EE_read( i2c_address,sizeof(MCB.buffer), MCB.buffer);
		mark = MCB.buffer[0];

		if ( ( mark_Z == '\0') && ( mark == 0xC5) ) {
			// Tengo una transicion VACIO->DATO.
			MCB.DELptr = i;
			MCB.RDptr = i;
			transicion = TRUE;
		}

		if ( ( mark_Z == 0xC5) && ( mark == '\0') ) {
			// Tengo una transicion DATO->VACIO.
			MCB.WRptr = i;
			transicion = TRUE;
		}

		mark_Z = mark;
	}

	if ( ! transicion ) {
		// Si no hubieron transiciones es que la memoria esta llena o vacia.
		if ( mark == '\0') {
			// Memoria vacia.
			MCB.RDptr = 0;
			MCB.WRptr = 0;
			MCB.DELptr = 0;
			MCB.rcds4rd = 0;
			MCB.rcds4del = 0;
		} else {
			// Memoria llena
			MCB.DELptr = 0;
			MCB.RDptr = 0;
			MCB.WRptr = 0;
			MCB.rcds4rd = MAXMEMRCS;
			MCB.rcds4del = MAXMEMRCS;
		}
	} else {
		// Memoria con datos. Calculo los registro ocupados.
		if ( MCB.WRptr > MCB.DELptr) {
			MCB.rcds4rd = ( MCB.WRptr - MCB.DELptr);
			MCB.rcds4del = MCB.rcds4rd;
		} else {
			MCB.rcds4rd = MAXMEMRCS - MCB.DELptr + MCB.WRptr;
			MCB.rcds4del = MCB.rcds4rd;
		}
	}

	xSemaphoreGive( semph_MEM );

}
//------------------------------------------------------------------------------------
u16 MEM_getRdPtr(void)
{
	return (MCB.RDptr);
}
//------------------------------------------------------------------------------------
u16 MEM_getWrPtr(void)
{
	return (MCB.WRptr);
}
//------------------------------------------------------------------------------------
u16 MEM_getRcdsFree(void)
{
	return (MAXMEMRCS - MCB.rcds4del);
}
//------------------------------------------------------------------------------------
u16 MEM_getRcds4rd(void)
{
	return (MCB.rcds4rd);
}
//------------------------------------------------------------------------------------
u16 MEM_getRcds4del(void)
{
	return (MCB.rcds4del);
}
//------------------------------------------------------------------------------------
u16 MEM_getDELptr(void)
{
	return (MCB.DELptr);
}
//------------------------------------------------------------------------------------
void MEM_drop(void)
{
// Borro toda la memoria grabando todos los registros con una marca.

u16 i2c_address;
u16 i;

	// Puede demorar por lo que debe pararse el watchdog.
	wdt_reset();
	wdt_disable();

	while ( xSemaphoreTake( semph_MEM, ( TickType_t ) 10 ) != pdTRUE )
		taskYIELD();

	// inicializo la estructura lineal base.
	memset( MCB.buffer,'\0', sizeof(MCB.buffer) );
	// Borro.
	for ( i=0; i < MAXMEMRCS; i++) {
		// Calculo la direccion del registro a borrar
		i2c_address = EEADDR_START + i * MEMRECDSIZE;
		EE_pageWrite( i2c_address,sizeof(MCB.buffer), &MCB.buffer);

		// DEBUG
		//EE_read( i2c_address,sizeof(MCB.buffer), MCB.buffer);
		//snprintf_P( mem_printfBuff,64,PSTR("ptr=%d,[0x%x] "),i,MCB.buffer[0] );
		//TERMrprintfStr( mem_printfBuff );
		//if ( (i % 8) == 0 ) {
		//	TERMrprintfChar('\n');
		//	taskYIELD();
		//}

		// Imprimo realimentacion c/32 recs.
		if ( (i % 32) == 0 ) {
			TERMrprintfChar('.');
			taskYIELD();
		}
	}

	MCB.RDptr = 0;
	MCB.WRptr = 0;
	MCB.DELptr = 0;
	MCB.rcds4del = 0;
	MCB.rcds4rd = 0;

	TERMrprintfChar('\n');
	xSemaphoreGive( semph_MEM );

}
//------------------------------------------------------------------------------------
s08 MEM_erase( void )
{
	/* Borra un registro de la memoria.
	 * Para esto lo rellena con blancos.
	 * El registro que borra es el que esta apuntando el DELptr.
	 */

s08 retS = MEM_OK;
s08 retEE;
u16 i2c_address;

	while ( xSemaphoreTake( semph_MEM, ( TickType_t ) 10 ) != pdTRUE )
		taskYIELD();

	// Si la memoria esta vacia no tengo nada que borrar: salgo
	if ( MEMisEmpty4Del() ) {
		retS = MEM_EMPTY;
		goto quit;
	}

	// inicializo la estructura lineal base.
	memset( MCB.buffer,'\0', sizeof(MCB.buffer) );
	// Calculo la direccion del registro a borrar
	i2c_address = EEADDR_START + MCB.DELptr * MEMRECDSIZE;
	retEE = EE_pageWrite( i2c_address,sizeof(MCB.buffer), &MCB.buffer);
	if ( !retEE ) {
		retS = MEM_WREEFAIL;
		goto quit;
	}

	MCB.rcds4del--;
	// Avanzo el puntero de DEL en modo circular
	MCB.DELptr = (++MCB.DELptr == MAXMEMRCS) ?  0 : MCB.DELptr;

	retS = MEM_OK;

quit:
	xSemaphoreGive( semph_MEM );
	return(retS);
	//

}
//------------------------------------------------------------------------------------
s08 MEM_write( void *dataFrame, const u08 dataFrameSize )
{
	/* Graba en la memoria un frame apuntado por *dataFrame. Para que la funcion
	 * sea generica, el puntero es void y entonces debemos pasar el tamaño de la estructura
	 * a la que apunta.
	 * El resultado lo devolvemos en retStatus.
	 * Solo escribe si la memoria no esta llena. En este caso el dato se pierde.
	 *
	 */

s08 retS = MEM_OK;
s08 retEE;
u16 i2c_address;

	while ( xSemaphoreTake( semph_MEM, ( TickType_t ) 10 ) != pdTRUE )
		taskYIELD();

	// Si la memoria esta llena no puedo escribir: salgo
	if ( MEMisFull() ) {
		retS = MEM_FULL;
		goto quit;
	}

	// inicializo la estructura lineal base.
	memset( MCB.buffer,'\0', sizeof(MCB.buffer) );
	// copio los datos recibidos del frame al buffer a partir de la direccion 1
	memcpy ( MCB.buffer, dataFrame, dataFrameSize );
	// Calculo y grabo el checksum a continuacion del frame.
	// El checksum es solo del dataFrame por eso paso dicho size.
	MCB.buffer[dataFrameSize] = pv_MEMchecksum( MCB.buffer, dataFrameSize );
	// Write en EE
	// Calculo la direccion de escritura
	i2c_address = EEADDR_START + MCB.WRptr * MEMRECDSIZE;
	retEE = EE_pageWrite( i2c_address,sizeof(MCB.buffer), &MCB.buffer);
	if ( !retEE ) {
		retS = MEM_WREEFAIL;
		goto quit;
	}

	MCB.rcds4rd++;
	MCB.rcds4del++;
	// Avanzo el puntero de WR en modo circular
	MCB.WRptr = (++MCB.WRptr == MAXMEMRCS) ?  0 : MCB.WRptr;

	retS = MEM_OK;

quit:
	xSemaphoreGive( semph_MEM );
	return(retS);
	//

}
//------------------------------------------------------------------------------------
s08 MEM_read( void *dataFrame, const u08 dataFrameSize, const s08 modo )
{
	/* Lee un registro ( como string ) y lo copia al espacio de memoria apuntado
	 * *dataFrame. El puntero es void y entonces debemos pasar el tamaño de la estructura
	 * a la que apunta.
	 * En modo indico con 0 si quiero que inicialize rdPtr a delPtr o sino usar
	 * rdPtr.
	 * En caso que tenga problemas para leer o de checksum, debo continuar e indicar
	 * el error.
	 */

s08 retS = MEM_OK;
s08 retEE;
u16 i2c_address;
u08 rdCheckSum;

	while ( xSemaphoreTake( semph_MEM, ( TickType_t ) 10 ) != pdTRUE )
		taskYIELD();

	// Si la memoria esta vacia salgo
	// Como el read no decrementa rcds, no puedo usarlo para determinar que este vacia.
	if ( MEMisEmpty4Read() ) {
		retS = MEM_EMPTY;
		goto quit;
	}

	retS = MEM_OK;

	// inicializo la estructura lineal base.
	memset( MCB.buffer,'\0', sizeof(MCB.buffer) );
	// Leo el registro de la EE
	if ( modo == 0 ) {	MCB.RDptr = MCB.DELptr; }
	// Calculo la direccion de lectura
	i2c_address = EEADDR_START + MCB.RDptr * MEMRECDSIZE;
	// Avanzo el puntero de RD en modo circular siempre.  !!!
	MCB.RDptr = (++MCB.RDptr == MAXMEMRCS) ?  0 : MCB.RDptr;
	MCB.rcds4rd--;

	retEE = EE_read( i2c_address,sizeof(MCB.buffer), MCB.buffer);
	if ( !retEE ) {
		retS = MEM_RDEEFAIL;
		goto quit;
	}

	// Verifico los datos leidos ( checksum )
	// El checksum es solo del dataFrame por eso paso dicho size.
	rdCheckSum = pv_MEMchecksum( MCB.buffer, dataFrameSize );
	if ( rdCheckSum != MCB.buffer[dataFrameSize] ) {
		retS = MEM_CKSERR;
		goto quit;
	}

	// Copio los datos a la estructura de salida.
	memcpy( dataFrame, &MCB.buffer, dataFrameSize );

quit:
	xSemaphoreGive( semph_MEM );
	return(retS);
	//

}
//------------------------------------------------------------------------------------
static u08 pv_MEMchecksum( char *buff, u08 limit )
{
u08 checksum = 0;
u08 i;

	for( i=0; i<limit; i++)
		checksum += buff[i];

	checksum = ~checksum;
	return (checksum);
}
//----------------------------------------------------------------------------------
s08 MEMisFull(void)
{
	if (MCB.rcds4del == MAXMEMRCS ) {
		return(TRUE);
	} else {
		return(FALSE);
	}
}
//----------------------------------------------------------------------------------
s08 MEMisEmpty4Del(void)
{
	if (MCB.rcds4del == 0 ) {
		return(TRUE);
	} else {
		return(FALSE);
	}
}
//----------------------------------------------------------------------------------
s08 MEMisEmpty4Read(void)
{

	if (MCB.rcds4rd == 0 ) {
		return(TRUE);
	} else {
		return(FALSE);
	}
}
//----------------------------------------------------------------------------------

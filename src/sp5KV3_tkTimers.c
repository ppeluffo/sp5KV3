/*
 * sp5KV3_tkTimers.c
 *
 *  Created on: 30/5/2015
 *      Author: pablo
 */

#include <sp5KV3.h>

#define MAXSYSTEMTIMERS	10

typedef struct {
	s08 autoreload;
	s08 isRunning;
	u32 counter;
	u32 value;
	void (*callBackFunc)(void);
} structTimer;

static s08 timersDefined = -1;
static structTimer systemTimers[MAXSYSTEMTIMERS];

static char timer_printfBuff[CHAR64];

/*------------------------------------------------------------------------------------*/
void tkTimers(void * pvParameters)
{

( void ) pvParameters;
u08 i;

	vTaskDelay( ( TickType_t)( 1000 / portTICK_RATE_MS ) );

	// Espero la notificacion para arrancar
	while ( startToken != STOK_TIMERS ) {
		vTaskDelay( ( TickType_t)( 100 / portTICK_RATE_MS ) );
	}
	TERMrprintfProgStrM("starting tkTimers..\r\n");
	startToken++;

	// Loop
	for( ;; )
	{

		clearWdg(WDG_TIMERS);
		vTaskDelay( ( TickType_t)( 100 / portTICK_RATE_MS ) );

		while ( xSemaphoreTake( sem_TIMERS, ( TickType_t ) 10 ) != pdTRUE )
			taskYIELD();

		for ( i=0; i < MAXSYSTEMTIMERS; i++ ) {

			// Si el timer esta corriendo...
			if ( systemTimers[i].isRunning ) {
				if ( systemTimers[i].counter > 0 ) {
					systemTimers[i].counter-- ;
				}

				// Fin de ciclo ?
				if ( systemTimers[i].counter == 0 ) {
					if ( systemTimers[i].autoreload  ) {
						// Si es autoreload lo recargo.
						systemTimers[i].counter = systemTimers[i].value;
					} else {
						// Sino lo detengo.
						systemTimers[i].isRunning = FALSE;
					}
					// Ejecuto el callback
					systemTimers[i].callBackFunc();
				}
			}
		}

		xSemaphoreGive( sem_TIMERS );
	}
}
/*------------------------------------------------------------------------------------*/
s08 sTimerCreate(s08 autoreload, void (*callBackFunc)(void), u08 *timerHandle)
{
s08 ret = FALSE;

	while ( xSemaphoreTake( sem_TIMERS, ( TickType_t ) 10 ) != pdTRUE )
		taskYIELD();

	if ( ++timersDefined < MAXSYSTEMTIMERS ) {
		systemTimers[timersDefined].autoreload = autoreload;
		systemTimers[timersDefined].callBackFunc = callBackFunc;
		*timerHandle = timersDefined;
		ret = TRUE;
	} else {
		// No hay mas timers
		timersDefined = MAXSYSTEMTIMERS;
		ret = FALSE;
	}

	xSemaphoreGive( sem_TIMERS );
	return(ret);

}
/*------------------------------------------------------------------------------------*/
s08 sTimerStart(u08 timerId)
{
	// Poner a correr un timer es solo prender la flag correspondiente para que
	// la tarea ppal. comienze a incrementarlo.

s08 ret = FALSE;

	while ( xSemaphoreTake( sem_TIMERS, ( TickType_t ) 10 ) != pdTRUE )
		taskYIELD();

	if ( timerId < MAXSYSTEMTIMERS ) {
		systemTimers[timerId].isRunning = TRUE;
		ret = TRUE;
	}

	xSemaphoreGive( sem_TIMERS );
	return(ret);

}
/*------------------------------------------------------------------------------------*/
s08 sTimerRestart(u08 timerId)
{
	// Reinicio el valor de conteo y lo arranco.

s08 ret = FALSE;

	while ( xSemaphoreTake( sem_TIMERS, ( TickType_t ) 10 ) != pdTRUE )
		taskYIELD();

	if ( timerId < MAXSYSTEMTIMERS ) {
		systemTimers[timerId].counter = systemTimers[timerId].value;
		systemTimers[timerId].isRunning = TRUE;
		ret = TRUE;
	}

	xSemaphoreGive( sem_TIMERS );
	return(ret);

}
/*------------------------------------------------------------------------------------*/
s08 sTimerStop(u08 timerId)
{
s08 ret = FALSE;

	while ( xSemaphoreTake( sem_TIMERS, ( TickType_t ) 10 ) != pdTRUE )
		taskYIELD();

	if ( timerId < MAXSYSTEMTIMERS ) {
		systemTimers[timerId].isRunning = FALSE;
		ret = TRUE;
	}

	xSemaphoreGive( sem_TIMERS );
	return(ret);

}
/*------------------------------------------------------------------------------------*/
s08 sTimerChangePeriod(u08 timerId, u32 timeSecs)
{
s08 ret = FALSE;

	while ( xSemaphoreTake( sem_TIMERS, ( TickType_t ) 10 ) != pdTRUE )
		taskYIELD();

	if ( timerId < MAXSYSTEMTIMERS ) {
		systemTimers[timerId].value = ( timeSecs * SEC_2TIMERTICKS );
		systemTimers[timerId].counter = systemTimers[timerId].value;

			//snprintf_P( timer_printfBuff,CHAR64,PSTR("[CP DEBUG] id=%d, ms=%lu, v=%lu, c=%lu\r\n\0"), timerId, timeMs, systemTimers[timerId].value, systemTimers[timerId].counter );
			//TERMrprintfStr( timer_printfBuff );

		ret = TRUE;
	}

	xSemaphoreGive( sem_TIMERS );
	return(ret);

}
/*------------------------------------------------------------------------------------*/
s08 sTimerIsRunning(u08 timerId)
{
	if ( timerId < MAXSYSTEMTIMERS ) {
		return ( systemTimers[timerId].isRunning );
	}
	return(FALSE);

}
/*------------------------------------------------------------------------------------*/
u32 sTimerTimeRemains(u08 timerId)
{

u32 falta = 0;

	while ( xSemaphoreTake( sem_TIMERS, ( TickType_t ) 10 ) != pdTRUE )
		taskYIELD();

	if ( timerId < MAXSYSTEMTIMERS ) {
		if ( systemTimers[timerId].isRunning == FALSE) {
			falta = 0;
		} else {
			falta = ( systemTimers[timerId].counter / SEC_2TIMERTICKS );
		}
	}

	xSemaphoreGive( sem_TIMERS );
	return(falta);

}
/*------------------------------------------------------------------------------------*/
void sTimersShowInfo(void)
{
u08 i;
u08 pos;

	for ( i = 0; i <= timersDefined; i++ ) {
		pos = snprintf_P( timer_printfBuff,CHAR128,PSTR("T_%02d"), i);

		if ( systemTimers[i].autoreload ) {
			pos += snprintf_P( &timer_printfBuff[pos],CHAR128,PSTR(" AR"));
		} else {
			pos += snprintf_P( &timer_printfBuff[pos],CHAR128,PSTR(" OS"));
		}

		if ( systemTimers[i].isRunning ) {
			pos += snprintf_P( &timer_printfBuff[pos],CHAR128,PSTR("  RUN"));
		} else {
			pos += snprintf_P( &timer_printfBuff[pos],CHAR128,PSTR(" STOP"));
		}

		pos += snprintf_P( &timer_printfBuff[pos],CHAR128,PSTR(" VALUE=%lu"), systemTimers[i].value / SEC_2TIMERTICKS);
		pos += snprintf_P( &timer_printfBuff[pos],CHAR128,PSTR(" COUNT=%lu\r\n\0"), systemTimers[i].counter / SEC_2TIMERTICKS);

		TERMrprintfStr( timer_printfBuff );
	}

}
/*------------------------------------------------------------------------------------*/
void sTimersInfo(const u08 param, u08 timerId, u32 *ret)
{
	switch(param) {
	case T_COUNTER:
		*ret = systemTimers[timerId].counter / SEC_2TIMERTICKS;
		break;
	case T_VALUE:
		*ret = systemTimers[timerId].value / SEC_2TIMERTICKS;
		break;
	case T_REMAINS:
		*ret = sTimerTimeRemains(timerId);
		break;
	default:
		ret = NULL;
		break;
	}
}

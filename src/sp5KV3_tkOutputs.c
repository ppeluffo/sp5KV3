/*
 * sp5KV3_tkOutputs.c
 *
 *  Created on: 8/7/2015
 *      Author: pablo
 */


#include <sp5KV3.h>

#define CICLOSEN30S	300

char out_printfBuff[CHAR128];

static void pv_configOutManual(void);
static void pv_configOutInicial(void);

static void pv_setConsignaInicial ( void );
static void pv_checkAndSetConsigna(void);

// CONSIGNA_TIMER
typedef enum { CALLBACK_CONSIGNA, INIT_CONSIGNA, START_CONSIGNA, STOP_CONSIGNA,RESET_CONSIGNA, EXPIRED_CONSIGNA } t_consignaTimer;
u08 xTimer_consigna;
static s08 ac_consignaTimer(t_consignaTimer action, u16 secs, s08 *retStatus);
void consigna_TimerCallBack (void );

RtcTimeType rtcDateTime;
u16 now;

/*------------------------------------------------------------------------------------*/
void tkOutput(void * pvParameters)
{

( void ) pvParameters;
BaseType_t xResult;
uint32_t ulNotifiedValue;

	vTaskDelay( ( TickType_t)( 1000 / portTICK_RATE_MS ) );

	// Espero la notificacion para arrancar
	while ( startToken != STOK_OUT ) {
		vTaskDelay( ( TickType_t)( 100 / portTICK_RATE_MS ) );
	}
	TERMrprintfProgStrM("starting tkOutput..\r\n");
	startToken++;

	// Configuracion INICIAL

	MCP_output0Disable();
	MCP_output1Disable();
	MCP_output2Disable();
	MCP_output3Disable();
	MCP_outputsSleep();
	MCP_outputsNoReset();

	pv_configOutInicial();

	// Arranco el timer de consignas.
	ac_consignaTimer( INIT_CONSIGNA, NULL,NULL );
	ac_consignaTimer( START_CONSIGNA, 30, NULL );

	// Loop
	for( ;; )
	{

		clearWdg(WDG_OUT);

		// Genero una espera de 100ms por algun mensaje para hacer algo.
		xResult = xTaskNotifyWait( 0x00,          	 				/* Don't clear bits on entry. */
		                           ULONG_MAX,        				/* Clear all bits on exit. */
		                           &ulNotifiedValue, 				/* Stores the notified value. */
								   (100 / portTICK_RATE_MS ) );

		if( xResult == pdTRUE ) {
			// Debo chequear y actualizar las salidas
			if ( ( ulNotifiedValue & MSG2OUT_UPDATE ) != 0 ) {
				pv_configOutInicial();
		    }
		 }

		// Cada 30s chequeo las consignas. De este modo la aplico al menos 2 veces
		if ( ac_consignaTimer( EXPIRED_CONSIGNA, NULL, NULL ))  {
			ac_consignaTimer( RESET_CONSIGNA, NULL, NULL );
			pv_checkAndSetConsigna();
		}

	}
}
/*------------------------------------------------------------------------------------*/
static void pv_configOutInicial(void)
{

	if ( systemVars.outputs.wrkMode == OUTPUT_MANUAL ) {
		pv_configOutManual();
	}

	if ( systemVars.outputs.wrkMode == OUTPUT_CONSIGNA ) {
		pv_setConsignaInicial();
	}

}
/*------------------------------------------------------------------------------------*/
static void pv_configOutManual(void)
{
	// Solo si estoy en modo manual porque si estoy en modo consigna y por error
	// mando las salidas, no las toco.
	if ( systemVars.outputs.wrkMode == OUTPUT_MANUAL ) {

		// Seteo las salidas.
		MCP_outputsNoSleep();

		// Habilito las salidas.
		MCP_output0Enable();
		MCP_output1Enable();
		MCP_output2Enable();
		MCP_output3Enable();

		// Configuro sus valores.
		( systemVars.outputs.dout[0] == 0 ) ? MCP_output0Phase_01() : MCP_output0Phase_10();
		( systemVars.outputs.dout[1] == 0 ) ? MCP_output1Phase_01() : MCP_output1Phase_10();
		( systemVars.outputs.dout[2] == 0 ) ? MCP_output2Phase_01() : MCP_output2Phase_10();
		( systemVars.outputs.dout[3] == 0 ) ? MCP_output3Phase_01() : MCP_output3Phase_10();

		snprintf_P( out_printfBuff,CHAR256,PSTR("tkOutput:: Reconfig OUT manual to [%d,%d,%d,%d]\r\n\0"),systemVars.outputs.dout[0],systemVars.outputs.dout[1],systemVars.outputs.dout[2],systemVars.outputs.dout[3] );
	} else {
		snprintf_P( out_printfBuff,CHAR256,PSTR("tkOutput:: Reconfig OUT manual ERROR: modo consigna!!\r\n\0") );
		TERMrprintfStr( out_printfBuff );
	}

	TERMrprintfStr( out_printfBuff );
}
/*------------------------------------------------------------------------------------*/
static void pv_setConsignaInicial ( void )
{
	// Determino cual consigna corresponde aplicar y la aplico.
RtcTimeType rtcDateTime;
u16 now;

	// Hora actual en minutos.
	rtcGetTime(&rtcDateTime);
	now = rtcDateTime.hour * 60 + rtcDateTime.min;

	//           C.diurna                      C.nocturna
	// |----------|-------------------------------|---------------|
	// 0         hhmm1                          hhmm2            24
	//   nocturna             diurna                 nocturna

	if ( now <= systemVars.outputs.horaConsDia ) {
		pb_setConsignaNocturna(CONSIGNA_MS);
		return;
	}

	if ( ( now > systemVars.outputs.horaConsDia ) && ( now <= systemVars.outputs.horaConsNoc )) {
		pb_setConsignaDiurna(CONSIGNA_MS);
		return;
	}

	if ( now > systemVars.outputs.horaConsNoc ) {
		pb_setConsignaNocturna(CONSIGNA_MS);
		return;
	}

}
/*------------------------------------------------------------------------------------*/
static void pv_checkAndSetConsigna(void)
{
RtcTimeType rtcDateTime;
u16 now;

	// Hora actual en minutos.
	rtcGetTime(&rtcDateTime);
	now = rtcDateTime.hour * 60 + rtcDateTime.min;

	if ( now == systemVars.outputs.horaConsDia ) {
		pb_setConsignaDiurna(CONSIGNA_MS);
		return;
	}

	if ( now == systemVars.outputs.horaConsNoc ) {
		pb_setConsignaNocturna(CONSIGNA_MS);
		return;
	}
}
/*------------------------------------------------------------------------------------*/
static s08 ac_consignaTimer(t_consignaTimer action, u16 secs, s08 *retStatus)
{
// Timer de uso general, configurado para modo one-shot.

static s08 f_consigna = FALSE;
s08 ret;

	switch (action ) {
	case INIT_CONSIGNA:
		// Configuro el timer pero no lo prendo
		ret = sTimerCreate(TRUE,  consigna_TimerCallBack , &xTimer_consigna );
		ret = sTimerChangePeriod(xTimer_consigna, 25 );
		break;

	case CALLBACK_CONSIGNA:
		f_consigna = TRUE;
		ret = TRUE;
		break;

	case START_CONSIGNA:
		// Arranco el timer que me va a sacar de este modo
		f_consigna = FALSE;
		ret = sTimerChangePeriod(xTimer_consigna, (u32)(secs));
		ret = sTimerRestart(xTimer_consigna);
		break;

	case STOP_CONSIGNA:
		f_consigna = FALSE;
		ret = sTimerStop(xTimer_consigna);
		break;

	case RESET_CONSIGNA:
		f_consigna = FALSE;
		break;

	case  EXPIRED_CONSIGNA:
		break;
	}

	*retStatus = ret;
	return(f_consigna);
}
/*------------------------------------------------------------------------------------*/
void consigna_TimerCallBack ( void )
{
	ac_consignaTimer(CALLBACK_CONSIGNA, NULL , NULL);
}
/*------------------------------------------------------------------------------------*/

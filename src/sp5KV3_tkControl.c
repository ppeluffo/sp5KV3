/*
 * sp5KV3_tkControl.c
 *
 *  Created on: 7/4/2015
 *      Author: pablo
 *
 *  Tareas de control generales del SP5K
 *  - Recibe un mensaje del timer del led para indicar si debe prender o apagarlo.
 */


#include <sp5KV3.h>

static char ctl_printfBuff[CHAR64];

static void loadSystemParams(void);
void loadDefaults( void );				// Publica
/*------------------------------------------------------------------------------------*/
// LED
typedef enum { CALLBACK_LED, INIT_LED, PRENDER_LED, APAGAR_LED, TOGGLE_LED } t_ledActions;
u08 xTimer_keepAliveLed;
void keepAliveLed_TimerCallBack (void );
static void ac_systemLeds(t_ledActions action);
/*------------------------------------------------------------------------------------*/
// WATCHDOG
typedef enum { CALLBACK_WDG, INIT_WDG, ARM_WDG, CHECK_WDG } t_WdgActions;
u08 xTimer_wdg;
void wdg__TimerCallBack( void );
static void ac_wdg(t_WdgActions action);
/*------------------------------------------------------------------------------------*/
// TERMINAL
typedef enum { CALLBACK_TERM, CHECK_TERM, INIT_TERM, PRENDER_TERM, APAGAR_TERM, ISPRENDIDA_TERM, ISAPAGADA_TERM, RESTART_TERM } t_termActions;
u08 xTimer_terminal;
void terminal_TimerCallBack (void );
static u08 ac_terminal(t_termActions action, u16 secs);
/*------------------------------------------------------------------------------------*/
// EXIT WRKMODE2NORMAL
typedef enum { CALLBACK_EWM2N, INIT_EWM2N, CHECK_EWM2N, START_EWM2N } t_exitActions;
u08 xTimer_ewm2n;
void exitWrkMode_TimerCallBack ( void );
static void ac_exitWrkMode2Normal(t_exitActions action);

/*------------------------------------------------------------------------------------*/

void tkControl(void * pvParameters)
{

( void ) pvParameters;
BaseType_t xResult;
uint32_t ulNotifiedValue;

	MCP_init();

	// TERMINAL: Debe ser lo primero que incia para poder mandar mensajes de log.
	ac_terminal(INIT_TERM, NULL);

	loadSystemParams();

	// WATCHDOG
	ac_wdg( INIT_WDG );

	// LEDS
	ac_systemLeds( INIT_LED );

	// EXITWRKMODE2NORMAL
	ac_exitWrkMode2Normal(INIT_EWM2N);

	// MEMORIA
	MEM_init();
	snprintf_P( ctl_printfBuff,CHAR256,PSTR("Init memory: pWr=%d,pRd=%d,pDel=%d,free=%d,4rd=%d,4del=%d  \r\n"), MEM_getWrPtr(), MEM_getRdPtr(), MEM_getDELptr(), MEM_getRcdsFree(),MEM_getRcds4rd(),MEM_getRcds4del() );
	TERMrprintfStr( ctl_printfBuff );

	// Habilito arrancar otras tareas
	startToken = STOK_TIMERS;

	// Espero la notificacion para arrancar
	while ( startToken != STOK_CTL ) {
		vTaskDelay( ( TickType_t)( 100 / portTICK_RATE_MS ) );
	}
	TERMrprintfProgStrM("starting tkControl..\r\n");
	startToken++;

	// Loop
	for( ;; )
	{
		clearWdg(WDG_CTL);

		// LED
		ac_systemLeds(TOGGLE_LED);

		// WATCHDOG
		ac_wdg(CHECK_WDG);

		// TERMINAL
		ac_terminal(CHECK_TERM, NULL);

		// EXITWRKMODE2NORMAL
		ac_exitWrkMode2Normal(CHECK_EWM2N);

		// Genero una espera de 100ms por algun mensaje para hacer algo.
		xResult = xTaskNotifyWait( 0x00,          	 				/* Don't clear bits on entry. */
		                           ULONG_MAX,        				/* Clear all bits on exit. */
		                           &ulNotifiedValue, 				/* Stores the notified value. */
								   (100 / portTICK_RATE_MS ) );

		if( xResult == pdTRUE ) {
			// Arrancar el timer y control de modo service/monitor
			if ( ( ulNotifiedValue & CTLMSG_STARTEWM2N ) != 0 ) {
				ac_exitWrkMode2Normal(START_EWM2N);
		    }

		 }
	}
}

/*------------------------------------------------------------------------------------*/
// WATCHDOG
static void ac_wdg(t_WdgActions action)
{

static s08 f_WDGCallback = FALSE;
static u32 counter = 864000;	// 24hs*60m*60s*10ticks/s.

	// Daily reset.
	// Una vez al dia me reseteo.
	if ( counter-- == 0 ) {
		wdt_enable(WDTO_30MS);
		while(1) {}
	}

	switch(action) {
	case INIT_WDG:
		f_WDGCallback = FALSE;
		// Activo el timer del WDG.
		sTimerCreate(TRUE, wdg__TimerCallBack , &xTimer_wdg );
		sTimerChangePeriod(xTimer_wdg, 1 );
		sTimerStart(xTimer_wdg);

		ac_wdg(ARM_WDG);
		break;

	case ARM_WDG:
		wdt_reset();
		// Armo los wdg individuales.
		systemWdg = WDG_TIMERS + WDG_CTL + WDG_CMD + WDG_EVN + WDG_DIN + WDG_AIN + WDG_GPRS + WDG_OUT; //  1111 1111 = 0xFF
		break;

	case CALLBACK_WDG:
		f_WDGCallback = TRUE;
		break;

	case CHECK_WDG:
		if ( f_WDGCallback ) {
			f_WDGCallback = FALSE;
			if ( systemWdg == 0 ) {
				ac_wdg(ARM_WDG);
			}
		}
		break;
	}
}

void wdg__TimerCallBack( void )
{
	// Este timer expira c/1seg. Si todas las flags de watchdogs individuales estan
	// en 0, puedo resetear el watchdog del sistema.
	// En caso contrario, el watchdog del sistema va a expirar y resetear el sistema.

	ac_wdg(CALLBACK_WDG);
}
/*------------------------------------------------------------------------------------*/
// TERMINAL
s08 terminal_isPrendida(void)
{
	return( ac_terminal(ISPRENDIDA_TERM, NULL));

}

s08 terminal_isApagada(void)
{
	return( ac_terminal(ISAPAGADA_TERM, NULL));

}

void terminal_restartTimer( u16 secs)
{
	ac_terminal(RESTART_TERM, secs);
}

static u08 ac_terminal(t_termActions action, u16 secs)
{

static s08 f_TERMCallback = FALSE;
static s08 f_TERMisOn = FALSE;
u08 tpin;

	switch(action) {
	case INIT_TERM:
		cbi(TERMSW_DDR, TERMSW_BIT);	// El pin de la terminal es una entrada
		MCP_setTermPwr(1);				// La terminal arranca inicialmente prendida
		f_TERMisOn = TRUE;

		sTimerCreate(TRUE, terminal_TimerCallBack , &xTimer_terminal );
		sTimerChangePeriod(xTimer_terminal, (u32)SECS2TERMOFF);
		sTimerStart(xTimer_terminal);

		f_TERMCallback = FALSE;
		break;

	case PRENDER_TERM:
		MCP_setTermPwr(1);
		f_TERMisOn = TRUE;
		TERMrprintfProgStrM("Terminal going on..\r\n");
		// Prendida muestreo c/2mins
		ac_terminal(RESTART_TERM, SECS2TERMOFF);
		break;

	case APAGAR_TERM:
		TERMrprintfProgStrM("Terminal going off..\r\n");
		MCP_setTermPwr(0);
		f_TERMisOn = FALSE;
		// Apagada muestreo c/1s.
		ac_terminal(RESTART_TERM, 1);
		break;

	case CALLBACK_TERM:
		f_TERMCallback = TRUE;
		break;

	case ISPRENDIDA_TERM:
		return(f_TERMisOn);
		break;

	case ISAPAGADA_TERM:
		return(~f_TERMisOn);
		break;

	case RESTART_TERM:
		sTimerChangePeriod( xTimer_terminal, (u32)(secs) );
		break;

	case CHECK_TERM:
		if ( f_TERMCallback ) {
			f_TERMCallback = FALSE;

			// En modo service o monitor, la terminal esta prendida por lo que no debo
			// chequear por apagarla.
			if ( systemVars.wrkMode == WK_SERVICE ) return(TRUE);
			if ( systemVars.wrkMode == WK_MONITOR_FRAME ) return(TRUE);
			if ( systemVars.wrkMode == WK_MONITOR_SQE )  return(TRUE);
			if ( systemVars.pwrMode == PWR_CONTINUO )  return(TRUE);

			// Leo el estado del pin de la terminal ( 0 o 1 )
			tpin = ( TERMSW_PIN & _BV(TERMSW_BIT) ) >> TERMSW_BIT;

			// Terminal apagada que debe prenderse
			if ( terminal_isApagada() && ( tpin == ON ) ) {
				ac_terminal( PRENDER_TERM, NULL);
				return(TRUE);
			}

			// Terminal prendida que debo apagarla
			if ( terminal_isPrendida() && ( tpin == OFF ) ) {
				ac_terminal( APAGAR_TERM, NULL );
				return(TRUE);
			}
		}

		// Si la terminal estaba prendida, vuelvo a chequear en 2 mins.
		// Si estaba apagada, en 1s porque el timer es autoreload.
		break;
	}

	return(TRUE);
}

void terminal_TimerCallBack( void )
{
	ac_terminal(CALLBACK_TERM,NULL);
}
/*------------------------------------------------------------------------------------*/
// LED KEEPALIVE
static void ac_systemLeds(t_ledActions action)
{
	// Todas las funciones las englobo en este modulo.

static s08 f_KALisOn = FALSE;
static s08 f_KALCallback = FALSE;
//u32 tickCount;

	switch (action ) {
	case INIT_LED:
		sbi(LED_KA_DDR, LED_KA_BIT);		// El pin del led de KA ( PD6 ) es una salida.
		sbi(LED_MODEM_DDR, LED_MODEM_BIT);	// El pin del led de KA ( PD6 ) es una salida.

		// Activo el timer del led.
		sTimerCreate(TRUE, keepAliveLed_TimerCallBack , &xTimer_keepAliveLed );
		sTimerChangePeriod(xTimer_keepAliveLed, 2 );
		sTimerStart(xTimer_keepAliveLed);

		// Prendo.
		ac_systemLeds(PRENDER_LED);

		f_KALCallback = FALSE;
		break;

	case PRENDER_LED:
		// Solo lo prendo si hay una terminal prendida
		if ( terminal_isPrendida() ) {

			f_KALisOn = TRUE;				// Flag que indica que los leds estan prendido.
			MCP_setLed(1);					// Led placa logica
			cbi(LED_KA_PORT, LED_KA_BIT);	// Led placa analogica ( kalive )

			// El led del modem prende solo si este esta prendido.
			if ( !gprsAllowSleep() ) {		// Led placa analogica ( modem )
				cbi(LED_MODEM_PORT, LED_MODEM_BIT);
			}
		}
		break;

	case APAGAR_LED:
		f_KALisOn = FALSE;
		MCP_setLed(0);						// Led placa logica
		sbi(LED_KA_PORT, LED_KA_BIT);		// Led placa analogica ( kalive )
		sbi(LED_MODEM_PORT, LED_MODEM_BIT); // Led placa analogica ( modem )
		break;

	case CALLBACK_LED:
		f_KALCallback = TRUE;
		break;

	case TOGGLE_LED:
		// Primero lo apago siempre
		if ( f_KALisOn ) {
			ac_systemLeds(APAGAR_LED);
		} else {
			// Lo prendo solo cuando lo indica el callback
			if ( f_KALCallback ) {
				f_KALCallback = FALSE;
				ac_systemLeds(PRENDER_LED);
			}
		}
		break;
	}
}

void keepAliveLed_TimerCallBack( void )
{
	// Prende una flag.
	ac_systemLeds( CALLBACK_LED);
}
/*------------------------------------------------------------------------------------*/
// EXITWRKMODE2NORMAL
void exitWrkMode_TimerCallBack ( void )
{
	ac_exitWrkMode2Normal(CALLBACK_EWM2N);
}
static void ac_exitWrkMode2Normal(t_exitActions action)
{

static s08 f_EWM2NCallback = FALSE;
u32 tickCount;

	switch (action ) {
	case INIT_EWM2N:
		// Configuro el timer pero no lo prendo
		sTimerCreate(FALSE, exitWrkMode_TimerCallBack , &xTimer_ewm2n );
		sTimerChangePeriod(xTimer_ewm2n, 1 );
		break;

	case CALLBACK_EWM2N:
		f_EWM2NCallback = TRUE;
		break;

	case CHECK_EWM2N:
		if (f_EWM2NCallback) {
			f_EWM2NCallback = FALSE;
			// Fijo el modo normal
			while ( xSemaphoreTake( sem_SYSVars, ( TickType_t ) 10 ) != pdTRUE )
				taskYIELD();
			systemVars.wrkMode = WK_NORMAL;
			xSemaphoreGive( sem_SYSVars );

			// Aviso a la tarea tkAnalogIn para que recargue la configuracion
			xTaskNotify(xHandle_tkAIn, AINMSG_RELOAD , eSetBits );
		}
		break;
	case START_EWM2N:
		// Arranco el timer que me va a sacar de este modo
		sTimerChangePeriod(xTimer_ewm2n, (u32)TO2SERVICEMODE );
		sTimerRestart(xTimer_ewm2n);
		f_EWM2NCallback = FALSE;
		break;
	}
}
//------------------------------------------------------------------------------------
// GENERAL
static void loadSystemParams(void)
{
s08 eeStatus = FALSE;
u08 i;

	// Leo la configuracion:  Intento leer hasta 3 veces.
	for (i=0; i<3; i++) {
		eeStatus = paramLoad( &systemVars, EEADDR_START_PARAMS, sizeof(systemVarsType));
		if (eeStatus == TRUE) break;
		vTaskDelay( 200 / portTICK_RATE_MS );
	}

	// Confirmo que pude leer bien.
	if ( (eeStatus == FALSE) || (systemVars.initByte != 0x49) ) {
		// No pude leer la EE correctamente o es la primera vez que arranco.
		// Inicializo a default.
		loadDefaults();
		snprintf_P( ctl_printfBuff,CHAR64,PSTR("loadSystemParams::Load EE error: default !!\r\n") );
		TERMrprintfStr( ctl_printfBuff );

	} else {
		// Pude leer la eeprom correctamente.
		snprintf_P( ctl_printfBuff,CHAR64,PSTR("loadSystemParams::Load config ok !!\r\n") );
		TERMrprintfStr( ctl_printfBuff );
	}

	// Ajustes de inicio:
	strncpy_P(systemVars.dlgIp, PSTR("000.000.000.000\0"),16);
	systemVars.csq = 0;
	systemVars.dbm = 0;
	systemVars.dcd = 0;
	systemVars.ri = 0;
	systemVars.termsw = 0;
	systemVars.wrkMode = WK_NORMAL;

	snprintf_P( ctl_printfBuff,CHAR64,PSTR("loadSystemParams::FW %d analogs\r\n"), NRO_CHANNELS );
	TERMrprintfStr( ctl_printfBuff );
}

void loadDefaults( void )
{
	// Valores por default.
u08 channel;

	while ( xSemaphoreTake( sem_SYSVars, ( TickType_t ) 10 ) != pdTRUE )
		taskYIELD();

	systemVars.initByte = 0x49;
	strncpy_P(systemVars.dlgId, PSTR("SPY999\0"),DLGID_LENGTH);
	strncpy_P(systemVars.serverPort, PSTR("80\0"),PORT_LENGTH	);
	strncpy_P(systemVars.passwd, PSTR("spymovil123\0"),PASSWD_LENGTH);
	strncpy_P(systemVars.serverScript, PSTR("/cgi-bin/sp5K/sp5K.pl\0"),SCRIPT_LENGTH);
//	strncpy_P(sVars->serverScript, PSTR("/cgi-bin/sp5K/test.pl\0"),SCRIPT_LENGTH);

	systemVars.csq = 0;
	systemVars.dbm = 0;
	systemVars.gsmBand = 8;
	systemVars.dcd = 0;
	systemVars.ri = 0;
	systemVars.termsw = 0;
	systemVars.wrkMode = WK_NORMAL;
	systemVars.pwrMode = PWR_CONTINUO;

	strncpy_P(systemVars.apn, PSTR("SPYMOVIL.VPNANTEL\0"),APN_LENGTH);

	// DEBUG
	//systemVars.debugLevel = D_NONE;
	//systemVars.debugLevel = D_DATA;
	//systemVars.debugLevel = D_GPRS + D_DIGITAL;
	//systemVars.debugLevel = D_BASIC + D_GPRS;
	systemVars.debugLevel = D_BASIC;

	strncpy_P(systemVars.serverAddress, PSTR("192.168.0.9\0"),IP_LENGTH);
	systemVars.timerPoll = 60;			// Poleo c/5 minutos
	systemVars.timerDial = 10800;		// Transmito c/3 hs.

	systemVars.pwrSave = modoPWRSAVE_OFF;
	systemVars.pwrSaveStartTime =pv_convertHHMM2min(2230);	// 22:30 PM
	systemVars.pwrSaveEndTime =	pv_convertHHMM2min(630);	// 6:30 AM

	// Todos los canales quedan por default en 0-20mA, 0-6k.
	for ( channel = 0; channel < NRO_CHANNELS; channel++) {
		systemVars.Imin[channel] = 0;
		systemVars.Imax[channel] = 20;
		systemVars.Mmin[channel] = 0;
		systemVars.Mmax[channel] = 6.0;
		systemVars.offmmin[channel] = 0.0;
		systemVars.offmmax[channel] = 0.0;
	}

	strncpy_P(systemVars.aChName[0], PSTR("pA\0"),3);
	strncpy_P(systemVars.aChName[1], PSTR("pB\0"),3);
	strncpy_P(systemVars.aChName[2], PSTR("pC\0"),3);

	// Canales digitales
	strncpy_P(systemVars.dChName[0], PSTR("v0\0"),3);
	systemVars.magPP[0] = 0.1;
	strncpy_P(systemVars.dChName[1], PSTR("v1\0"),3);
	systemVars.magPP[1] = 0.1;

	// Salidas
	systemVars.outputs.wrkMode = OUTPUT_CONSIGNA;
	systemVars.outputs.dout[0] = 0;
	systemVars.outputs.dout[1] = 0;
	systemVars.outputs.dout[2] = 0;
	systemVars.outputs.dout[3] = 0;
	systemVars.outputs.horaConsDia = pv_convertHHMM2min(530);		// Consigna diurna
	systemVars.outputs.horaConsNoc = pv_convertHHMM2min(2330);		// Consigna nocturna
	systemVars.outputs.chVA = 0;
	systemVars.outputs.chVB = 1;

	// Almaceno en la EE para el proximo arranque.
	paramStore(&systemVars, EEADDR_START_PARAMS, sizeof(systemVars));

	xSemaphoreGive( sem_SYSVars );

}
/*------------------------------------------------------------------------------------*/




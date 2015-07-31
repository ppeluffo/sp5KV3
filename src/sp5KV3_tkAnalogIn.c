/*
 * sp5KV3_tkAnalogIn.c
 *
 *  Created on: 14/4/2015
 *      Author: pablo
 */


#include <sp5KV3.h>

char aIn_printfBuff[CHAR128];

// Estados
typedef enum {	tkdST_INIT 		= 0,
				tkdST_IDLE		= 1,
				tkdST_AWAITPWR	= 2,
				tkdST_POLL		= 3
} t_tkData_state;

// Eventos
typedef enum {
	dEV00 = 0, 	// Init
	dEV01 = 1,	// EV_MSGreload
	dEV02 = 2,	// EV_MSGCmdPoll
	dEV03 = 3,	// EV_timerPollExpired
	dEV04 = 4,	// EV_aInPwrOnTimerExpired
	dEV05 = 5,	// EV_pollCounterNOT_0

} t_tkData_eventos;

#define dEVENT_COUNT		6

static s08 dEventos[dEVENT_COUNT];

// transiciones
static int trD00_fn(void);
static int trD01_fn(void);
static int trD02_fn(void);
static int trD03_fn(void);
static int trD04_fn(void);
static int trD05_fn(void);
static int trD06_fn(void);

static u08 nroPoleos;					// Contador del nro de poleos
static u08 tkAIN_state;					// Estado
static u32 tickCount;					// para usar en los mensajes del debug.
static s08 initStatus;
static s08 f_msgReload, f_msgPoll;		// flags de los mensajes recibidos.

#define CICLOS_POLEO			3		// ciclos de poleo para promediar.

double rAIn[NRO_CHANNELS + 1];			// Almaceno los datos de conversor A/D

// Funciones generales
static void AIN_getNextEvent(void);
static void pvAIN_printExitMsg(u08 code);
static double pvReadADCchannel( u08 ADCchannel, u08 DLGchannel  ) ;
static void pv_AinLoadParameters( void );

static frameDataType Aframe;
//--------------------------------------------------------------------------------------
// AIN_PWRON_TIMER
typedef enum { CALLBACK_AINPWRON, INIT_AINPWRON, START_AINPWRON, EXPIRED_AINPWRON } t_AinPwrOnActions;
u08 xTimer_aInPwrOn;
void aInPwrOn_TimerCallBack (void );
static s08 ac_aInPwrOnTimer(t_AinPwrOnActions action, u16 secs);
//--------------------------------------------------------------------------------------
// AIN_POLL_TIMER
typedef enum { CALLBACK_AINPOLL, INIT_AINPOLL, START_AINPOLL, STOP_AINPOLL } t_AinPollActions;
u08 xTimer_aInPoll;
s08 f_pollEnabled = TRUE;
void aInPoll_TimerCallBack (void );
static void ac_aInPollTimer(t_AinPollActions action, u16 secs);

/*------------------------------------------------------------------------------------*/
void tkAnalogIn(void * pvParameters)
{

( void ) pvParameters;
BaseType_t xResult;
uint32_t ulNotifiedValue;

	vTaskDelay( ( TickType_t)( 1000 / portTICK_RATE_MS ) );

	// Espero la notificacion para arrancar
	while ( startToken != STOK_AIN ) {
		vTaskDelay( ( TickType_t)( 100 / portTICK_RATE_MS ) );
	}
	TERMrprintfProgStrM("starting tkAnalogIn..\r\n");
	startToken++;

	// Creo los timers
	ac_aInPwrOnTimer( INIT_AINPWRON, NULL );
	ac_aInPollTimer ( INIT_AINPOLL, NULL );

	tkAIN_state = tkdST_INIT;
	initStatus = TRUE;			// Evento inicial ( arranque ).
	f_msgReload = FALSE;
	//
	for( ;; )
	{

		clearWdg(WDG_AIN);

		// Espero hasta 100ms por un mensaje.
		xResult = xTaskNotifyWait( 0x00, ULONG_MAX, &ulNotifiedValue, ((TickType_t) 100 / portTICK_RATE_MS ) );
		// Si llego un mensaje, prendo la flag correspondiente.
		f_msgPoll = FALSE;
		if ( xResult == pdTRUE ) {
			if ( ( ulNotifiedValue & AINMSG_RELOAD ) != 0 ) {
				f_msgReload = TRUE;
			}

			if ( ( ulNotifiedValue & AINMSG_POLL ) != 0 ) {
				f_msgPoll = TRUE;
			}
		}

		// Analizo los eventos.
		AIN_getNextEvent();

		// El manejar la FSM con un switch por estado y no por transicion me permite
		// priorizar las transiciones.
		// Luego de c/transicion debe venir un break así solo evaluo de a 1 transicion por loop.
		//
		switch ( tkAIN_state ) {
		case tkdST_INIT:
			tkAIN_state = trD00_fn();	// TR00
			break;

		case tkdST_IDLE:
			if ( dEventos[dEV01] == TRUE  ) { tkAIN_state = trD01_fn();break; }
			if ( dEventos[dEV02] == TRUE  ) { tkAIN_state = trD02_fn();break; }
			if ( dEventos[dEV03] == TRUE  ) { tkAIN_state = trD03_fn();break; }
			break;

		case tkdST_AWAITPWR:
			if ( dEventos[dEV04] == TRUE  ) { tkAIN_state = trD04_fn();break; }
			break;

		case tkdST_POLL:
			if ( dEventos[dEV05] == TRUE  ) { tkAIN_state = trD05_fn();break; }
			tkAIN_state = trD06_fn();
			break;

		default:
			TERMrprintfProgStrM("tkAnalogIn::ERROR state NOT DEFINED..\r\n");
			tkAIN_state  = tkdST_INIT;
			break;

		}

	}

}
/*------------------------------------------------------------------------------------*/
static void AIN_getNextEvent(void)
{
// Evaluo todas las condiciones que generan los eventos que disparan las transiciones.
// Tenemos un array de eventos y todos se evaluan.

u08 i;

	// Inicializo la lista de eventos.
	for ( i=0; i < dEVENT_COUNT; i++ ) {
		dEventos[i] = FALSE;
	}

	// Evaluo los eventos
	// EV00: INIT
	if ( initStatus == TRUE ) { dEventos[dEV00] = TRUE; }

	// EV01: EV_MSGreload
	if ( f_msgReload == TRUE ) { dEventos[dEV01] = TRUE; }

	// EV02: EV_MSGCmdPoll
	if ( f_msgPoll == TRUE ) { dEventos[dEV02] = TRUE; }

	// EV03: EV_timerPollExpired: poll enable.
	if ( f_pollEnabled == TRUE ) { dEventos[dEV03] = TRUE; }

	// EV04: EV_aInPwrOnTimerExpired
	if (  ac_aInPwrOnTimer(EXPIRED_AINPWRON, NULL) == TRUE ) { dEventos[dEV04] = TRUE; }

	// EV05: EV_pollCounterNOT_0
	if ( nroPoleos != 0 ) { dEventos[dEV05] = TRUE; }

}
/*------------------------------------------------------------------------------------*/
static int trD00_fn(void)
{
	// Evento inicial. Solo salta al primer estado operativo.
	// Inicializo el sistema aqui
	// tkdST_INIT->tkdST_IDLE


	// Init (load parameters) & start pollTimer
	initStatus = FALSE;
	pv_AinLoadParameters();

	if ( systemVars.pwrMode == PWR_DISCRETO ) {
		// Apagar los sensores.
		MCP_setSensorPwr( 0 );
		MCP_setAnalogPwr( 0 );
		tickCount = xTaskGetTickCount();
		snprintf_P( aIn_printfBuff,CHAR128,PSTR(".[%06lu] tkAnalogIn::trD00 pwroff sensors\r\n"), tickCount);
	} else {
		// Prender los sensores.
		MCP_setSensorPwr( 1 );
		MCP_setAnalogPwr( 1 );
		tickCount = xTaskGetTickCount();
		snprintf_P( aIn_printfBuff,CHAR128,PSTR(".[%06lu] tkAnalogIn::trD00 pwron sensors\r\n"), tickCount);
	}

	if ( (systemVars.debugLevel & D_DATA) != 0) {
		TERMrprintfStr( aIn_printfBuff );
	}

	pvAIN_printExitMsg(0);
	return(tkdST_IDLE);
}
/*------------------------------------------------------------------------------------*/
static int trD01_fn(void)
{
	// MSG de autoreload
	// tkdST_IDLE->tkdST_IDLE

	f_msgReload = FALSE;
	// Init (load parameters) & start pollTimer
	pv_AinLoadParameters();

	if ( systemVars.pwrMode == PWR_DISCRETO ) {
		// Apagar los sensores.
		MCP_setSensorPwr( 0 );
		MCP_setAnalogPwr( 0 );
		tickCount = xTaskGetTickCount();
		snprintf_P( aIn_printfBuff,CHAR128,PSTR(".[%06lu] tkAnalogIn::trD01 pwroff sensors\r\n"), tickCount);
	} else {
		// Prender los sensores.
		MCP_setSensorPwr( 1 );
		MCP_setAnalogPwr( 1 );
		tickCount = xTaskGetTickCount();
		snprintf_P( aIn_printfBuff,CHAR128,PSTR(".[%06lu] tkAnalogIn::trD01 pwron sensors\r\n"), tickCount);
	}

	if ( (systemVars.debugLevel & D_DATA) != 0) {
		TERMrprintfStr( aIn_printfBuff );
	}

	pvAIN_printExitMsg(1);
	return(tkdST_IDLE);
}
/*------------------------------------------------------------------------------------*/
static int trD02_fn(void)
{
	// tkdST_IDLE->tkdST_AWAITPWR

	// A este estado llego porque expiro el timer. Aunque es autoreload, debo resetear
	// la flag que uso para detectar el evento.
	f_pollEnabled = FALSE;

	//  En modo discreto debo prender sensores
	if ( systemVars.pwrMode == PWR_DISCRETO ) {
		MCP_setSensorPwr( 1 );
		MCP_setAnalogPwr( 1 );
		tickCount = xTaskGetTickCount();
		snprintf_P( aIn_printfBuff,CHAR128,PSTR(".[%06lu] tkAnalogIn::trD02 pwron sensors\r\n"), tickCount);
		if ( (systemVars.debugLevel & D_DATA) != 0) {
			TERMrprintfStr( aIn_printfBuff );
		}
	}

	// ac_initSensorPwrTimer: Espero que se estabilize por 5s.
	ac_aInPwrOnTimer(START_AINPWRON, 5);

	pvAIN_printExitMsg(2);
	return(tkdST_AWAITPWR);
}
/*------------------------------------------------------------------------------------*/
static int trD03_fn(void)
{
	// tkdST_IDLE->tkdST_AWAITPWR

	// A este estado llego porque expiro el timer. Aunque es autoreload, debo resetear
	// la flag que uso para detectar el evento.
	f_pollEnabled = FALSE;

	//  En modo discreto debo prender sensores
	if ( systemVars.pwrMode == PWR_DISCRETO ) {
		MCP_setSensorPwr( 1 );
		MCP_setAnalogPwr( 1 );
		tickCount = xTaskGetTickCount();
		snprintf_P( aIn_printfBuff,CHAR128,PSTR(".[%06lu] tkAnalogIn::trD03 pwron sensors\r\n"), tickCount);
		if ( (systemVars.debugLevel & D_DATA) != 0) {
			TERMrprintfStr( aIn_printfBuff );
		}
	}

	// ac_initSensorPwrTimer: Espero que se estabilize por 5s.
	ac_aInPwrOnTimer(START_AINPWRON, 5);

	pvAIN_printExitMsg(3);
	return(tkdST_AWAITPWR);
}
/*------------------------------------------------------------------------------------*/
static int trD04_fn(void)
{
	// tkdST_AWAITPWR->tkdST_POLL

u08 channel;

	nroPoleos = CICLOS_POLEO;

	// Init Data Structure
	for ( channel = 0; channel < ( NRO_CHANNELS + 1 ); channel++) {
		rAIn[channel] = 0;
	}

	pvAIN_printExitMsg(4);
	return(tkdST_POLL);
}
/*------------------------------------------------------------------------------------*/
static int trD05_fn(void)
{
	// tkdST_POLL->tkdST_POLL
	// Poleo
u16 adcRetValue;

	if ( nroPoleos > 0 ) {
		nroPoleos--;
	}

	// Dummy convert para prender el ADC ( estabiliza la medida).
	// Esta espera de 1.5s es util entre ciclos de poleo.
	ADS7828_convert( 0, &adcRetValue );
	vTaskDelay( (portTickType)(1500 / portTICK_RATE_MS) );
	//
	rAIn[0] += pvReadADCchannel(3,0);	// AIN0->ADC3;
	rAIn[1] += pvReadADCchannel(5,1);	// AIN1->ADC5;
	rAIn[2] += pvReadADCchannel(7,2);   // AIN2->ADC7;
	rAIn[3] += pvReadADCchannel(1,3);	// BATT->ADC1;

	pvAIN_printExitMsg(5);
	return(tkdST_POLL);

}
/*------------------------------------------------------------------------------------*/
static int trD06_fn(void)
{
	// tkdST_POLL->tkdST_IDLE

double I,M;
u16 D;
u08 channel;
u16 pos = 0;
s08 retMEM = 0xFF;

	//  En modo discreto debo apagar sensores
	if ( systemVars.pwrMode == PWR_DISCRETO ) {
		MCP_setSensorPwr( 0 );
		MCP_setAnalogPwr( 0 );
		tickCount = xTaskGetTickCount();
		snprintf_P( aIn_printfBuff,CHAR128,PSTR(".[%06lu] tkAnalogIn::trD06 pwroff sensors\r\n"), tickCount);
		if ( (systemVars.debugLevel & D_DATA) != 0) {
			TERMrprintfStr( aIn_printfBuff );
		}
	}

	// Promedio:
	for ( channel = 0; channel < ( NRO_CHANNELS + 1 ); channel++) {
		rAIn[channel] /= CICLOS_POLEO;
		tickCount = xTaskGetTickCount();
		snprintf_P( aIn_printfBuff,CHAR128,PSTR(".[%06lu] tkAnalogIn::trD06 AvgAdcIn[%d]=%.1f\r\n"), tickCount, channel, rAIn[channel]);
		if ( (systemVars.debugLevel & D_DATA) != 0) {
			TERMrprintfStr( aIn_printfBuff );
		}
	}

	// Convierto de ADC a magnitudes.
	for ( channel = 0; channel < NRO_CHANNELS ; channel++) {
		// Calculo la corriente
		I = rAIn[channel] * 20 / 4096;
		// Calculo la pendiente
		M = 0;
		D = systemVars.Imax[channel] - systemVars.Imin[channel];
		if ( D != 0 ) {
			M = ( (systemVars.Mmax[channel] + systemVars.offmmax[channel]) - ( systemVars.Mmin[channel] + systemVars.offmmin[channel]) ) / D;
		}
		rAIn[channel] = M * ( I - systemVars.Imin[channel] ) + systemVars.Mmin[channel] + systemVars.offmmin[channel];
	}

	rAIn[NRO_CHANNELS] = (15 * rAIn[NRO_CHANNELS]) / 4096;	// Bateria

	// Paso al systemVars.
	while ( xSemaphoreTake( sem_SYSVars, ( TickType_t ) 10 ) != pdTRUE )
		taskYIELD();

	switch ( systemVars.wrkMode ) {
	case WK_SERVICE:
		pos = snprintf_P( aIn_printfBuff, CHAR128, PSTR("service::{" ));
		break;
	case WK_MONITOR_FRAME:
		pos = snprintf_P( aIn_printfBuff, CHAR128, PSTR("monitor::{" ));
		break;
	default:
		pos = snprintf_P( aIn_printfBuff, CHAR128, PSTR("frame::{" ));
	}

	// Marco que el recd esta ocupado.
	Aframe.tagByte = 0xC5;

	// Inserto el timeStamp.
	rtcGetTime(&Aframe.rtc);
	pos += snprintf_P( &aIn_printfBuff[pos], CHAR128,PSTR( "%04d%02d%02d,"),Aframe.rtc.year,Aframe.rtc.month,Aframe.rtc.day );
	pos += snprintf_P( &aIn_printfBuff[pos], CHAR128, PSTR("%02d%02d%02d,"),Aframe.rtc.hour,Aframe.rtc.min, Aframe.rtc.sec );

	// Valores analogicos
	Aframe.analogIn[0] = rAIn[0];
	Aframe.analogIn[1] = rAIn[1];
	Aframe.analogIn[2] = rAIn[2];
	for ( channel = 0; channel < NRO_CHANNELS; channel++) {
		pos += snprintf_P( &aIn_printfBuff[pos], CHAR128, PSTR("%s=%.2f,"),systemVars.aChName[channel],Aframe.analogIn[channel] );
	}

	// Leo los datos digitales y los pongo en 0.
	getDigitalInputs( &Aframe.dIn, TRUE );
	// Convierto los pulsos a los valores de la magnitud.
	Aframe.dIn.din0_pulses *=  systemVars.magPP[0];
	Aframe.dIn.din1_pulses *=  systemVars.magPP[1];
	pos += snprintf_P( &aIn_printfBuff[pos], CHAR128, PSTR("%sP=%.2f,%sL=%d,%sW=%d,"), systemVars.dChName[0],Aframe.dIn.din0_pulses,systemVars.dChName[0],Aframe.dIn.din0_level,systemVars.dChName[0], Aframe.dIn.din0_width);
	pos += snprintf_P( &aIn_printfBuff[pos], CHAR128, PSTR("%sP=%.2f,%sL=%d,%sW=%d"), systemVars.dChName[1],Aframe.dIn.din1_pulses,systemVars.dChName[1],Aframe.dIn.din1_level,systemVars.dChName[1], Aframe.dIn.din1_width);

	// Bateria
	Aframe.batt = rAIn[3];
	pos += snprintf_P( &aIn_printfBuff[pos], CHAR128, PSTR(",bt=%.2f}\r\n\0"),Aframe.batt );

	xSemaphoreGive( sem_SYSVars );

	// Print
	if ( (systemVars.debugLevel & D_BASIC) != 0) {
		TERMrprintfStr( aIn_printfBuff );
	}

	// BD SAVE FRAME solo en modo normal.
	if ( systemVars.wrkMode == WK_NORMAL ) {
		retMEM = MEM_write( &Aframe, sizeof(Aframe) );
		if ( retMEM != MEM_OK ) {
			tickCount = xTaskGetTickCount();
			snprintf_P( aIn_printfBuff,CHAR128,PSTR(".[%06lu] tkAnalogIn::trD06 wrMEM ERROR = %d!!!\r\n"), tickCount, retMEM);
			TERMrprintfStr( aIn_printfBuff );
		}
		snprintf_P( aIn_printfBuff,CHAR256,PSTR("memStatus: pWr=%d,pRd=%d,pDel=%d,free=%d,4rd=%d,4del=%d  \r\n"), MEM_getWrPtr(), MEM_getRdPtr(), MEM_getDELptr(), MEM_getRcdsFree(),MEM_getRcds4rd(),MEM_getRcds4del() );
		if ( (systemVars.debugLevel & D_BASIC) != 0) {
			TERMrprintfStr( aIn_printfBuff );
		}

	}

	// Indico a tkEventos que hay datos validos si no estoy en monitor
	if ( systemVars.wrkMode == WK_NORMAL ) {
		xTaskNotify( xHandle_tkEventos, FRAMERDY_BIT , eSetBits );
	}

	pvAIN_printExitMsg(6);
	return(tkdST_IDLE);

}
/*------------------------------------------------------------------------------------*/
static void pvAIN_printExitMsg(u08 code)
{
	if ( (systemVars.debugLevel & D_DATA) != 0) {
		tickCount = xTaskGetTickCount();
		snprintf_P( aIn_printfBuff,CHAR128,PSTR(".[%06lu] tkAnalogIn::exit TR%02d\r\n"), tickCount,code);
		TERMrprintfStr( aIn_printfBuff );
	}
}
/*------------------------------------------------------------------------------------*/
// TIMER_POLL ( autoreload )
static void ac_aInPollTimer(t_AinPollActions action, u16 secs)
{
u32 pollCounter;

	switch (action ) {
	case INIT_AINPOLL:
		// Configuro el timer pero no lo prendo
		sTimerCreate(TRUE, aInPoll_TimerCallBack , &xTimer_aInPoll );
		sTimerChangePeriod(xTimer_aInPoll, 1 );
		break;

	case CALLBACK_AINPOLL:
		f_pollEnabled = TRUE;
		// Confirmo el periodo
		sTimersInfo(T_COUNTER, xTimer_aInPoll, &pollCounter );
		if ( ( pollCounter ) != systemVars.timerPoll ) {
			f_msgReload = FALSE;
			tickCount = xTaskGetTickCount();
			snprintf_P( aIn_printfBuff,CHAR128,PSTR(".[%06lu] tkAnalogIn::Callback adj.timerPoll(%lu)\r\n"), tickCount, pollCounter);
			TERMrprintfStr( aIn_printfBuff );
		}
		break;

	case START_AINPOLL:
		// Arranco el timer que me va a sacar de este modo
		sTimerChangePeriod(xTimer_aInPoll, (u32)(secs) );
		sTimerRestart(xTimer_aInPoll);
		break;

	case  STOP_AINPOLL:
		sTimerStop(xTimer_aInPoll);
		break;
	}

}

void aInPoll_TimerCallBack ( void )
{
	ac_aInPollTimer(CALLBACK_AINPOLL,NULL);
}
/*------------------------------------------------------------------------------------*/
// AIN_TIMER_PWR_ON ( on shot )
static s08 ac_aInPwrOnTimer(t_AinPwrOnActions action, u16 secs)
{

static s08 f_AINPWRONExpired = FALSE;

	switch (action ) {
	case INIT_AINPWRON:
		// Configuro el timer pero no lo prendo
		sTimerCreate(FALSE, aInPwrOn_TimerCallBack , &xTimer_aInPwrOn );
		sTimerChangePeriod(xTimer_aInPwrOn, 5 );
		break;

	case CALLBACK_AINPWRON:
		f_AINPWRONExpired = TRUE;
		break;

	case START_AINPWRON:
		// Arranco el timer que me va a sacar de este modo
		f_AINPWRONExpired = FALSE;
		sTimerChangePeriod(xTimer_aInPwrOn, (u32)(secs) );
		sTimerRestart(xTimer_aInPwrOn);
		break;
	case  EXPIRED_AINPWRON:
		return(f_AINPWRONExpired);
		break;
	}

	return(f_AINPWRONExpired);
}

void aInPwrOn_TimerCallBack ( void )
{
	 ac_aInPwrOnTimer(CALLBACK_AINPWRON, NULL);
}
/*------------------------------------------------------------------------------------*/
static double pvReadADCchannel(u08 ADCchannel, u08 DLGchannel )
{

u16 adcRetValue;

	ADS7828_convert( ADCchannel, &adcRetValue );

	// Debug log ??
	if ( (systemVars.debugLevel & D_DATA) != 0) {
		tickCount = xTaskGetTickCount();
		snprintf_P( aIn_printfBuff,CHAR128,PSTR(".[%06lu] tkAnalogIn::trD05: CH=%d, ADCch=%d, val=%.0f\r\n"), tickCount, DLGchannel, ADCchannel, (double) adcRetValue);
		TERMrprintfStr( aIn_printfBuff );
	}

	return(adcRetValue);
}
/*------------------------------------------------------------------------------------*/
static void pv_AinLoadParameters( void )
{
	// Dependiendo del modo de trabajo normal, service, idle, monitor, setea el
	// timer de poleo.

	switch (systemVars.wrkMode ) {
	case WK_IDLE:
		// Apago el timer del poll.
		ac_aInPollTimer(STOP_AINPOLL, NULL);
		break;
	case WK_NORMAL:
		// EL tiempo de poleo lo marca el systemVars
		ac_aInPollTimer(START_AINPOLL, systemVars.timerPoll );
		break;
	case WK_SERVICE:
		// Dejo de polear
		ac_aInPollTimer(STOP_AINPOLL, NULL);
		break;
	case WK_MONITOR_FRAME:
		// Poleo c/15s.
		ac_aInPollTimer(START_AINPOLL, 15);
		break;
	default:
		// idle, monitorSQE
		// Apago el timer del poll.
		ac_aInPollTimer(STOP_AINPOLL, NULL);
		break;
	}

}
/*------------------------------------------------------------------------------------*/
s16 getTimeToNextPoll(void)
{
s16 retVal = -1;

	// Lo determina en base al time elapsed y el timerPoll.

	if ( systemVars.wrkMode == WK_NORMAL ) {
		retVal = (s16) ( sTimerTimeRemains(xTimer_aInPoll)) ;
	}
	return (retVal);
}
/*------------------------------------------------------------------------------------*/

void getAnalogFrame (frameDataType *dFrame)
{
	memcpy(dFrame, &Aframe, sizeof(Aframe) );
}
/*------------------------------------------------------------------------------------*/

/*
 * sp5K_tkGprs2.c
 *
 *  Created on: 21/05/2014
 *      Author: root
 *
 *  Creo una maquina de estados con 4 estados. c/u es una nueva maquina de estados.
 *  Los eventos los evaluo de acuerdo al estado.
 *
 *  V2.0.10 @ 2014-11-28
 *  Monitoreo del SQE.
 *  - Agrego un nuevo valor al wrkMode, MONITOR.
 *    Al pasar a este modo, tkData es lo mismo que service, pero tkGprs NO.
 *
 *
 */
#include <sp5KV3.h>

char gprs_printfBuff[CHAR256];

static s08 pvGPRS_RspIs(const char *rsp);
static void pvGPRS_sendCmd (char *s, const int clearbuff , const int partialFrame );

s08 fl_tkGprsReloadConfigFlag;
s08 fl_gprsAllowsSleep = FALSE;

// Estados
typedef enum {// gST_INIT = 0,
				gST_OFF = 1,
				gST_ONoffline = 2,
				gST_ONInit = 3,
				gST_ONData = 4
} t_tkGprs_state;

// Subestados.
typedef enum {
				// Estado OFF
				gSST_OFF_Entry = 0,
	            gSST_OFF_Init = 1,
	            gSST_OFF_Idle = 2,
	            gSST_OFF_Standby = 3,
				gSST_OFF_PwrSave = 4,
	            gSST_OFF_await01 = 5,
	            gSST_OFF_await02 = 6,
	            gSST_OFF_await03 = 7,
	            gSST_OFF_dummy01 = 8,
	            gSST_OFF_dummy02 = 9,
	            gSST_OFF_dummy03 = 10,
				gSST_OFF_await04 = 11,

	            // Estado ONoffline
	            gSST_ONoffline_Entry = 12,
	            gSST_ONoffline_cpin = 13,
	            gSST_ONoffline_await01 = 14,
	            gSST_ONoffline_dummy01 = 15,
	            gSST_ONoffline_net = 16,
	            gSST_ONoffline_dummy02 = 17,
	            gSST_ONoffline_dummy03 = 18,
	            gSST_ONoffline_sqe01 = 19,
	            gSST_ONoffline_sqe02 = 20,
	            gSST_ONoffline_sqe03 = 21,
	            gSST_ONoffline_sqe04 = 22,
	            gSST_ONoffline_apn = 23,
	            gSST_ONoffline_ip = 24,
	            gSST_ONoffline_dummy04 = 25,
	            gSST_ONoffline_dummy05 = 26,
	            gSST_ONoffline_dummy06 = 27,

	            // Estado ONInit
	            gSST_ONinit_Entry = 28,
	            gSST_ONinit_socket = 29,
	            gSST_ONinit_dummyS01 = 30,
	            gSST_ONinit_dummyS02 = 31,
	            gSST_ONinit_dummyS03 = 32,
	            gSST_ONinit_initFrame = 33,
	            gSST_ONinit_dummyF01 = 34,
	            gSST_ONinit_dummyF02 = 35,
	            gSST_ONinit_dummyF03 = 36,
	            gSST_ONinit_dummyF04 = 37,
	            gSST_ONinit_dummyF05 = 38,
	            gSST_ONinit_await01 = 39,

	            // Estado ONData
	            gSST_ONdata_Entry = 40,
	            gSST_ONdata_data = 41,
	            gSST_ONdata_dummy01 = 42,
	            gSST_ONdata_dummy02 = 43,
	            gSST_ONdata_await01 = 44,
	            gSST_ONdata_socket = 45,
	            gSST_ONdata_dummyS01 = 46,
	            gSST_ONdata_dummyS02 = 47,
	            gSST_ONdata_dummyS03 = 48,
	            gSST_ONdata_frame = 49,
	            gSST_ONdata_dummyF01 = 50,
	            gSST_ONdata_dummyF02 = 51,
	            gSST_ONdata_dummyF03 = 52,
	            gSST_ONdata_dummyF04 = 53,
	            gSST_ONdata_await02 = 54

} t_tkGprs_subState;

// Eventos
typedef enum {

	gEV00 = 0, 	// Init
	gEV01 = 1,	// ReloadConfig
	gEV02 = 2,	// (wrkMode == NORMAL) || (wrkMode == MONITOR* )
	gEV03 = 3,	// timer1 expired
	gEV04 = 4,	// timerDial expired
	gEV05 = 5,	// GPRS ATrsp == OK
	gEV06 = 6,	// SWtryes_NOT_0
	gEV07 = 7,	// HWtryes_NOT_0

	gEV08 = 8,	// GPRS CPINrsp == READY
	gEV09 = 9,	// GPRS CREGrsp == +CREG 0,1
	gEV10 = 10, // GPRS E2IPArsp == OK
	gEV11 = 11, // pTryes_NOT_0
	gEV12 = 12, // tryes_NOT_0
	gEV13 = 13, // (wrkMode == MONITOR)

	gEV14 = 14, // socket_IS_OPEN
	gEV15 = 15, // SKTryes_NOT_0
	gEV16 = 16, // Server INITrsp_OK

	gEV17 = 17, // BDempty ? (  MEMisEmpty4Del() == 0 )
	gEV18 = 18, // pwrMode == CONT
	gEV19 = 19,	// TxWinTryes ==  MAX_TXWINTRYES
	gEV20 = 20,	// serverResponse == RX_OK
	gEV21 = 21,	// f_sendTail == TRUE

	gEV22 = 22,	// inPwrSave == TRUE

} t_tkGprs_eventos;

#define gEVENT_COUNT		23

s08 gEventos[gEVENT_COUNT];

typedef enum { RSP_NONE = 0,
	             RSP_OK = 1,
	             RSP_READY = 2,
	             RSP_CREG = 3,
	             RSP_APN = 4,
	             RSP_CONNECT = 5,
	             RSP_HTML = 6,
	             RSP_INIT = 7,
	             RSP_RXOK = 8,
	             RSP_IPOK = 9

} t_tkGprs_responses;

t_tkGprs_responses GPRSrsp;

// Acciones Generales
static int gacRELOAD_CONFIG_fn(const char *s);
static void GPRS_getNextEvent(u08 state);
static void pvGPRS_printExitMsg(u08 code);
static void pvGPRS_openSocket(void);
static void pvGPRS_closeSocket(void);
static void pvGPRS_sendInitFrame(void);
static s08 pvGPRS_qySocketIsOpen(void);
static void pvGPRS_processServerClock(void);
static u08 pvGPRS_processPwrMode(void);
static u08 pvGPRS_processTimerPoll(void);
static u08 pvGPRS_processPwrSave(void);
static u08 pvGPRS_processTimerDial(void);
static u08 pvGPRS_processAnalogChannels(u08 channel);
static u08 pvGPRS_processDigitalChannels(u08 channel);
static u08 pvGPRS_processOuputs(void);
static s08 pvGPRS_analizeRXframe(void);

// Estado OFF
static void SM_off(void);

static int gac00_fn(void);
static int gac01_fn(void);
static int gac02_fn(void);
static int gac03_fn(void);
static int gac04_fn(void);
static int gac05_fn(void);
static int gac06_fn(void);
static int gac07_fn(void);
static int gac08_fn(void);
static int gac09_fn(void);
static int gac10_fn(void);
static int gac11_fn(void);
static int gac12_fn(void);
static int gac13_fn(void);
static int gac14_fn(void);
static int gac15_fn(void);
static int gac16_fn(void);

// Estado ONoffline
static void SM_onOffline(void);

static int gac20_fn(void);
static int gac21_fn(void);
static int gac22_fn(void);
static int gac23_fn(void);
static int gac24_fn(void);
static int gac25_fn(void);
static int gac26_fn(void);
static int gac27_fn(void);
static int gac28_fn(void);
static int gac29_fn(void);
static int gac30_fn(void);
static int gac31_fn(void);
static int gac32_fn(void);
static int gac33_fn(void);
static int gac34_fn(void);
static int gac35_fn(void);
static int gac36_fn(void);
static int gac37_fn(void);
static int gac38_fn(void);
static int gac39_fn(void);
static int gac40_fn(void);
static int gac41_fn(void);
static int gac42_fn(void);
static int gac43_fn(void);
static int gac44_fn(void);

// Estado ONInit
static void SM_onInit(void);

static int gac50_fn(void);
static int gac51_fn(void);
static int gac52_fn(void);
static int gac53_fn(void);
static int gac54_fn(void);
static int gac55_fn(void);
static int gac56_fn(void);
static int gac57_fn(void);
static int gac58_fn(void);
static int gac59_fn(void);
static int gac60_fn(void);
static int gac61_fn(void);
static int gac62_fn(void);
static int gac63_fn(void);
static int gac64_fn(void);
static int gac65_fn(void);
static int gac66_fn(void);
static int gac67_fn(void);
static int gac68_fn(void);
static int gac69_fn(void);

// Estado ONData
static void SM_onData(void);

static int gac70_fn(void);
static int gac71_fn(void);
static int gac72_fn(void);
static int gac73_fn(void);
static int gac74_fn(void);
static int gac75_fn(void);
static int gac76_fn(void);
static int gac77_fn(void);
static int gac78_fn(void);
static int gac79_fn(void);
static int gac80_fn(void);
static int gac81_fn(void);
static int gac82_fn(void);
static int gac83_fn(void);
static int gac84_fn(void);
static int gac85_fn(void);
static int gac86_fn(void);
static int gac87_fn(void);
static int gac88_fn(void);
static int gac89_fn(void);
static int gac90_fn(void);
static int gac91_fn(void);
static int gac92_fn(void);
static int gac93_fn(void);
static int gac94_fn(void);
static int gac95_fn(void);

u08 tkGprs_state, tkGprs_subState;
static u32 tickCount;					// para usar en los mensajes del debug.
static s08 arranqueFlag;
static s08 fl_pwrSave;

//-------------------------------------------------------------------------------------
// GPRS_TIMER_GENERAL01
typedef enum { CALLBACK_GPRST1, INIT_GPRST1, START_GPRST1, STOP_GPRST1, EXPIRED_GPRST1 } t_GprsTimer1;
u08 xTimer_gprsT1;
static s08 ac_gprsTimer1(t_GprsTimer1 action, u16 secs, s08 *retStatus);
void gprsT1_TimerCallBack (void );

// GPRS_TIMER_DIAL
typedef enum { CALLBACK_GPRSTd, INIT_GPRSTd, START_GPRSTd, STOP_GPRSTd, EXPIRED_GPRSTd } t_GprsTimerDial;
u08 xTimer_gprsTDial;
static s08 ac_gprsTimerDial(t_GprsTimerDial action, u32 secs);
void gprsTDial_TimerCallBack (void );

//-------------------------------------------------------------------------------------

#define MAX_HWTRYES		3
#define MAX_SWTRYES		3
static u08 HWtryes;
static u08 SWtryes;

#define MAX_TRYES		3
#define MAX_PTRYES		3
static u08 tryes;
static u08 pTryes;

static u08 sockTryes;
#define MAX_SOCKTRYES		3

// Contador de cuantas veces intente mandar la misma ventana de datos.
static u08 TxWinTryes;
#define MAX_TXWINTRYES		4

// Las ventanas de datos pueden no ser completas y con esta flag lo indico.
static s08 f_sendTail;

// Contador con los frames enviados al server aun sin ACK.
static u08 nroFrameInTxWindow;
#define MAX_TXFRM4WINDOW	12

static frameDataType rdFrame;

#define MAXTIMEINASTATE 1800 // 3 minutos ( de a 100ms)
static u32 counterCtlState;

u08 modemStatus;

//------------------------------------------------------------------------------------
void tkGprs(void * pvParameters)
{

( void ) pvParameters;
BaseType_t xResult;
uint32_t ulNotifiedValue;

	vTaskDelay( ( TickType_t)( 1000 / portTICK_RATE_MS ) );

	// Espero la notificacion para arrancar
	while ( startToken != STOK_GPRS ) {
		vTaskDelay( ( TickType_t)( 100 / portTICK_RATE_MS ) );
	}
	TERMrprintfProgStrM("starting tkGprs..\r\n");
	startToken++;

	// Creo los timers
	ac_gprsTimer1( INIT_GPRST1, (u16) NULL, (s08) NULL );
	ac_gprsTimerDial( INIT_GPRSTd, (u16) NULL );

	tkGprs_state = gST_OFF;
	tkGprs_subState = gSST_OFF_Entry;
	arranqueFlag = TRUE;
	//
	for( ;; )
	{
		clearWdg(WDG_GPRS);

		// Espero hasta 100ms por un mensaje.
		xResult = xTaskNotifyWait( 0x00, ULONG_MAX, &ulNotifiedValue, ((TickType_t) 100 / portTICK_RATE_MS ) );
		// Si llego un mensaje, prendo la flag correspondiente.
		if ( xResult == pdTRUE ) {
			if ( ( ulNotifiedValue & GPRSMSG_RELOAD ) != 0 ) {
				fl_tkGprsReloadConfigFlag = TRUE;
			}
		}

		// Analizo los eventos.
		GPRS_getNextEvent(tkGprs_state);

		// Control del tiempo que esta activo c/estado.
		counterCtlState--;
		if ( counterCtlState == 0 ) {
			snprintf_P( gprs_printfBuff,CHAR256,PSTR("tkGprs::ERROR exceso de tiempo en estado [%d/%d]\r\n\0"),tkGprs_state,tkGprs_subState);
			TERMrprintfStr( gprs_printfBuff );

			tkGprs_state = gST_OFF;
			tkGprs_subState = gSST_OFF_Entry;
			arranqueFlag = TRUE;
		}


		// El manejar la FSM con un switch por estado y no por transicion me permite
		// priorizar las transiciones.
		// Luego de c/transicion debe venir un break así solo evaluo de a 1 transicion por loop.
		//
		switch ( tkGprs_state ) {
		case gST_OFF:
			SM_off();
			break;
		case gST_ONoffline:
			SM_onOffline();
			break;
		case gST_ONInit:
			SM_onInit();
			break;
		case gST_ONData:
			SM_onData();
			break;
		default:
			TERMrprintfProgStrM("tkGprs::ERROR state NOT DEFINED\r\n\0");
			tkGprs_state = gST_OFF;
			tkGprs_subState = gSST_OFF_Entry;
			break;
		}
	}
}
//------------------------------------------------------------------------------------
static void GPRS_getNextEvent(u08 state)
{
// Evaluo todas las condiciones que generan los eventos que disparan las transiciones.
// Tenemos un array de eventos y todos se evaluan.
// Las condiciones que evaluo dependen del estado ya que no todas se deben dar siempre


u08 i;

	// Inicializo la lista de eventos.
	for ( i=0; i < gEVENT_COUNT; i++ ) {
		gEventos[i] = FALSE;
	}

	switch (state) {
	case gST_OFF:
		// Evaluo solo los eventos del estado OFF.
		// EV01: ReloadConfig
		if ( fl_tkGprsReloadConfigFlag == TRUE ) { gEventos[gEV01] = TRUE; }
		// EV02: (wrkMode == NORMAL) || (wrkMode == MONITOR )
		if ( ( systemVars.wrkMode == WK_NORMAL ) || ( systemVars.wrkMode == WK_MONITOR_SQE ) ) { gEventos[gEV02] = TRUE; }
		// EV03: timer1 expired
		if ( ac_gprsTimer1(EXPIRED_GPRST1, (u16) NULL, (s08) NULL) ) { gEventos[gEV03] = TRUE; }
		// EV04: timer dial expired
		if ( ac_gprsTimerDial(EXPIRED_GPRSTd, (u16) NULL) ) { gEventos[gEV04] = TRUE; }
		// EV05: GPRS ATrsp == OK
		if ( GPRSrsp == RSP_OK ) { gEventos[gEV05] = TRUE; }
		// EV06: SWtryes_NOT_0()
		if ( SWtryes > 0 ) { gEventos[gEV06] = TRUE; }
		// EV07: HWtryes_NOT_0()
		if ( HWtryes > 0 ) { gEventos[gEV07] = TRUE; }
		// EV22: InPwrSaveInterval()
		if ( fl_pwrSave ) { gEventos[gEV22] = TRUE; }
		break;

	case gST_ONoffline:
		// Evaluo solo los eventos del estado ONBoffline
		// EV01: ReloadConfig
		if ( fl_tkGprsReloadConfigFlag == TRUE ) { gEventos[gEV01] = TRUE; }
		// EV03: timer1 == 0
		if (ac_gprsTimer1(EXPIRED_GPRST1, (u16) NULL, (s08) NULL ) )  { gEventos[gEV03] = TRUE; }
		// EV08: CPIN ready
		if ( GPRSrsp == RSP_READY ) { gEventos[gEV08] = TRUE; }
		// EV09: CREG (NET available)
		if ( GPRSrsp == RSP_CREG ) { gEventos[gEV09] = TRUE; }
		// EV10: IP_OK.
		if ( GPRSrsp == RSP_IPOK ) { gEventos[gEV10] = TRUE; }
		// EV11: pTryes NOT 0
		if ( pTryes > 0 ) { gEventos[gEV11] = TRUE; }
		// EV12: tryes NOT 0
		if ( tryes > 0 ) { gEventos[gEV12] = TRUE; }
		// EV13: (wrkMode == MONITOR)
		if ( systemVars.wrkMode == WK_MONITOR_SQE  ) { gEventos[gEV13] = TRUE; }
		break;

	case gST_ONInit:
		// Estado ON_INITFRAME
		// EV01: ReloadConfig
		if ( fl_tkGprsReloadConfigFlag == TRUE ) { gEventos[gEV01] = TRUE; }
		// EV03: timer1 == 0
		if ( ac_gprsTimer1(EXPIRED_GPRST1, (u16) NULL, (s08) NULL )) { gEventos[gEV03] = TRUE; }
		// EV11: pTryes NOT 0
		if ( pTryes > 0 ) { gEventos[gEV11] = TRUE; }
		// EV14: socket == OPEN
		if ( pvGPRS_qySocketIsOpen() ) { gEventos[gEV14] = TRUE; }
		// EV15: socketTryes NOT 0
		if ( sockTryes > 0 ) { gEventos[gEV15] = TRUE; }
		// EV16: serverResponse == INIT_OK
		if ( GPRSrsp == RSP_INIT ) { gEventos[gEV16] = TRUE; }
		break;

	case gST_ONData:
		// Estado ONData
		// EV01: ReloadConfig
		if ( fl_tkGprsReloadConfigFlag == TRUE ) { gEventos[gEV01] = TRUE; }
		// EV03: timer1 == 0
		if ( ac_gprsTimer1(EXPIRED_GPRST1, (u16) NULL, (s08) NULL )) { gEventos[gEV03] = TRUE; }
		// EV11: pTryes NOT 0
		if ( pTryes > 0 ) { gEventos[gEV11] = TRUE; }
		// EV14: socket == OPEN
		if ( pvGPRS_qySocketIsOpen() ) { gEventos[gEV14] = TRUE; }
		// EV15: socketTryes NOT 0
		if ( sockTryes > 0 ) { gEventos[gEV15] = TRUE; }
		// EV17: BD empty ?
		if (  MEMisEmpty4Del() ) { gEventos[gEV17] = TRUE; }
		// EV18: pwrMode == CONT
		if ( systemVars.pwrMode == PWR_CONTINUO ) { gEventos[gEV18] = TRUE; }
		// EV19: TxWinTryes ==  MAX_TXWINTRYES
		if ( TxWinTryes ==  MAX_TXWINTRYES ) { gEventos[gEV19] = TRUE; }
		// EV20: serverResponse == RX_OK
		if ( GPRSrsp == RSP_RXOK ) { gEventos[gEV20] = TRUE; }
		// EV21: f_sendTail == TRUE
		if ( f_sendTail == TRUE ) { gEventos[gEV21] = TRUE; }
		break;
	}
}
//------------------------------------------------------------------------------------
static void SM_off(void)
{
	// Maquina de estados del estado OFF.( MODEM APAGADO)

	switch ( tkGprs_subState ) {
	case gSST_OFF_Entry:
		tkGprs_subState = gac00_fn();	// gTR00
		break;
	case gSST_OFF_Init:
		if ( gEventos[gEV01] ) {	tkGprs_subState = gacRELOAD_CONFIG_fn("gSST_OFF_Init"); break; }
		if ( gEventos[gEV02]  ) {
			tkGprs_subState = gac03_fn(); // gTR03
		} else {
			tkGprs_subState = gac01_fn(); // gTR01
		}
		break;
	case gSST_OFF_Idle:
		if ( gEventos[gEV01] ) {	tkGprs_subState = gacRELOAD_CONFIG_fn("gSST_OFF_Idle"); break; }
		if ( gEventos[gEV03] ) {	tkGprs_subState = gac02_fn(); break; }	// gTR02
		break;
	case gSST_OFF_Standby:
		if ( gEventos[gEV01] ) {	tkGprs_subState = gacRELOAD_CONFIG_fn("gSST_OFF_Standby"); break; }
		if ( gEventos[gEV04] ) {	tkGprs_subState = gac04_fn(); break; }	// gTR04
		break;
	case gSST_OFF_PwrSave:
		if ( gEventos[gEV01] ) {	tkGprs_subState = gacRELOAD_CONFIG_fn("gSST_OFF_pwrSave"); break; }
		if ( gEventos[gEV22] ) {
			tkGprs_subState = gac16_fn();	// gTR16
		} else {
			tkGprs_subState = gac15_fn();	// gTR15
		}
		break;
	case gSST_OFF_await01:
		if ( gEventos[gEV01] ) {	tkGprs_subState = gacRELOAD_CONFIG_fn("gSST_OFF_await01"); break; }
		if ( gEventos[gEV03] ) {	tkGprs_subState = gac05_fn(); break; }	// gTR05
		break;
	case gSST_OFF_await02:
		if ( gEventos[gEV01]  ) {	tkGprs_subState = gacRELOAD_CONFIG_fn("gSST_OFF_await02"); break; }
		if ( gEventos[gEV03]  ) {	tkGprs_subState = gac06_fn(); break; }	// gTR06
		break;
	case gSST_OFF_await03:
		if ( gEventos[gEV01] ) {	tkGprs_subState = gacRELOAD_CONFIG_fn("gSST_OFF_await03"); break; }
		if ( gEventos[gEV03] ) {	tkGprs_subState = gac07_fn(); break; }	// gTR07
		break;
	case gSST_OFF_dummy01:
		if ( gEventos[gEV01] ) {	tkGprs_subState = gacRELOAD_CONFIG_fn("gSST_OFF_dummy01"); break; }
		if ( gEventos[gEV05] ) {
			tkGprs_subState = gac08_fn(); 	// gTR08
		} else {
			tkGprs_subState = gac09_fn();	// gTR09
		}
		break;
	case gSST_OFF_dummy02:
		if ( gEventos[gEV01] ) {	tkGprs_subState = gacRELOAD_CONFIG_fn("gSST_OFF_dummy02"); break; }
		if ( gEventos[gEV06] ) {
			tkGprs_subState = gac10_fn(); // gTR10
		} else {
			tkGprs_subState = gac11_fn(); // gTR11
		}
		break;
	case gSST_OFF_dummy03:
		if ( gEventos[gEV01] ) {	tkGprs_subState = gacRELOAD_CONFIG_fn("gSST_OFF_dummy03"); break; }
		if ( gEventos[gEV07] ) {
			tkGprs_subState = gac12_fn();// gTR12
		} else {
			tkGprs_subState = gac13_fn();// gTR13
		}
		break;
	case gSST_OFF_await04:
		if ( gEventos[gEV01] ) {	tkGprs_subState = gacRELOAD_CONFIG_fn("gSST_OFF_await04"); break; }
		if ( gEventos[gEV03] ) {	tkGprs_subState = gac14_fn(); break; }	// gTR14
		break;

	default:
		snprintf_P( gprs_printfBuff,CHAR256,PSTR("tkGprs::ERROR sst_off: subState  (%d) NOT DEFINED\r\n\0"),tkGprs_subState);
		TERMrprintfStr( gprs_printfBuff );
		tkGprs_state = gST_OFF;
		tkGprs_subState = gSST_OFF_Entry;
		break;
	}
}
//------------------------------------------------------------------------------------
static void SM_onOffline(void)
{
	// Maquina de estados del estado ONoffline

	switch ( tkGprs_subState ) {
	case gSST_ONoffline_Entry:
		tkGprs_subState = gac20_fn();										// gTR20
		break;
	case gSST_ONoffline_cpin:
		if ( gEventos[gEV01] ) {	tkGprs_subState = gacRELOAD_CONFIG_fn("gSST_ONoffline_Cpin"); break; }
		if ( gEventos[gEV08] ) {
			tkGprs_subState = gac21_fn(); // gTR21
		} else {
			tkGprs_subState = gac22_fn(); // gTR22
		}
		break;
	case gSST_ONoffline_await01:
		if ( gEventos[gEV01] ) {	tkGprs_subState = gacRELOAD_CONFIG_fn("gSST_ONoffline_await01"); break; }
		if ( gEventos[gEV03] ) {	tkGprs_subState = gac23_fn(); break; }				// gTR23
		break;
	case gSST_ONoffline_dummy01:
		if ( gEventos[gEV01] ) {	tkGprs_subState = gacRELOAD_CONFIG_fn("gSST_ONoffline_dummy01"); break; }
		if ( gEventos[gEV12] ) {
			tkGprs_subState = gac24_fn(); 	// gTR24
		} else {
			tkGprs_subState = gac25_fn(); 	// gTR25
		}
		break;
	case gSST_ONoffline_net:
		if ( gEventos[gEV01] ) {	tkGprs_subState = gacRELOAD_CONFIG_fn("gSST_ONoffline_net"); break; }
		if ( gEventos[gEV03] ) {	tkGprs_subState = gac26_fn(); break; }				// gTR26
		break;
	case gSST_ONoffline_dummy02:
		if ( gEventos[gEV01]  ) {	tkGprs_subState = gacRELOAD_CONFIG_fn("gSST_ONoffline_dummy02"); break; }
		if ( gEventos[gEV09]  ) {
			tkGprs_subState = gac30_fn();	// gTR30
		} else {
			tkGprs_subState = gac27_fn();	// gTR27
		}
		break;
	case gSST_ONoffline_dummy03:
		if ( gEventos[gEV01]  ) {	tkGprs_subState = gacRELOAD_CONFIG_fn("gSST_ONoffline_dummy03"); break; }
		if ( gEventos[gEV12]  ) {
			tkGprs_subState = gac28_fn(); 	// gTR28
		} else {
			tkGprs_subState = gac29_fn();	// gTR29
		}
		break;
	case gSST_ONoffline_sqe01:
		if ( gEventos[gEV01] ) {	tkGprs_subState = gacRELOAD_CONFIG_fn("gSST_ONoffline_sqe01"); break; }
		if ( gEventos[gEV13] ) {
			tkGprs_subState = gac41_fn();	// gTR41
		} else {
			tkGprs_subState = gac40_fn();	// gTR40
		}
		break;
	case gSST_ONoffline_sqe02:
		if ( gEventos[gEV01] ) {	tkGprs_subState = gacRELOAD_CONFIG_fn("gSST_ONoffline_sqe02"); break; }
		if ( gEventos[gEV03] ) {	tkGprs_subState = gac31_fn(); break; }				// gTR31
		break;
	case gSST_ONoffline_sqe03:
		if ( gEventos[gEV01] ) {	tkGprs_subState = gacRELOAD_CONFIG_fn("gSST_ONoffline_sqe03"); break; }
		if ( gEventos[gEV03] ) {	tkGprs_subState = gac42_fn(); break; }				// gTR42
		break;
	case gSST_ONoffline_sqe04:
		if ( gEventos[gEV01] ) {	tkGprs_subState = gacRELOAD_CONFIG_fn("gSST_ONoffline_sqe04"); break; }
		if ( gEventos[gEV12] ) {
			tkGprs_subState = gac43_fn();	// gTR43
		} else {
			tkGprs_subState = gac44_fn();	// gTR44
		}
		break;
	case gSST_ONoffline_apn:
		if ( gEventos[gEV01] ) {	tkGprs_subState = gacRELOAD_CONFIG_fn("gSST_ONoffline_apn"); break; }
		if ( gEventos[gEV03] ) {	tkGprs_subState = gac32_fn(); break; }				// gTR32
		break;
	case gSST_ONoffline_ip:
		if ( gEventos[gEV01] ) {	tkGprs_subState = gacRELOAD_CONFIG_fn("gSST_ONoffline_ip"); break; }
		if ( gEventos[gEV03] ) {	tkGprs_subState = gac33_fn(); break; }				// gTR33
		break;
	case gSST_ONoffline_dummy04:
		if ( gEventos[gEV01] ) {	tkGprs_subState = gacRELOAD_CONFIG_fn("gSST_ONoffline_dummy04"); break; }
		if ( gEventos[gEV10] ) {
			tkGprs_subState = gac39_fn(); // gTR39
		} else {
			tkGprs_subState = gac34_fn(); // gTR34
		}
		break;
	case gSST_ONoffline_dummy05:
		if ( gEventos[gEV01] ) {	tkGprs_subState = gacRELOAD_CONFIG_fn("gSST_ONoffline_dummy05"); break; }
		if ( gEventos[gEV11] ) {
			tkGprs_subState = gac35_fn();	// gTR35
		} else {
			tkGprs_subState = gac36_fn(); 	// gTR36
		}
		break;
	case gSST_ONoffline_dummy06:
		if ( gEventos[gEV01] ) {	tkGprs_subState = gacRELOAD_CONFIG_fn("gSST_ONoffline_dummy06"); break; }
		if ( gEventos[gEV12] ) {
			tkGprs_subState = gac37_fn(); 	// gTR37
		} else {
			tkGprs_subState = gac38_fn(); 	// gTR38
		}
		break;
	default:
		snprintf_P( gprs_printfBuff,CHAR256,PSTR("tkGprs::ERROR sst_onOffline: subState  (%d) NOT DEFINED\r\n\0"),tkGprs_subState);
		TERMrprintfStr( gprs_printfBuff );
		tkGprs_state = gST_OFF;
		tkGprs_subState = gSST_OFF_Init;
		break;
	}

}
//------------------------------------------------------------------------------------
static void SM_onInit(void)
{

	// Maquina de estados del estado ONinit

	switch ( tkGprs_subState ) {
	case  gSST_ONinit_Entry:
		tkGprs_subState = gac50_fn();													// gTR50
		break;
	case gSST_ONinit_socket:
		if ( gEventos[gEV01] ) {	tkGprs_subState = gacRELOAD_CONFIG_fn("gSST_ONinit_socket"); break; } // gTRacRCF
		if ( gEventos[gEV03] ) {	tkGprs_subState = gac51_fn(); break; }				// gTR51
		break;
	case gSST_ONinit_dummyS01:
		if ( gEventos[gEV01] ) {	tkGprs_subState = gacRELOAD_CONFIG_fn("gSST_ONinit_dummyS01"); break; } // gTRacRCF
		if ( gEventos[gEV14]  ) {
			tkGprs_subState = gac57_fn(); 	// gTR57
		} else {
			tkGprs_subState = gac52_fn(); 	// gTR52
		}
		break;
	case gSST_ONinit_dummyS02:
		if ( gEventos[gEV01] ) {	tkGprs_subState = gacRELOAD_CONFIG_fn("gSST_ONinit_dummyS02"); break; } // gTRacRCF
		if ( gEventos[gEV11] ) {
			tkGprs_subState = gac53_fn(); 	// gTR53
		} else {
			tkGprs_subState = gac54_fn();	// gTR54
		}
		break;
	case gSST_ONinit_dummyS03:
		if ( gEventos[gEV01] ) {	tkGprs_subState = gacRELOAD_CONFIG_fn("gSST_ONinit_dummyS03"); break; } // gTRacRCF
		if ( gEventos[gEV15] ) {
			tkGprs_subState = gac55_fn();	// gTR55
		} else {
			tkGprs_subState = gac56_fn(); 	// gTR56
		}
		break;
	case gSST_ONinit_initFrame:
		if ( gEventos[gEV01] ) {	tkGprs_subState = gacRELOAD_CONFIG_fn("gSST_ONinit_initFrame"); break; } // gTRacRCF
		if ( gEventos[gEV03] ) {	tkGprs_subState = gac58_fn(); break; }				// gTR58
		break;
	case gSST_ONinit_dummyF01:
		if ( gEventos[gEV01] ) {	tkGprs_subState = gacRELOAD_CONFIG_fn("gSST_ONinit_dummyF01"); break; } // gTRacRCF
		if ( gEventos[gEV16] ) {
			tkGprs_subState = gac59_fn(); 	// gTR59
		} else {
			tkGprs_subState = gac60_fn();	// gTR60
		}
		break;
	case gSST_ONinit_dummyF02:
		if ( gEventos[gEV01] ) {	tkGprs_subState = gacRELOAD_CONFIG_fn("gSST_ONinit_dummyF02"); break; } // gTRacRCF
		if ( gEventos[gEV14] ) {
			tkGprs_subState = gac61_fn(); 	// gTR61
		} else {
			tkGprs_subState = gac66_fn(); 	// gTR66
		}
		break;
	case gSST_ONinit_dummyF03:
		if ( gEventos[gEV01] ) {	tkGprs_subState = gacRELOAD_CONFIG_fn("gSST_ONinit_dummyF03"); break; } // gTRacRCF
		if ( gEventos[gEV11] ) {
			tkGprs_subState = gac62_fn(); 	// gTR62
		} else {
			tkGprs_subState = gac63_fn(); 	// gTR63
		}
		break;
	case gSST_ONinit_dummyF04:
		if ( gEventos[gEV01] ) {	tkGprs_subState = gacRELOAD_CONFIG_fn("gSST_ONinit_dummyF04"); break; } // gTRacRCF
		if ( gEventos[gEV15] ) {
			tkGprs_subState = gac64_fn(); 	// gTR64
		} else {
			tkGprs_subState = gac65_fn(); 	// gTR65
		}
		break;
	case gSST_ONinit_dummyF05:
		if ( gEventos[gEV01] ) {	tkGprs_subState = gacRELOAD_CONFIG_fn("gSST_ONinit_dummyF05"); break; } // gTRacRCF
		if ( gEventos[gEV15] ) {
			tkGprs_subState = gac67_fn();	// gTR67
		} else {
			tkGprs_subState = gac68_fn(); 	// gTR68
		}
		break;
	case gSST_ONinit_await01:
		if ( gEventos[gEV01] ) {	tkGprs_subState = gacRELOAD_CONFIG_fn("gSST_ONinit_await01"); break; } // gTRacRCF
		if ( gEventos[gEV03] ) {	tkGprs_subState = gac69_fn(); break; }				// gTR69
		if ( ! gEventos[gEV14] ) {	tkGprs_subState = gac69_fn(); break; }				// gTR69
		break;
	default:
		snprintf_P( gprs_printfBuff,CHAR256,PSTR("tkGprs::ERROR sst_onInit: subState  (%d) NOT DEFINED\r\n\0"),tkGprs_subState);
		TERMrprintfStr( gprs_printfBuff );
		tkGprs_state = gST_OFF;
		tkGprs_subState = gSST_OFF_Init;
		break;
	}
}
//------------------------------------------------------------------------------------
static void SM_onData(void)
{
	// Maquina de estados del estado ONData

	switch ( tkGprs_subState ) {
	case  gSST_ONdata_Entry:
		tkGprs_subState = gac70_fn();													// gTR70
		break;
	case gSST_ONdata_data:
		if ( gEventos[gEV01] ) {	tkGprs_subState = gacRELOAD_CONFIG_fn("gSST_ONdata_data"); break; } // gTRacRCF
		if ( gEventos[gEV17] ) {
			tkGprs_subState = gac72_fn(); 	// gTR72
		} else {
			tkGprs_subState = gac71_fn(); 	// gTR71
		}
		break;
	case gSST_ONdata_dummy01:
		if ( gEventos[gEV01] ) {	tkGprs_subState = gacRELOAD_CONFIG_fn("gSST_ONdata_dummy01"); break; } // gTRacRCF
		if ( gEventos[gEV18] ) {
			tkGprs_subState = gac73_fn(); 	// gTR73
		} else {
			tkGprs_subState = gac74_fn(); 	// gTR74
		}
		break;
	case gSST_ONdata_await01:
		if ( gEventos[gEV01] ) {	tkGprs_subState = gacRELOAD_CONFIG_fn("gSST_ONdata_await01"); break; } // gTRacRCF
		if ( gEventos[gEV03] ) {	tkGprs_subState = gac75_fn(); break; }				// gTR75
		break;
	case gSST_ONdata_dummy02:
		if ( gEventos[gEV01] ) {	tkGprs_subState = gacRELOAD_CONFIG_fn("gSST_ONdata_dummy02"); break; } // gTRacRCF
		if ( gEventos[gEV19] ) {	tkGprs_subState = gac95_fn(); break; }				// gTR95
		if ( gEventos[gEV14] ) {
			tkGprs_subState = gac84_fn(); 	// gTR84
		} else {
			tkGprs_subState = gac76_fn();   // gTR76
		}
		break;
	case gSST_ONdata_socket:
		if ( gEventos[gEV01] ) {	tkGprs_subState = gacRELOAD_CONFIG_fn("gSST_ONdata_socket"); break; } // gTRacRCF
		if ( gEventos[gEV03] ) {	tkGprs_subState = gac77_fn(); break; }				// gTR77
		break;
	case gSST_ONdata_dummyS01:
		if ( gEventos[gEV01] ) {	tkGprs_subState = gacRELOAD_CONFIG_fn("gSST_ONdata_dummyS01"); break; } // gTRacRCF
		if ( gEventos[gEV14] ) {
			tkGprs_subState = gac79_fn(); 	// gTR79
		} else {
			tkGprs_subState = gac78_fn(); 	// gTR78
		}
		break;
	case gSST_ONdata_dummyS02:
		if ( gEventos[gEV01] ) {	tkGprs_subState = gacRELOAD_CONFIG_fn("gSST_ONdata_dummyS02"); break; } // gTRacRCF
		if ( gEventos[gEV11] ) {
			tkGprs_subState = gac80_fn();	// gTR80
		} else {
			tkGprs_subState = gac81_fn();  	// gTR81
		}
		break;
	case gSST_ONdata_dummyS03:
		if ( gEventos[gEV01] ) {	tkGprs_subState = gacRELOAD_CONFIG_fn("gSST_ONdata_dummyS03"); break; } // gTRacRCF
		if ( gEventos[gEV15] ) {
			tkGprs_subState = gac82_fn(); 	// gTR82
		} else {
			tkGprs_subState = gac83_fn();  	// gTR83
		}
		break;
	case gSST_ONdata_frame:
		if ( gEventos[gEV01] ) {	tkGprs_subState = gacRELOAD_CONFIG_fn("gSST_ONdata_frame"); break; } // gTRacRCF
		if ( gEventos[gEV21] ) {
			tkGprs_subState = gac85_fn(); 	// gTR85
		} else {
			tkGprs_subState = gac86_fn();	// gTR86
		}
		break;
	case gSST_ONdata_dummyF01:
		if ( gEventos[gEV01] ) {	tkGprs_subState = gacRELOAD_CONFIG_fn("gSST_ONdata_dummyF01"); break; } // gTRacRCF
		if ( gEventos[gEV03] ) {	tkGprs_subState = gac87_fn(); break; }				// gTR87
		break;
	case gSST_ONdata_dummyF02:
		if ( gEventos[gEV01] ) {	tkGprs_subState = gacRELOAD_CONFIG_fn("gSST_ONdata_dummyF02"); break; } // gTRacRCF
		if ( gEventos[gEV20] ) {
			tkGprs_subState = gac88_fn();  	// gTR88
		} else {
			tkGprs_subState = gac89_fn();  	// gTR89
		}
		break;
	case gSST_ONdata_dummyF03:
		if ( gEventos[gEV01] ) {	tkGprs_subState = gacRELOAD_CONFIG_fn("gSST_ONdata_dummyF03"); break; } // gTRacRCF
		if ( gEventos[gEV14] ) {
			tkGprs_subState = gac90_fn();	// gTR90
		} else {
			tkGprs_subState = gac91_fn();  	// gTR91
		}
		break;
	case gSST_ONdata_dummyF04:
		if ( gEventos[gEV01] ) {	tkGprs_subState = gacRELOAD_CONFIG_fn("gSST_ONdata_dummyF04"); break; } // gTRacRCF
		if ( gEventos[gEV11] ) {
			tkGprs_subState = gac92_fn();	// gTR92
		} else {
			tkGprs_subState = gac93_fn();  // gTR93
		}
		break;
	case gSST_ONdata_await02:
		if ( gEventos[gEV01] ) {	tkGprs_subState = gacRELOAD_CONFIG_fn("gSST_ONdata_await02"); break; } // gTRacRCF
		if ( gEventos[gEV03] ) {	tkGprs_subState = gac94_fn(); break; }				// gTR94
		if ( ! gEventos[gEV14] ) {	tkGprs_subState = gac94_fn(); break; }				// gTR94
		break;

	default:
		snprintf_P( gprs_printfBuff,CHAR256,PSTR("tkGprs::ERROR sst_onData: subState  (%d) NOT DEFINED\r\n\0"),tkGprs_subState);
		TERMrprintfStr( gprs_printfBuff );
		tkGprs_state = gST_OFF;
		tkGprs_subState = gSST_OFF_Init;
		break;	}

}
/*------------------------------------------------------------------------------------*/
static int gacRELOAD_CONFIG_fn(const char *s)
{

	snprintf_P( gprs_printfBuff,CHAR256,PSTR("tkGprs::RCONF:[%s]\r\n\0"),s);
	TERMrprintfStr( gprs_printfBuff );

	// Al reconfigurarme debo rearrancar lo antes posible.
	arranqueFlag = TRUE;

	pvGPRS_printExitMsg(100);
	tkGprs_state = gST_OFF;
	return(gSST_OFF_Entry);

}
//------------------------------------------------------------------------------------
/*
 *  FUNCIONES DEL ESTADO OFF:
 *  El modem esta apagado ya sea en standby o idle, y sale hasta quedar prendido
 *  Utilizamos timer1.
 *  Timer1 es general.
 *
 */
//------------------------------------------------------------------------------------
static int gac00_fn(void)
{

	// Evento inicial. Solo salta al primer estado operativo.
	// Inicializo el sistema aqui
	// gST_INIT -> gST_Off_Init

	strncpy_P(systemVars.dlgIp, PSTR("000.000.000.000\0"),16);
	systemVars.csq = 0;
	systemVars.dbm = 0;
	fl_tkGprsReloadConfigFlag = FALSE;
	modemStatus = M_OFF;
	fl_gprsAllowsSleep = TRUE;
	fl_pwrSave = FALSE;

	ac_gprsTimer1( STOP_GPRST1, (u16) NULL , (s08) NULL );
	ac_gprsTimerDial( STOP_GPRSTd, (u16) NULL );

	pvGPRS_printExitMsg(0);
	return(gSST_OFF_Init);
}
//------------------------------------------------------------------------------------
static int gac01_fn(void)
{
	// gST_Off_Init -> gST_Off_Idle

s08 retS;

	// Apago el modem
	retS = MCP_setGprsPwr(0);
	// Activo el pwr del modem para que no consuma
	vTaskDelay( (portTickType)( 500 / portTICK_RATE_MS ) );
	retS = MCP_setGprsPwr(1);

	modemStatus = M_OFF_IDLE;
	fl_gprsAllowsSleep = TRUE;

	pvGPRS_printExitMsg(1);
	return(gSST_OFF_Idle);
}
//------------------------------------------------------------------------------------
static int gac02_fn(void)
{

	// gST_Off_Idle -> gST_Off_Init
	strncpy_P(systemVars.dlgIp, PSTR("000.000.000.000\0"),16);
	systemVars.csq = 0;
	systemVars.dbm = 0;

	modemStatus = M_OFF;
	fl_gprsAllowsSleep = TRUE;

	pvGPRS_printExitMsg(2);
	return(gSST_OFF_Init);
}
//------------------------------------------------------------------------------------
static int gac03_fn(void)
{
	// gST_Off_Init -> gST_Off_Standby
	// Me preparo para generar la espera de timerDial.
s08 retS;
u32 timerVal;
RtcTimeType rtcDateTime;
u16 now;

	tickCount = xTaskGetTickCount();
	// Hora actual en minutos.
	rtcGetTime(&rtcDateTime);
	now = rtcDateTime.hour * 60 + rtcDateTime.min;

	switch (systemVars.pwrMode ) {
		case PWR_CONTINUO:
			timerVal = 90;
			break;
		case PWR_DISCRETO:
			timerVal = systemVars.timerDial;
			break;
		default:
			timerVal = 300;
			TERMrprintfProgStrM("tkGprs::gTR03::ERROR pwrMode:\r\n\0");
			break;
	}
	snprintf_P( gprs_printfBuff,CHAR256,PSTR(".[%06lu] tkGprs::gTR03 Modem off(await=%lu):\r\n\0"), tickCount, sTimerTimeRemains(xTimer_gprsT1));

	// Al arranque disco a los 15s sin importar el modo.
	if ( arranqueFlag == TRUE) {
		arranqueFlag = FALSE;
		timerVal = 15;
	}

	// Si estoy en modo MONITOR ( SQE / FRAME ) no espero nada.
	if ( (systemVars.wrkMode == WK_MONITOR_FRAME) || (systemVars.wrkMode == WK_MONITOR_SQE )) {
		timerVal = 5;
	}

	// Arranco el timer a esperar
	ac_gprsTimerDial(START_GPRSTd, timerVal);

	// Apago el modem
	retS = MCP_setGprsPwr(0);
	// necesario para que no prenda por ruido.
	retS = MCP_setGprsSw(0);
	TERMrprintfProgStrM("GPRS apago modem\r\n\0");

	fl_gprsAllowsSleep = TRUE;
	modemStatus = M_OFF;

	HWtryes = MAX_HWTRYES;
	SWtryes = MAX_SWTRYES;

	// Activo el pwr del modem para que no consuma
	retS = MCP_setGprsPwr(1);
	vTaskDelay( (portTickType)( 1000 / portTICK_RATE_MS ) );

	if ( (systemVars.debugLevel & D_GPRS) != 0) {
		TERMrprintfStr( gprs_printfBuff );
	}

	pvGPRS_printExitMsg(3);
	return(gSST_OFF_Standby);
}
//------------------------------------------------------------------------------------
static int gac04_fn(void)
{

	// gST_Off_Standby -> gST_Off_await01
	// Prendo el modem y espero el settle time de la fuente ( 5s)

RtcTimeType rtcDateTime;
u16 now;

	// Hora actual en minutos.
	rtcGetTime(&rtcDateTime);
	now = rtcDateTime.hour * 60 + rtcDateTime.min;

	// Evaluo el pwrSave
	fl_pwrSave = FALSE;
	// Case1: pwrSave solo funciona en modo discreto
	if ( systemVars.pwrMode != PWR_DISCRETO ) { goto quit; }

	// Case2: modo discreto y pwrSave activado
	if ( systemVars.pwrSave == modoPWRSAVE_ON ) {

		// Caso 2.1: pwrStart < pwrEnd
		if ( systemVars.pwrSaveStartTime < systemVars.pwrSaveEndTime ) {
			if ( ( now > systemVars.pwrSaveStartTime) && ( now < systemVars.pwrSaveEndTime) ) {
				// Estoy dentro del intervalo de pwrSave.
				fl_pwrSave = TRUE;
				goto quit;
			}
		}

		// Caso 2.2: pwrStart > pwrEnd ( deberia ser lo mas comun )
		if ( systemVars.pwrSaveStartTime >= systemVars.pwrSaveEndTime ) {
			if ( now < systemVars.pwrSaveEndTime) {
				fl_pwrSave = TRUE;
				goto quit;
			}

			if ( now > systemVars.pwrSaveStartTime ) {
				fl_pwrSave = TRUE;
				goto quit;
			}
		}
	}

quit:

	pvGPRS_printExitMsg(4);
	return(gSST_OFF_PwrSave);
}
//------------------------------------------------------------------------------------
static int gac05_fn(void)
{
	// gST_Off_await01 -> gST_Off_await02
	// Genero un pulso en el SW del modem para prenderlo

s08 retS;

	modemStatus = M_ON_CONFIG;

	retS = MCP_setGprsSw(0);
	vTaskDelay( (portTickType)( 500 / portTICK_RATE_MS ) );
	retS = MCP_setGprsSw(1);

	// Inicializo el timer para esperar el pwr settle time
	// (que empieze a pizcar)
	ac_gprsTimer1(START_GPRST1, 30, (s08) NULL);

	if ( (systemVars.debugLevel & D_GPRS) != 0) {
		tickCount = xTaskGetTickCount();
		snprintf_P( gprs_printfBuff,CHAR256,PSTR(".[%06lu] tkGprs::gTR05 (t=%lu)(sw=%d)(hw=%d):\r\n\0"),tickCount, sTimerTimeRemains(xTimer_gprsT1),SWtryes,HWtryes);
		TERMrprintfStr( gprs_printfBuff );
	}

	pvGPRS_printExitMsg(5);
	return(gSST_OFF_await02);
}
//------------------------------------------------------------------------------------
static int gac06_fn(void)
{
	// gST_Off_await02 -> gST_Off_await03

	// Envio AT
	pvGPRS_sendCmd ("AT", FALSE, FALSE );

	// Inicializo el timer para esperar el cmdRsp
	ac_gprsTimer1(START_GPRST1, 2, (s08) NULL);

	pvGPRS_printExitMsg(6);
	return(gSST_OFF_await03);
}
//------------------------------------------------------------------------------------
static int gac07_fn(void)
{
	// gST_Off_await03 -> gST_Off_dummy01

	// Leo y Evaluo la respuesta al comando AT
	if ( pvGPRS_RspIs("OK\0") == TRUE ) {
		GPRSrsp = RSP_OK;

		if ( (systemVars.debugLevel & D_GPRS) != 0) {
			tickCount = xTaskGetTickCount();
			snprintf_P( gprs_printfBuff,CHAR256,PSTR(".[%06lu] tkGprs::gTR07 Rsp=\r\n\0"),tickCount);
			TERMrprintfStr( gprs_printfBuff );
			TERMrprintfStr( getGprsRxBufferPtr() );
			TERMrprintfCRLF();
		}

	}

	pvGPRS_printExitMsg(7);
	return(gSST_OFF_dummy01);
}
//------------------------------------------------------------------------------------
static int gac08_fn(void)
{
	// gST_Off_dummy01 -> gST_OnOffline

	pvGPRS_printExitMsg(8);

	// CAMBIO DE ESTADO:
	tkGprs_state = gST_ONoffline;
	return(gSST_ONoffline_Entry);
}
//------------------------------------------------------------------------------------
static int gac09_fn(void)
{
	// gST_Off_dummy01 -> gST_Off_dummy02

	pvGPRS_printExitMsg(9);
	return(gSST_OFF_dummy02);
}
//------------------------------------------------------------------------------------
static int gac10_fn(void)
{
	// gST_Off_dummy02 -> gST_Off_await02

s08 retS;

	// Genero un pulso en el SW del modem para prenderlo
	retS = MCP_setGprsSw(0);
	vTaskDelay( (portTickType)( 500 / portTICK_RATE_MS ) );
	retS = MCP_setGprsSw(1);

	// Inicializo el timer para esperar el pwr settle time
	ac_gprsTimer1(START_GPRST1, 30, (s08) NULL);

	if ( SWtryes > 0 ) { SWtryes--; }

	if ( (systemVars.debugLevel & D_GPRS) != 0) {
		tickCount = xTaskGetTickCount();
		snprintf_P( gprs_printfBuff,CHAR256,PSTR(".[%06lu] tkGprs::gTR10:(t=%lu)(sw=%d)(hw=%d):\r\n\0"), tickCount, sTimerTimeRemains(xTimer_gprsT1),SWtryes,HWtryes);
		TERMrprintfStr( gprs_printfBuff );
	}

	pvGPRS_printExitMsg(10);
	return(gSST_OFF_await02);
}
//------------------------------------------------------------------------------------
static int gac11_fn(void)
{
	// gST_Off_dummy02 -> gST_Off_dummy03

	pvGPRS_printExitMsg(11);
	return(gSST_OFF_dummy03);
}
//------------------------------------------------------------------------------------
static int gac12_fn(void)
{
	// gST_Off_dummy03 -> gST_Off_standby

	s08 retS;

	// Apago y prendo la fuente HW
	retS = MCP_setGprsPwr(0);

	// Inicializo el timer de settle time
	ac_gprsTimer1(START_GPRST1, 15, (s08) NULL);

	if (HWtryes > 0 ) { HWtryes--; }
	SWtryes = MAX_SWTRYES;

	if ( (systemVars.debugLevel & D_GPRS) != 0) {
		tickCount = xTaskGetTickCount();
		snprintf_P( gprs_printfBuff,CHAR256,PSTR(".[%06lu] tkGprs::gTR12:(t=%lu)(sw=%d)(hw=%d):\r\n\0"), tickCount, sTimerTimeRemains(xTimer_gprsT1),SWtryes,HWtryes);
		TERMrprintfStr( gprs_printfBuff );
	}

	pvGPRS_printExitMsg(12);
	return(gSST_OFF_Standby);
}
//------------------------------------------------------------------------------------
static int gac13_fn(void)
{
	// gST_Off_dummy03 -> gST_Off_await04

	// Inicializo el timer en 20m para esperar entes de reintentar.
	ac_gprsTimer1(START_GPRST1, 1200, (s08) NULL);

	pvGPRS_printExitMsg(13);
	return(gSST_OFF_await04);
}
//------------------------------------------------------------------------------------
static int gac14_fn(void)
{
	// gST_Off_dummy03 -> gST_Off_init

	pvGPRS_printExitMsg(14);
	return(gSST_OFF_Init);
}
//------------------------------------------------------------------------------------
static int gac15_fn(void)
{
	// gST_Off_Standby -> gST_Off_PWRSAVE
s08 retS;

	// Activo la fuente
	if ( (systemVars.debugLevel & D_BASIC) != 0) {
		TERMrprintfProgStrM("GPRS prendo modem..\r\n");
	}

	retS = MCP_setGprsPwr(1);		// Ya deberia estar activada del estado anterior !!
	fl_gprsAllowsSleep = FALSE;
	// Espero que se estabilize 5s
	ac_gprsTimer1(START_GPRST1, 5, &retS );

	pvGPRS_printExitMsg(15);
	return(gSST_OFF_await01);
}
//------------------------------------------------------------------------------------
static int gac16_fn(void)
{
	// gST_Off_PWRSAVE -> gST_Off_Standby

	// Recalculo el tiempo para quedarme en standby ya que estoy en pwrSave.

u32 timerVal;
RtcTimeType rtcDateTime;
u16 now;

	tickCount = xTaskGetTickCount();

	// Hora actual en minutos.
	rtcGetTime(&rtcDateTime);
	now = rtcDateTime.hour * 60 + rtcDateTime.min;

	// Si estoy en modo MONITOR ( SQE / FRAME ) no espero nada.
	if ( (systemVars.wrkMode == WK_MONITOR_FRAME) || (systemVars.wrkMode == WK_MONITOR_SQE )) {
		timerVal = 5;
		goto quit;
	}

	// Caso 1: pwrStart < pwrEnd
	if ( systemVars.pwrSaveStartTime < systemVars.pwrSaveEndTime ) {
		if ( ( now > systemVars.pwrSaveStartTime) && ( now < systemVars.pwrSaveEndTime) ) {
			timerVal = (u32)( systemVars.pwrSaveEndTime - now ) * 60;
			goto quit;
		}
	}

	if ( systemVars.pwrSaveStartTime >= systemVars.pwrSaveEndTime ) {
		if ( now < systemVars.pwrSaveEndTime) {
			timerVal = (u32)( systemVars.pwrSaveEndTime - now ) * 60;
			goto quit;
		}
		if ( now > systemVars.pwrSaveStartTime ) {
			timerVal = systemVars.pwrSaveEndTime + ( 1440 - now);
			timerVal *= 60;
			goto quit;
		}
	}

quit:

	// Arranco el timer a esperar
	ac_gprsTimerDial(START_GPRSTd, timerVal);
	snprintf_P( gprs_printfBuff,CHAR256,PSTR(".[%06lu] tkGprs::gTR16 (pwrSave) (await=%lu)(tnd=%lu):\r\n\0"), tickCount, timerVal,getTimeToNextDial() );
	if ( (systemVars.debugLevel & D_GPRS) != 0) {
		TERMrprintfStr( gprs_printfBuff );
	}

	pvGPRS_printExitMsg(16);
	return(gSST_OFF_Standby);
}
//------------------------------------------------------------------------------------

/*
 *  FUNCIONES DEL ESTADO ON-OFFLINE:
 *  El modem esta prendido pero requiere de congiguracion, APN, IP.
 *  Utilizamos el timer1 y 2 contadores, tryes y ptryes.
 *
 */
//------------------------------------------------------------------------------------
static int gac20_fn(void)
{
	// stOFF::gSST_ONoffline_Entry -> gSST_ONoffline_cpin

	// init_tryes
	tryes = MAX_TRYES;
	if ( (systemVars.debugLevel & D_BASIC) != 0) {
		TERMrprintfProgStrM("GPRS configuro modem..\r\n");
	}

	// query CPIN
	pvGPRS_sendCmd ("AT+CPIN?", FALSE, FALSE );

	vTaskDelay( (portTickType)(1000 / portTICK_RATE_MS) );

	// Leo y Evaluo la respuesta al comando AT
	if ( pvGPRS_RspIs("READY") == TRUE ) {
		GPRSrsp = RSP_READY;

		if ( (systemVars.debugLevel & D_GPRS) != 0) {
			tickCount = xTaskGetTickCount();
			snprintf_P( gprs_printfBuff,CHAR256,PSTR(".[%06lu] tkGprs::gTR20 Rsp=\r\n\0"),tickCount);
			TERMrprintfStr( gprs_printfBuff );
			TERMrprintfStr( getGprsRxBufferPtr() );
			TERMrprintfCRLF();
		}
	}

	pvGPRS_printExitMsg(20);
	return(gSST_ONoffline_cpin);

}
//------------------------------------------------------------------------------------
static int gac21_fn(void)
{
	// gSST_ONoffline_cpin -> gSST_ONoffline_net

	// Send CONFIG
	pvGPRS_sendCmd ("AT&D0&C1", FALSE, FALSE );
	vTaskDelay( (portTickType)( 100 / portTICK_RATE_MS ) );
	if ( (systemVars.debugLevel & D_GPRS) != 0) {
		TERMrprintfStr( getGprsRxBufferPtr() );
		TERMrprintfCRLF();
	}

	// Configuro la secuencia de escape +++AT
	pvGPRS_sendCmd ("AT*E2IPS=2,8,2,1020,0,15", FALSE, FALSE );
	vTaskDelay( (portTickType)( 100 / portTICK_RATE_MS ) );
	if ( (systemVars.debugLevel & D_GPRS) != 0) {
		TERMrprintfStr( getGprsRxBufferPtr() );
		TERMrprintfCRLF();
	}

	// SMS Envio: Los envio en modo texto
	pvGPRS_sendCmd ("AT+CMGF=1", FALSE, FALSE );
	vTaskDelay( (portTickType)( 100 / portTICK_RATE_MS ) );
	if ( (systemVars.debugLevel & D_GPRS) != 0) {
		TERMrprintfStr( getGprsRxBufferPtr() );
		TERMrprintfCRLF();
	}

	// SMS Recepcion: No indico al TE ni le paso el mensaje
	pvGPRS_sendCmd ("AT+CNMI=1,0", FALSE, FALSE );
	vTaskDelay( (portTickType)( 100 / portTICK_RATE_MS ) );
	if ( (systemVars.debugLevel & D_GPRS) != 0) {
		TERMrprintfStr( getGprsRxBufferPtr() );
		TERMrprintfCRLF();
	}

	// SMS indicacion: Bajando el RI por 100ms.
	pvGPRS_sendCmd ("AT*E2SMSRI=100", FALSE, FALSE );
	vTaskDelay( (portTickType)( 100 / portTICK_RATE_MS ) );
	if ( (systemVars.debugLevel & D_GPRS) != 0) {
		TERMrprintfStr( getGprsRxBufferPtr() );
		TERMrprintfCRLF();
	}

	// Borro todos los SMS de la memoria
	pvGPRS_sendCmd ("AT+CMGD=0,4", FALSE, FALSE );
	vTaskDelay( (portTickType)( 100 / portTICK_RATE_MS ) );
	if ( (systemVars.debugLevel & D_GPRS) != 0) {
		TERMrprintfStr( getGprsRxBufferPtr() );
		TERMrprintfCRLF();
	}
	// Deshabilito los mensajes
	pvGPRS_sendCmd ("AT*E2IPEV=0,0",FALSE, FALSE );
	vTaskDelay( (portTickType)( 100 / portTICK_RATE_MS ) );
	if ( (systemVars.debugLevel & D_GPRS) != 0) {
		TERMrprintfStr( getGprsRxBufferPtr() );
		TERMrprintfCRLF();
	}

	// Query NET available
	pvGPRS_sendCmd ("AT+CREG?", FALSE, FALSE );
	TERMrprintfStr( getGprsRxBufferPtr() );
	vTaskDelay( (portTickType)( 100 / portTICK_RATE_MS ) );
	TERMrprintfCRLF();

	// init_timer1 ( 5s) para esperar respuesta del comado NET
	ac_gprsTimer1(START_GPRST1, 5, (s08) NULL);

	// init tryes
	tryes = MAX_TRYES;

	if ( (systemVars.debugLevel & D_GPRS) != 0) {
		tickCount = xTaskGetTickCount();
		snprintf_P( gprs_printfBuff,CHAR256,PSTR(".[%06lu] tkGprs::gTR21 NET register (%d)\r\n\0"),tickCount, tryes);
		TERMrprintfStr( gprs_printfBuff );
	}

	pvGPRS_printExitMsg(21);
	return(gSST_ONoffline_net);
}
//------------------------------------------------------------------------------------
static int gac22_fn(void)
{
	// gST_Off_cpin -> gST_Off_await01

	// init_timer1 ( 10s) para re-esperar respuesta del comado NET
	ac_gprsTimer1(START_GPRST1, 10, (s08) NULL);

	pvGPRS_printExitMsg(22);
	return(gSST_ONoffline_await01);
}
//------------------------------------------------------------------------------------
static int gac23_fn(void)
{
	// gST_Off_await01 -> gST_Off_dummy01

	pvGPRS_printExitMsg(23);
	return(gSST_ONoffline_dummy01);
}
//------------------------------------------------------------------------------------
static int gac24_fn(void)
{
	// gST_Off_dummy01 -> gST_Off_cpin

	if ( tryes > 0 ) {
		tryes--;
	}

	// query CPIN
	pvGPRS_sendCmd ("AT+CPIN?", FALSE, FALSE );

	vTaskDelay( (portTickType)(500 / portTICK_RATE_MS) );

	// Evaluo la respuesta al comando CPIN
	if ( pvGPRS_RspIs("READY") == TRUE ) {
		GPRSrsp = RSP_READY;

		if ( (systemVars.debugLevel & D_GPRS) != 0) {
			tickCount = xTaskGetTickCount();
			snprintf_P( gprs_printfBuff,CHAR256,PSTR(".[%06lu] tkGprs::gTR24 Rsp=\r\n\0"),tickCount);
			TERMrprintfStr( gprs_printfBuff );
			TERMrprintfStr( getGprsRxBufferPtr() );
			TERMrprintfCRLF();
		}
	}

	pvGPRS_printExitMsg(24);
	return(gSST_ONoffline_cpin);

}
//------------------------------------------------------------------------------------
static int gac25_fn(void)
{
	// gST_Off_dummy01 -> gST_Off_EXIT

	tkGprs_state = gST_OFF;
	pvGPRS_printExitMsg(25);
	return(gSST_OFF_Entry);

}
//------------------------------------------------------------------------------------
static int gac26_fn(void)
{
	// stOFF::gSST_ONoffline_net -> gSST_ONoffline_dummy02

	// Evaluo la respuesta al comando AT+CREG ( home network )
	// Evaluo la respuesta al comando CPIN
	if ( pvGPRS_RspIs("+CREG: 0,1") == TRUE ) {
		GPRSrsp = RSP_CREG;

		if ( (systemVars.debugLevel & D_GPRS) != 0) {
			tickCount = xTaskGetTickCount();
			snprintf_P( gprs_printfBuff,CHAR256,PSTR(".[%06lu] tkGprs::gTR26 Rsp=\r\n\0"),tickCount);
			TERMrprintfStr( gprs_printfBuff );
			TERMrprintfStr( getGprsRxBufferPtr() );
			TERMrprintfCRLF();
		}
	}

	// Evaluo la respuesta al comando AT+CREG ( roaming network )
//	if ( gprsRspIs("+CREG: 0,5") == TRUE ) {
//		GPRSrsp = RSP_CREG;
//		debugPrintModemBuffer();
//	}

	pvGPRS_printExitMsg(26);
	return(gSST_ONoffline_dummy02);
}
//------------------------------------------------------------------------------------
static int gac27_fn(void)
{
	// gST_Off_dummy02 -> gST_Off_dummy03

	pvGPRS_printExitMsg(27);
	return(gSST_ONoffline_dummy03);
}
//------------------------------------------------------------------------------------
static int gac28_fn(void)
{
	// gST_Off_dummy03 -> gST_Off_net

	if (tryes > 0) {
		tryes--;
	}

	// init_timer1 ( 5s) para esperar respuesta del reintento al comado NET
	ac_gprsTimer1(START_GPRST1, 5, (s08) NULL);

	// Re-Query NET available
	pvGPRS_sendCmd ("AT+CREG?", FALSE, FALSE );
	if ( (systemVars.debugLevel & D_GPRS) != 0) {
		tickCount = xTaskGetTickCount();
		snprintf_P( gprs_printfBuff,CHAR256,PSTR(".[%06lu] tkGprs::gTR24 NET(%d) Retry=AT+CREG?\r\n\0"),tickCount,tryes);
		TERMrprintfStr( gprs_printfBuff );
		TERMrprintfStr( getGprsRxBufferPtr() );
		TERMrprintfCRLF();
	}

	pvGPRS_printExitMsg(28);
	return(gSST_ONoffline_net);
}
//------------------------------------------------------------------------------------
static int gac29_fn(void)
{
	// gST_Off_dummy03 -> gSST_OFF_Entry

	tkGprs_state = gST_OFF;
	pvGPRS_printExitMsg(29);
	return(gSST_OFF_Entry);

}
//------------------------------------------------------------------------------------
static int gac30_fn(void)
{
	// gSST_ONoffline_dummy02 -> gSST_ONoffline_sqe01

	// Print Net response
	if ( (systemVars.debugLevel & D_GPRS) != 0) {
		tickCount = xTaskGetTickCount();
		snprintf_P( gprs_printfBuff,CHAR256,PSTR(".[%06lu] tkGprs::gTR30 Rsp=\r\n\0"),tickCount);
		TERMrprintfStr( gprs_printfBuff );
		TERMrprintfStr( getGprsRxBufferPtr() );
		TERMrprintfCRLF();
	}

	pvGPRS_printExitMsg(30);
	return(gSST_ONoffline_sqe01);

}
//------------------------------------------------------------------------------------
static int gac31_fn(void)
{
	// gSST_ONoffline_sqe -> gSST_ONoffline_apn
u08 systemBand;
char *ts = NULL;

	// READ SQE
	ts = strchr(getGprsRxBufferPtr(), ':');
	ts++;
	systemVars.csq = atoi(ts);
	systemVars.dbm = 113 - 2 * systemVars.csq;
	if ( (systemVars.debugLevel & D_GPRS) != 0) {
		tickCount = xTaskGetTickCount();
		snprintf_P( gprs_printfBuff,CHAR256,PSTR(".[%06lu] tkGprs::gTR31 csq=%d\r\n\0"),tickCount, systemVars.csq);
		TERMrprintfStr( gprs_printfBuff );
		TERMrprintfStr( getGprsRxBufferPtr() );
		TERMrprintfCRLF();
	}
	// --------------------------------------------------------------------
	// Query BAND
	pvGPRS_sendCmd ("AT*EBSE?", FALSE, FALSE );
	vTaskDelay( (portTickType)( 100 / portTICK_RATE_MS ) );
	if ( (systemVars.debugLevel & D_GPRS) != 0) {
		tickCount = xTaskGetTickCount();
		snprintf_P( gprs_printfBuff,CHAR256,PSTR(".[%06lu] tkGprs::gTR31 Rsp=\r\n\0"),tickCount);
		TERMrprintfStr( gprs_printfBuff );
		TERMrprintfStr( getGprsRxBufferPtr() );
		TERMrprintfCRLF();
	}

	ts = strchr(getGprsRxBufferPtr(), ':');
	ts++;
	systemBand = atoi(ts);

	if ( (systemVars.debugLevel & D_GPRS) != 0) {
		tickCount = xTaskGetTickCount();
		snprintf_P( gprs_printfBuff,CHAR256,PSTR(".[%06lu] tkGprs::gTR31 GSM_BAND=%d\r\n\0"),tickCount, systemBand );
		TERMrprintfStr( gprs_printfBuff );
	}

	// Debo configurar el gsmBand: se activa la proxima vez que se prenda el modem.
	if ( systemVars.gsmBand != systemBand ) {
		if ( (systemVars.debugLevel & D_BASIC) != 0) {
			snprintf_P( gprs_printfBuff,CHAR256,PSTR("tkGPRS::gTR31: Reconfiguro GSM_BAND a modo %d\r\n\0"),systemVars.gsmBand);
			TERMrprintfStr( gprs_printfBuff );
		}

		// Reconfiguro  el modo
		memset(gprs_printfBuff, NULL, CHAR256);
		snprintf_P( gprs_printfBuff,CHAR256,PSTR("AT*EBSE=%d"),systemVars.gsmBand );
		pvGPRS_sendCmd (gprs_printfBuff, FALSE, FALSE );
		TERMrprintfStr( getGprsRxBufferPtr() );
		TERMrprintfCRLF();
		vTaskDelay( (portTickType)( 100 / portTICK_RATE_MS ) );

		// Guardo el profile
		pvGPRS_sendCmd ("AT&W",FALSE, FALSE );
		TERMrprintfStr( getGprsRxBufferPtr() );
		TERMrprintfCRLF();

		// Salgo a resetear el modem
		arranqueFlag = TRUE;
		tkGprs_state = gST_OFF;

		pvGPRS_printExitMsg(31);
		return(gSST_OFF_Entry);
	}

	// SET APN
	memset(gprs_printfBuff, NULL, CHAR256);
	snprintf_P( gprs_printfBuff,CHAR256,PSTR("AT+CGDCONT=1,\"IP\",\"%s\""),systemVars.apn);
	pvGPRS_sendCmd (gprs_printfBuff, FALSE, FALSE );
	vTaskDelay( (portTickType)(500 / portTICK_RATE_MS) );

	if ( (systemVars.debugLevel & D_GPRS) != 0) {
		tickCount = xTaskGetTickCount();
		snprintf_P( gprs_printfBuff,CHAR256,PSTR(".[%06lu] tkGprs::gTR31 Rsp=\r\n\0"),tickCount );
		TERMrprintfStr( gprs_printfBuff );
		TERMrprintfStr( getGprsRxBufferPtr() );
		TERMrprintfCRLF();
	}

	// init_timer (5s)
	ac_gprsTimer1(START_GPRST1, 5, (s08) NULL );

	pvGPRS_printExitMsg(31);
	return(gSST_ONoffline_apn);

}
//------------------------------------------------------------------------------------
static int gac32_fn(void)
{
	// gSST_ONoffline_apn -> gSST_ONoffline_ip

	// Vamos a esperar en forma persistente hasta 30s. por la IP durante 3 intentos
	tryes = MAX_TRYES;
	pTryes = 6;

	ac_gprsTimer1(START_GPRST1, 5, (s08) NULL );

	// ASK IP.
	pvGPRS_sendCmd ("AT*E2IPA=1,1", FALSE, FALSE );

	if ( (systemVars.debugLevel & D_GPRS) != 0) {
		tickCount = xTaskGetTickCount();
		snprintf_P( gprs_printfBuff,CHAR256,PSTR(".[%06lu] tkGprs::gTR32 ASK IP (%d).\r\n\0"),tickCount, tryes);
		TERMrprintfStr( gprs_printfBuff );
	}

	pvGPRS_printExitMsg(32);
	return(gSST_ONoffline_ip);

}
//------------------------------------------------------------------------------------
static int gac33_fn(void)
{
	// stOFF::gSST_ONoffline_ip -> gSST_ONoffline_dummy04

	// Evaluo la respuesta al comando AT*E2IPA
	// Evaluo la respuesta al comando CPIN
	if ( pvGPRS_RspIs("OK\0") == TRUE ) {
		GPRSrsp = RSP_IPOK;
	}

	if ( (systemVars.debugLevel & D_GPRS) != 0) {
		tickCount = xTaskGetTickCount();
		snprintf_P( gprs_printfBuff,CHAR256,PSTR(".[%06lu] tkGprs::gTR33 Rsp=\r\n\0"),tickCount);
		TERMrprintfStr( gprs_printfBuff );
		TERMrprintfStr( getGprsRxBufferPtr() );
		TERMrprintfCRLF();
	}

	pvGPRS_printExitMsg(33);
	return(gSST_ONoffline_dummy04);
}
//------------------------------------------------------------------------------------
static int gac34_fn(void)
{
	// gST_Off_dummy04 -> gST_Off_dummy05

	pvGPRS_printExitMsg(34);
	return(gSST_ONoffline_dummy05);
}
//------------------------------------------------------------------------------------
static int gac35_fn(void)
{
	// gST_Off_dummy05 -> gST_Off_ip

	if ( pTryes > 0 ) {
		pTryes--;
	}

	ac_gprsTimer1(START_GPRST1, 5, (s08) NULL );

	pvGPRS_printExitMsg(35);
	return(gSST_ONoffline_ip);
}
//------------------------------------------------------------------------------------
static int gac36_fn(void)
{
	// gST_Off_dummy05 -> gST_Off_dummy06

	pvGPRS_printExitMsg(36);
	return(gSST_ONoffline_dummy06);
}
//------------------------------------------------------------------------------------
static int gac37_fn(void)
{
	// gSST_ONoffline_dummy06 -> gSST_ONoffline_ip

	if (tryes > 0 ) {
		tryes--;
	}
	pTryes = 6;
	ac_gprsTimer1(START_GPRST1, 5, (s08) NULL );

	// ASK IP.
	pvGPRS_sendCmd ("AT*E2IPA=1,1", FALSE, FALSE );

	if ( (systemVars.debugLevel & D_GPRS) != 0) {
		tickCount = xTaskGetTickCount();
		snprintf_P( gprs_printfBuff,CHAR256,PSTR(".[%06lu] tkGprs::gTR37: ASK IP (%d)\r\n\0"),tickCount, tryes);
		TERMrprintfStr( gprs_printfBuff );
	}

	pvGPRS_printExitMsg(37);
	return(gSST_ONoffline_ip);

}
//------------------------------------------------------------------------------------
static int gac38_fn(void)
{
	// gST_Off_dummy06 -> gSST_OFF_Entry

	tkGprs_state = gST_OFF;
	pvGPRS_printExitMsg(38);
	return(gSST_OFF_Entry);

}
//------------------------------------------------------------------------------------
static int gac39_fn(void)
{
	// gSST_ONoffline_dummy04 -> gSST_ON_INIT

	// Query IP address.
char *ts = NULL;
int i=0;
char c;

	// ASK IP.
	pvGPRS_sendCmd ("AT*E2IPI=0", FALSE, FALSE );

	vTaskDelay( (portTickType)( 500 / portTICK_RATE_MS ) );

	// Leo la respuesta del modem al comando anterior ( E2IPA=1,1)
	if ( (systemVars.debugLevel & D_GPRS) != 0) {
		tickCount = xTaskGetTickCount();
		snprintf_P( gprs_printfBuff,CHAR256,PSTR(".[%06lu] tkGprs::gTR39  Rsp=\r\n\0"),tickCount);
		TERMrprintfStr( gprs_printfBuff );
		TERMrprintfStr( getGprsRxBufferPtr() );
		TERMrprintfCRLF();
	}

	// Extraigo la IP del token
	ts = strchr( getGprsRxBufferPtr(), '\"');
	ts++;
	while ( (c= *ts) != '\"') {
		systemVars.dlgIp[i++] = c;
		ts++;
	}
	systemVars.dlgIp[i++] = NULL;

	modemStatus = M_ON_READY;

	pvGPRS_printExitMsg(39);
	// CAMBIO DE ESTADO:
	tkGprs_state = gST_ONInit;
	return(gSST_ONinit_Entry);
}
//------------------------------------------------------------------------------------
static int gac40_fn(void)
{
	// gSST_ONoffline_sqe01 -> gSST_ONoffline_sqe02

	// Query SQE
	pvGPRS_sendCmd ("AT+CSQ", FALSE, FALSE );

	// init_timer1 ( 5s) para esperar respuesta del reintento al comado CSQ
	ac_gprsTimer1(START_GPRST1, 5 , (s08) NULL );

	pvGPRS_printExitMsg(40);
	return(gSST_ONoffline_sqe02);

}
//------------------------------------------------------------------------------------
static int gac41_fn(void)
{
	// gSST_ONoffline_sqe01 -> gSST_ONoffline_sqe03

	// Query SQE
	pvGPRS_sendCmd ("AT+CSQ", FALSE, FALSE );

	// init tryes
	tryes = 50;

	// init_timer1 ( 10s) para esperar respuesta del reintento al comado CSQ
	ac_gprsTimer1(START_GPRST1, 10, (s08) NULL );

	pvGPRS_printExitMsg(41);
	return(gSST_ONoffline_sqe03);

}
//------------------------------------------------------------------------------------
static int gac42_fn(void)
{
	// gSST_ONoffline_sqe03 -> gSST_ONoffline_sqe04

char *ts = NULL;

	// READ SQE
	if ( (systemVars.debugLevel & D_GPRS) != 0) {
		tickCount = xTaskGetTickCount();
		snprintf_P( gprs_printfBuff,CHAR256,PSTR(".[%06lu] tkGprs::gTR41 Rsp=\r\n\0"),tickCount);
		TERMrprintfStr( gprs_printfBuff );
		TERMrprintfStr( getGprsRxBufferPtr() );
		TERMrprintfCRLF();
	}

	ts = strchr(getGprsRxBufferPtr(), ':');
	ts++;
	systemVars.csq = atoi(ts);
	systemVars.dbm = 113 - 2 * systemVars.csq;

	snprintf_P( gprs_printfBuff,CHAR256,PSTR("tkGprs::gTR41 SQE monitor(%d): %d -> %d(dB)r\n\0"),tryes, systemVars.csq, systemVars.dbm);
	TERMrprintfStr( gprs_printfBuff );

	// dec_Tryes
	if (tryes > 0) {
		tryes--;
	}

	pvGPRS_printExitMsg(42);
	return(gSST_ONoffline_sqe04);

}
//------------------------------------------------------------------------------------
static int gac43_fn(void)
{
	// gSST_ONoffline_sqe04 -> gSST_ONoffline_sqe03

	// Query SQE
	pvGPRS_sendCmd ("AT+CSQ", FALSE, FALSE );

	// init_timer1 ( 10s) para esperar respuesta del reintento al comado CSQ
	ac_gprsTimer1(START_GPRST1, 10, (s08) NULL );

	pvGPRS_printExitMsg(43);
	return(gSST_ONoffline_sqe03);

}
//------------------------------------------------------------------------------------
static int gac44_fn(void)
{
	// gSST_ONoffline_sqe04 -> gSST_OFF_Entry

	// Salgo del modo monitor. Debo pasar a modo normal y discar inmediatamente.
	arranqueFlag = TRUE;
	while ( xSemaphoreTake( sem_SYSVars, ( TickType_t ) 10 ) != pdTRUE )
		taskYIELD();
	systemVars.wrkMode = WK_NORMAL;
	xSemaphoreGive( sem_SYSVars );
	xTaskNotify(xHandle_tkAIn, AINMSG_RELOAD , eSetBits );

	tkGprs_state = gST_OFF;
	pvGPRS_printExitMsg(44);
	return(gSST_OFF_Entry);

}
/*------------------------------------------------------------------------------------*/
/*
 *  FUNCIONES DEL ESTADO ON_INIT:
 *  Es el estado en el cual el modem esta abriendo peor primera vez un socket hacia
 *  el server, enviando un frame de INIT y esperando la respuesta para reconfigurarse
 */
/*------------------------------------------------------------------------------------*/

static int gac50_fn(void)
{

	// gSST_ONInitEntry -> gSST_ONInit_socket

	// Espero en forma persistente abrir el socket por 16s
	sockTryes = MAX_SOCKTRYES;
	pTryes = 5;
	ac_gprsTimer1(START_GPRST1, 5, (s08) NULL );
	TERMrprintfProgStrM("GPRS conecto al server..\r\n");

	if ( (systemVars.debugLevel & D_GPRS) != 0) {
		tickCount = xTaskGetTickCount();
		snprintf_P( gprs_printfBuff,CHAR256,PSTR(".[%06lu] tkGprs::gTR50 OPEN SOCKET (%d)\r\n\0"),tickCount, sockTryes);
		TERMrprintfStr( gprs_printfBuff );
	}

	// OPEN SOCKET.
	pvGPRS_openSocket();

	pvGPRS_printExitMsg(50);
	return(gSST_ONinit_socket);

}
/*------------------------------------------------------------------------------------*/

static int gac51_fn(void)
{
	// gSST_ONinit_socket -> gSST_ONinit_dummyS01

	// Evaluo la respuesta al comando AT*E2IPO ( open socket )

	if ( pvGPRS_RspIs("CONNECT") == TRUE ) {
		GPRSrsp = RSP_CONNECT;
		// El socket abrio: Muestro el mensaje del modem (CONNECT)
	}

	if ( (systemVars.debugLevel & D_GPRS) != 0) {
		tickCount = xTaskGetTickCount();
		snprintf_P( gprs_printfBuff,CHAR256,PSTR(".[%06lu] tkGprs::gTR51 Rsp=\r\n\0"),tickCount);
		TERMrprintfStr( gprs_printfBuff );
		TERMrprintfStr( getGprsRxBufferPtr() );
		TERMrprintfCRLF();
	}

	pvGPRS_printExitMsg(51);
	return(gSST_ONinit_dummyS01);

}
/*------------------------------------------------------------------------------------*/

static int gac52_fn(void)
{
	// gSST_ONinit_dummyS01 -> gSST_ONinit_dummyS02

	pvGPRS_printExitMsg(52);
	return(gSST_ONinit_dummyS02);

}
/*------------------------------------------------------------------------------------*/

static int gac53_fn(void)
{
	// gSST_ONinit_dummyS02 -> gSST_ONinit_socket

	if (pTryes > 0 ) {
		pTryes--;
	}
	ac_gprsTimer1(START_GPRST1, 3, (s08) NULL );

	pvGPRS_printExitMsg(53);
	return(gSST_ONinit_socket);

}
/*------------------------------------------------------------------------------------*/

static int gac54_fn(void)
{
	// gSST_ONinit_dummyS02 -> gSST_ONinit_dummyS03

	pvGPRS_printExitMsg(54);
	return(gSST_ONinit_dummyS03);

}
/*------------------------------------------------------------------------------------*/

static int gac55_fn(void)
{

	// gSST_ONInit_dummyS03 -> gSST_ONInit_socket

	// Espero en forma persistente abrir el socket por 16s
	if ( sockTryes > 0 ) {
		sockTryes--;
	}
	pTryes = 8;
	ac_gprsTimer1(START_GPRST1, 5, (s08) NULL );

	// OPEN SOCKET.
	pvGPRS_openSocket();

	if ( (systemVars.debugLevel & D_GPRS) != 0) {
		tickCount = xTaskGetTickCount();
		snprintf_P( gprs_printfBuff,CHAR256,PSTR(".[%06lu] tkGprs::gTR55 RETRY OPEN SOCKET (%d)\r\n\0"),tickCount, sockTryes);
		TERMrprintfStr( gprs_printfBuff );
	}

	pvGPRS_printExitMsg(55);
	return(gSST_ONinit_socket);

}
/*------------------------------------------------------------------------------------*/

static int gac56_fn(void)
{
	// gSST_ONInit_dummyS03 -> EXIT

	tkGprs_state = gST_OFF;
	pvGPRS_printExitMsg(56);
	return(gSST_OFF_Entry);

}
/*------------------------------------------------------------------------------------*/

static int gac57_fn(void)
{
	// gSST_ONinit_dummy01 -> gSST_ONinit_initFrame

	if ( (systemVars.debugLevel & D_GPRS) != 0) {
		tickCount = xTaskGetTickCount();
		snprintf_P( gprs_printfBuff,CHAR256,PSTR(".[%06lu] tkGprs::gTR57 INIT FRAME (%d)\r\n\0"),tickCount, sockTryes);
		TERMrprintfStr( gprs_printfBuff );
	}

	// El socket abrio por lo que tengo que trasmitir el frame de init.
	pvGPRS_sendInitFrame();

	// No inicializo ya que el total de reintentos combinados no debe superar a LOOPTryes
	if ( sockTryes > 0 ) {
		sockTryes--;
	}
	// Vamos a esperar en forma persistente hasta 15s
	pTryes = 6;
	ac_gprsTimer1(START_GPRST1, 3, (s08) NULL );

	pvGPRS_printExitMsg(57);
	return(gSST_ONinit_initFrame);

}
/*------------------------------------------------------------------------------------*/

static int gac58_fn(void)
{
	// gSST_ONinit_initFrame -> gSST_ONinit_dummyF01

	// Evaluo si llego una respuesta del server al frame de INIT
	if ( pvGPRS_RspIs("INIT_OK") == TRUE ) {
		GPRSrsp = RSP_INIT;
	}

	if ( (systemVars.debugLevel & D_GPRS) != 0) {
		tickCount = xTaskGetTickCount();
		snprintf_P( gprs_printfBuff,CHAR256,PSTR(".[%06lu] tkGprs::gTR58 Rsp=\r\n\0"),tickCount  );
		TERMrprintfStr( gprs_printfBuff );
		TERMrprintfStr( getGprsRxBufferPtr() );
		TERMrprintfCRLF();

	}

	pvGPRS_printExitMsg(58);
	return( gSST_ONinit_dummyF01);

}
/*------------------------------------------------------------------------------------*/

static int gac59_fn(void)
{
	// gSST_ONinit_dummyF01 -> gSST_ONData_Entry

u08 i;
u08 saveFlag = 0;
s08 retS = FALSE;

	// Analizo y proceso la respuesta del buffer.
	// Lineas de respuesta al INIT: tag: INIT_OK:
	// Pueden contener parametros de reconfiguracion.
	saveFlag = 0;
	pvGPRS_processServerClock();
	saveFlag = pvGPRS_processPwrMode();
	saveFlag += pvGPRS_processTimerPoll();
	saveFlag += pvGPRS_processTimerDial();
	saveFlag += pvGPRS_processPwrSave();

	// Reconfiguro los canales.
	saveFlag += pvGPRS_processAnalogChannels(0);
	saveFlag += pvGPRS_processAnalogChannels(1);
	saveFlag += pvGPRS_processAnalogChannels(2);

	saveFlag += pvGPRS_processDigitalChannels(0);
	saveFlag += pvGPRS_processDigitalChannels(1);

	// Salidas
	saveFlag += pvGPRS_processOuputs();

	if ( saveFlag > 0 ) {
		for ( i=0; i<3; i++) {
			vTaskDelay( (portTickType)( 500 / portTICK_RATE_MS ) );
			retS = pvSaveSystemParamsInEE( &systemVars );

			if ( (systemVars.debugLevel & D_GPRS) != 0) {
				tickCount = xTaskGetTickCount();
				if ( retS ) {
					snprintf_P( gprs_printfBuff,CHAR256,PSTR(".[%06lu] tkGprs::gTR59 SAVE OK\r\n\0"),tickCount);
					TERMrprintfStr( gprs_printfBuff );
					break;
				} else {
					snprintf_P( gprs_printfBuff,CHAR256,PSTR(".[%06lu] tkGprs::gTR59 FAIL OK\r\n\0"),tickCount);
					TERMrprintfStr( gprs_printfBuff );
				}
			}
		}
	}

	// Le debo avisar a tkAnalog que se reconfigure.
	xTaskNotify(xHandle_tkAIn, AINMSG_RELOAD , eSetBits );

	pvGPRS_closeSocket();
	ac_gprsTimer1(START_GPRST1, 10, (s08) NULL );

	pvGPRS_printExitMsg(59);
	return(gSST_ONinit_await01);
}
/*------------------------------------------------------------------------------------*/

static int gac60_fn(void)
{
	// gSST_ONinit_dummyF01 -> gSST_ONinit_dummyF02

	pvGPRS_printExitMsg(60);
	return(gSST_ONinit_dummyF02);

}
/*------------------------------------------------------------------------------------*/

static int gac61_fn(void)
{
	// gSST_ONinit_dummyF02 -> gSST_ONinit_dummyF03

	pvGPRS_printExitMsg(61);
	return(gSST_ONinit_dummyF03);

}
/*------------------------------------------------------------------------------------*/

static int gac62_fn(void)
{
	// gSST_ONinit_dummyF03 -> gSST_ONinit_initFrame

	if (pTryes > 0 ) {
		pTryes--;
	}
	ac_gprsTimer1(START_GPRST1, 5, (s08) NULL );

	pvGPRS_printExitMsg(62);
	return(gSST_ONinit_initFrame);

}
/*------------------------------------------------------------------------------------*/

static int gac63_fn(void)
{
	// gSST_ONinit_dummyF03 -> gSST_ONinit_dummyF04

	pvGPRS_printExitMsg(63);
	return(gSST_ONinit_dummyF04);

}
/*------------------------------------------------------------------------------------*/

static int gac64_fn(void)
{
	// gSST_ONinit_dummyF04 -> gSST_ONinit_initFrame

	pvGPRS_sendInitFrame();

	// No inicializo ya que el total de reintentos combinados no debe superar a LOOPTryes
	if ( sockTryes > 0 ) {
		sockTryes--;
	}
	// Vamos a esperar en forma persistente hasta 15s
	pTryes = 5;
	ac_gprsTimer1(START_GPRST1, 3, (s08) NULL );

	if ( (systemVars.debugLevel & D_GPRS) != 0) {
		tickCount = xTaskGetTickCount();
		snprintf_P( gprs_printfBuff,CHAR256,PSTR(".[%06lu] tkGprs::gTR64 RETRY INIT FRAME (%d)\r\n\0"),tickCount, sockTryes);
		TERMrprintfStr( gprs_printfBuff );
	}

	pvGPRS_printExitMsg(64);
	return(gSST_ONinit_initFrame);

}
/*------------------------------------------------------------------------------------*/

static int gac65_fn(void)
{
	// gSST_ONInit_dummyF04 -> EXIT

	tkGprs_state = gST_OFF;
	pvGPRS_printExitMsg(65);
	return(gSST_OFF_Entry);

}
/*------------------------------------------------------------------------------------*/

static int gac66_fn(void)
{
	// gSST_ONinit_dummyF02 -> gSST_ONinit_dummyF05

	pvGPRS_printExitMsg(66);
	return(gSST_ONinit_dummyF05);

}
/*------------------------------------------------------------------------------------*/

static int gac67_fn(void)
{

	// gSST_ONInit_dummy05 -> gSST_ONInit_socket

	// Espero en forma persistente abrir el socket por 30s
	pTryes = 15;

	ac_gprsTimer1(START_GPRST1, 2, (s08) NULL );

	// OPEN SOCKET.
	pvGPRS_openSocket();

	if ( (systemVars.debugLevel & D_GPRS) != 0) {
		tickCount = xTaskGetTickCount();
		snprintf_P( gprs_printfBuff,CHAR256,PSTR(".[%06lu] tkGprs::gTR67 RETRY OPEN SOCKET (%d)\r\n\0"),tickCount, sockTryes);
		TERMrprintfStr( gprs_printfBuff );
	}

	pvGPRS_printExitMsg(67);
	return(gSST_ONinit_socket);

}
/*------------------------------------------------------------------------------------*/

static int gac68_fn(void)
{
	// gSST_ONInit_dummyF05 -> EXIT

	tkGprs_state = gST_OFF;
	pvGPRS_printExitMsg(68);
	return(gSST_OFF_Entry);

}
/*------------------------------------------------------------------------------------*/

static int gac69_fn(void)
{

	// CAMBIO DE ESTADO:

	tkGprs_state = gST_ONData;
	pvGPRS_printExitMsg(69);
	return(gSST_ONdata_Entry);

}
/*------------------------------------------------------------------------------------*/
/*
 *  FUNCIONES DEL ESTADO ONData:
 *  El modem esta prendido y trasmitiendo datos al server.
 */
/*------------------------------------------------------------------------------------*/
static int gac70_fn(void)
{
    // gSST_ONdata_Entry ->  gSST_ONdata_data

	// Inicializo el contador de reintentos de envio del mismo grupo de datos.

	TxWinTryes = 0;

	if ( (systemVars.debugLevel & D_BASIC) != 0) {
		TERMrprintfProgStrM("GPRS trasmito frames..\r\n");
	}

	pvGPRS_printExitMsg(70);
	return(gSST_ONdata_data);
}
/*------------------------------------------------------------------------------------*/
static int gac71_fn(void)
{
    // gSST_ONdata_Data ->  gSST_ONdata_dummy02

	if ( TxWinTryes < MAX_TXWINTRYES ) {
		TxWinTryes++;
	}
	pvGPRS_printExitMsg(71);
	return(gSST_ONdata_dummy02);
}
/*------------------------------------------------------------------------------------*/
static int gac72_fn(void)
{
    // gSST_ONdata_Data ->  gSST_ONdata_dummy01

	pvGPRS_printExitMsg(72);
	return(gSST_ONdata_dummy01);
}
/*------------------------------------------------------------------------------------*/
static int gac73_fn(void)
{
    // gSST_ONdata_dummy01 ->  gSST_ONdata_await01

	// Espero 30s para volver a chequear la BD.
	ac_gprsTimer1(START_GPRST1, 30, (s08) NULL );

	pvGPRS_printExitMsg(73);
	return(gSST_ONdata_await01);
}
/*------------------------------------------------------------------------------------*/
static int gac74_fn(void)
{
	// gSST_ONdata_dummy01 -> EXIT to state OFF.

	tkGprs_state = gST_OFF;

	pvGPRS_printExitMsg(74);
	return(gSST_OFF_Entry);

}
/*------------------------------------------------------------------------------------*/
static int gac75_fn(void)
{
    // gSST_ONdata_await01 ->  gSST_ONdata_data

	pvGPRS_printExitMsg(75);
	return(gSST_ONdata_data);
}
/*------------------------------------------------------------------------------------*/
static int gac76_fn(void)
{

	// gSST_ONdata_dummy02 -> gSST_ONdata_socket

	// Espero en forma persistente abrir el socket por 30s
	sockTryes = MAX_SOCKTRYES;
	pTryes = 5;
	ac_gprsTimer1(START_GPRST1, 3, (s08) NULL );

	if ( (systemVars.debugLevel & D_GPRS) != 0) {
		tickCount = xTaskGetTickCount();
		snprintf_P( gprs_printfBuff,CHAR256,PSTR(".[%06lu] tkGprs::gTR76 OPEN SOCKET (%d)\r\n\0"),tickCount, sockTryes);
		TERMrprintfStr( gprs_printfBuff );
	}

	// OPEN SOCKET.
	pvGPRS_openSocket();

	pvGPRS_printExitMsg(76);
	return(gSST_ONdata_socket);
}
/*------------------------------------------------------------------------------------*/
static int gac77_fn(void)
{
	// gSST_ONdata_socket -> gSST_ONdata_dummyS01
	if ( pvGPRS_RspIs("CONNECT") == TRUE ) {
		GPRSrsp = RSP_CONNECT;
	}

	if ( (systemVars.debugLevel & D_GPRS) != 0) {
		tickCount = xTaskGetTickCount();
		snprintf_P( gprs_printfBuff,CHAR256,PSTR(".[%06lu] tkGprs::gTR77 Rsp=\r\n\0"),tickCount  );
		TERMrprintfStr( gprs_printfBuff );
		TERMrprintfStr( getGprsRxBufferPtr() );
		TERMrprintfCRLF();

	}

	pvGPRS_printExitMsg(77);
	return(gSST_ONdata_dummyS01);

}
/*------------------------------------------------------------------------------------*/
static int gac78_fn(void)
{
	// gSST_ONdata_dummyS01 -> gSST_ONdata_dummyS02

	pvGPRS_printExitMsg(78);
	return(gSST_ONdata_dummyS02);

}
/*------------------------------------------------------------------------------------*/
static int gac79_fn(void)
{
	// gSST_ONdata_dummyS01 -> gSST_ONdata_data

	// El socket abrio.
	pvGPRS_printExitMsg(79);
	return(gSST_ONdata_data);

}
/*------------------------------------------------------------------------------------*/
static int gac80_fn(void)
{
	// gSST_ONdata_dummyS02 -> gSST_ONdata_socket

	if (pTryes > 0 ) {
		pTryes--;
	}
	ac_gprsTimer1(START_GPRST1, 2, (s08) NULL );

	pvGPRS_printExitMsg(80);
	return(gSST_ONdata_socket);
}
/*------------------------------------------------------------------------------------*/
static int gac81_fn(void)
{
	// gSST_ONdata_dummyS02 -> gSST_ONdata_dummyS03

	pvGPRS_printExitMsg(81);
	return(gSST_ONdata_dummyS03);

}
/*------------------------------------------------------------------------------------*/
static int gac82_fn(void)
{

	// gSST_ONdata_dummyS03 -> gSST_ONdata_socket
	if ( sockTryes > 0 ) {
		sockTryes--;
	}
	pTryes = 8;
	ac_gprsTimer1(START_GPRST1, 2, (s08) NULL );

	// OPEN SOCKET.
	pvGPRS_openSocket();

	if ( (systemVars.debugLevel & D_GPRS) != 0) {
		tickCount = xTaskGetTickCount();
		snprintf_P( gprs_printfBuff,CHAR256,PSTR(".[%06lu] tkGprs::gTR82 RETRY OPEN SOCKET (%d)\r\n\0"),tickCount, sockTryes);
		TERMrprintfStr( gprs_printfBuff );
	}

	pvGPRS_printExitMsg(82);
	return(gSST_ONdata_socket);

}
/*------------------------------------------------------------------------------------*/
static int gac83_fn(void)
{
	// gSST_ONdata_dummyS03 -> EXIT

	tkGprs_state = gST_OFF;
	pvGPRS_printExitMsg(83);
	return(gSST_OFF_Entry);

}
/*------------------------------------------------------------------------------------*/
static int gac84_fn(void)
{
	// gSST_ONdata_dummy02 ->  gSST_ONdata_frame
	// El socket esta abierto.
	// Solo envio el header
	// Send Data Frame
	// GET /cgi-bin/sp5K/sp5K.pl?DLGID=SPY001&PASSWD=spymovil123

u16 pos = 0;

	nroFrameInTxWindow = 0;
	f_sendTail = FALSE;

	// HEADER:
	memset( gprs_printfBuff, '\0', sizeof(gprs_printfBuff));
	pos = snprintf_P( gprs_printfBuff,( CHAR256 - pos ),PSTR("GET " ));
	pos += snprintf_P( &gprs_printfBuff[pos],( CHAR256 - pos ),PSTR("%s"), systemVars.serverScript );
	pos += snprintf_P( &gprs_printfBuff[pos],( CHAR256 - pos ),PSTR("?DLGID=%s"), systemVars.dlgId );
	pos += snprintf_P( &gprs_printfBuff[pos],( CHAR256 - pos ),PSTR("&PASSWD=%s"), systemVars.passwd );

	// Trasmito parcial
	pvGPRS_sendCmd (gprs_printfBuff, FALSE, TRUE);
	if ( (systemVars.debugLevel & D_GPRS) != 0) {
		TERMrprintfStr( getGprsRxBufferPtr() );
		TERMrprintfCRLF();
	}
	taskYIELD();

	if ( (systemVars.debugLevel & D_GPRS) != 0) {
		tickCount = xTaskGetTickCount();
		snprintf_P( gprs_printfBuff,CHAR256,PSTR(".[%06lu] tkGprs::gTR84 (header)\r\n\0"),tickCount);
		TERMrprintfStr( gprs_printfBuff );
	}

	pvGPRS_printExitMsg(84);
	return(gSST_ONdata_frame);

}
/*------------------------------------------------------------------------------------*/
static int gac85_fn(void)
{
	// gSST_ONdata_frame ->  gSST_ONdata_dummyF01
	// Solo envio el Tail

u08 pos = 0;
s08 retS;

	// TAIL ( No mando el close) :
	memset( gprs_printfBuff, '\0', sizeof(gprs_printfBuff));
	pos = snprintf_P( gprs_printfBuff, ( CHAR256 - pos ),PSTR(" HTTP/1.1\n") );
	pos += snprintf_P( &gprs_printfBuff[pos], ( CHAR256 - pos ),PSTR("Host: www.spymovil.com\n" ));
	pos += snprintf_P( &gprs_printfBuff[pos], ( CHAR256 - pos ),PSTR("\n\n" ));

	// Trasmito
	pvGPRS_sendCmd (gprs_printfBuff, FALSE, TRUE);
	if ( (systemVars.debugLevel & D_GPRS) != 0) {
		TERMrprintfStr( gprs_printfBuff );
	}
	taskYIELD();

	// Fin del Frame
	//pvGPRS_sendCmd ("\n\n", TRUE, TRUE);

	// Vamos a esperar en forma persistente hasta 30s la respuesta del server al frame.
	pTryes = 6;
	ac_gprsTimer1(START_GPRST1, 5, &retS );

	pvGPRS_printExitMsg(85);
	return(gSST_ONdata_dummyF01);

}
/*------------------------------------------------------------------------------------*/
static int gac86_fn(void)
{
	// gSST_ONdata_frame ->  gSST_ONdata_frame
	// El socket esta abierto.
	// Leo la BD y trasmito el registro.
	// Evaluo la flag de envio del tail.

u16 pos = 0;
u08 channel;
u08 eeRdMode;
s08 retS;

static u16 rcdIndex = 0;

	// Si es el primer frame del window, leo desde le ppio.
	eeRdMode = nroFrameInTxWindow;

	// Leo un registro
	rcdIndex = MEM_getRdPtr();		// indice del registro que estoy leyendo.
	retS = MEM_read( &rdFrame, sizeof(rdFrame), eeRdMode );
	nroFrameInTxWindow++;

	switch(retS) {
	case MEM_EMPTY: // BD vacia
		if ( (systemVars.debugLevel & D_GPRS) != 0) {
			tickCount = xTaskGetTickCount();
			snprintf_P( gprs_printfBuff,CHAR256,PSTR(".[%06lu] tkGprs::gTR86 BD empty4read\r\n\0"),tickCount);
			TERMrprintfStr( gprs_printfBuff );
		}
		f_sendTail = TRUE;
		goto quit86;
		break;
	case MEM_OK: 	// Lectura correcta
		break;
	default: 		// Errores
		if ( (systemVars.debugLevel & D_GPRS) != 0) {
			tickCount = xTaskGetTickCount();
			snprintf_P( gprs_printfBuff,CHAR256,PSTR(".[%06lu] tkGprs::gTR86 BD Error (%d)\r\n\0"),tickCount, retS);
			TERMrprintfStr( gprs_printfBuff );
		}
		f_sendTail = TRUE;
		goto quit86;
		break;
	}

	// Lectura correcta: Trasmito
	// Data Frame:
	memset( gprs_printfBuff, '\0', sizeof(gprs_printfBuff));
	pos = snprintf_P( gprs_printfBuff,( CHAR256 - pos ),PSTR("&CTL=%d"), rcdIndex );
	pos += snprintf_P( &gprs_printfBuff[pos],( CHAR256 - pos ),PSTR("&LINE=") );

	pos += snprintf_P( &gprs_printfBuff[pos],( CHAR256 - pos ),PSTR( "%04d%02d%02d,"),rdFrame.rtc.year,rdFrame.rtc.month,rdFrame.rtc.day );
	pos += snprintf_P( &gprs_printfBuff[pos],( CHAR256 - pos ), PSTR("%02d%02d%02d,"),rdFrame.rtc.hour,rdFrame.rtc.min, rdFrame.rtc.sec );
	// Valores analogicos
	for ( channel = 0; channel < NRO_CHANNELS; channel++) {
		pos += snprintf_P( &gprs_printfBuff[pos],( CHAR256 - pos ),PSTR("%s>%.3f,"),systemVars.aChName[channel],rdFrame.analogIn[channel] );
	}
	// Valores digitales
	pos += snprintf_P( &gprs_printfBuff[pos],( CHAR256 - pos ), PSTR("%sP>%.2f,%sL>%d,%sW>%d,"), systemVars.dChName[0],rdFrame.dIn.din0_pulses,systemVars.dChName[0],rdFrame.dIn.din0_level,systemVars.dChName[0], rdFrame.dIn.din0_width);
	pos += snprintf_P( &gprs_printfBuff[pos],( CHAR256 - pos ), PSTR("%sP>%.2f,%sL>%d,%sW>%d,"), systemVars.dChName[1],rdFrame.dIn.din1_pulses,systemVars.dChName[1],rdFrame.dIn.din1_level,systemVars.dChName[1], rdFrame.dIn.din1_width);

	// Salidas
	pos += snprintf_P( &gprs_printfBuff[pos],( CHAR256 - pos ), PSTR("O0>%d,O1>%d,O2>%d,O3>%d,"), systemVars.outputs.dout[0], systemVars.outputs.dout[1], systemVars.outputs.dout[2], systemVars.outputs.dout[3] );
	// Bateria
	pos += snprintf_P( &gprs_printfBuff[pos],( CHAR256 - pos ), PSTR("bt>%.2f"),rdFrame.batt );

	// Trasmito parcial & debug
	pvGPRS_sendCmd (gprs_printfBuff, FALSE, TRUE);
	if ( (systemVars.debugLevel & D_GPRS) != 0) {
		TERMrprintfStr( gprs_printfBuff );
		//TERMrprintfStr( getGprsRxBufferPtr() );
		TERMrprintfCRLF();
	}

	// Status de la memoria
	if ( (systemVars.debugLevel & D_GPRS) != 0) {
		tickCount = xTaskGetTickCount();
		snprintf_P( gprs_printfBuff,CHAR256,PSTR(".[%06lu] tkGprs::gTR86 memStatus: pWr=%d,pRd=%d,pDel=%d,free=%d,4rd=%d,4del=%d  \r\n"),tickCount, MEM_getWrPtr(), MEM_getRdPtr(), MEM_getDELptr(), MEM_getRcdsFree(),MEM_getRcds4rd(),MEM_getRcds4del() );
		TERMrprintfStr( gprs_printfBuff );
	}
	// ultimo frame del dataWindow ?
	if ( nroFrameInTxWindow == MAX_TXFRM4WINDOW ) {
		f_sendTail = TRUE;
	}

	taskYIELD();

quit86:

	pvGPRS_printExitMsg(86);
	return(gSST_ONdata_frame);

}
/*------------------------------------------------------------------------------------*/
static int gac87_fn(void)
{
	// gSST_ONdata_dummyF01 ->  gSST_ONdata_dummyF02

	// Evaluo si llego una respuesta del server al frame de datos.
	if ( pvGPRS_RspIs("RX_OK") == TRUE ) {
		GPRSrsp = RSP_RXOK;
	}

	if ( (systemVars.debugLevel & D_GPRS) != 0) {
		tickCount = xTaskGetTickCount();
		snprintf_P( gprs_printfBuff,CHAR256,PSTR(".[%06lu] tkGprs::gTR87 Rsp=\r\n\0"),tickCount  );
		TERMrprintfStr( gprs_printfBuff );
		TERMrprintfStr( getGprsRxBufferPtr() );
		TERMrprintfCRLF();

	}

	pvGPRS_printExitMsg(87);
	return(gSST_ONdata_dummyF02);

}
/*------------------------------------------------------------------------------------*/
static int gac88_fn(void)
{
	// gSST_ONdata_dummyF02 ->  gSST_ONdata_await02

	pvGPRS_analizeRXframe();

	// Espero 10s entre windows o que cierre el socket
	pvGPRS_closeSocket();
	ac_gprsTimer1(START_GPRST1, 10, (s08) NULL );

	pvGPRS_printExitMsg(88);
	return(gSST_ONdata_await02);

}
/*------------------------------------------------------------------------------------*/
static int gac89_fn(void)
{
	// gSST_ONdata_dummyF02 -> gSST_ONdata_dummyF03

	pvGPRS_printExitMsg(89);
	return(gSST_ONdata_dummyF03);

}
/*------------------------------------------------------------------------------------*/
static int gac90_fn(void)
{
	// gSST_ONdata_dummyF03 -> gSST_ONdata_dummyF04

	pvGPRS_printExitMsg(90);
	return(gSST_ONdata_dummyF04);

}
/*------------------------------------------------------------------------------------*/
static int gac91_fn(void)
{
	// gSST_ONdata_dummyF04 -> gSST_ONdata_data

	pvGPRS_printExitMsg(91);
	return(gSST_ONdata_data);

}
/*------------------------------------------------------------------------------------*/
static int gac92_fn(void)
{
	// gSST_ONdata_dummyF03 -> gSST_ONdata_dummyF01

	if (pTryes > 0 ) {
		pTryes--;
	}
	ac_gprsTimer1(START_GPRST1, 5, (s08) NULL );

	pvGPRS_printExitMsg(92);
	return(gSST_ONdata_dummyF01);

}
/*------------------------------------------------------------------------------------*/
static int gac93_fn(void)
{
	// gSST_ONdata_dummyF04 -> EXIT

	tkGprs_state = gST_OFF;

	pvGPRS_printExitMsg(93);
	return(gSST_OFF_Entry);

}
/*------------------------------------------------------------------------------------*/
static int gac94_fn(void)
{
	// gSST_ONdata_dummyF3 -> gSST_ONdata_data

	pvGPRS_printExitMsg(96);

	return(gSST_ONdata_data);

}
/*------------------------------------------------------------------------------------*/
static int gac95_fn(void)
{
	// gSST_ONdata_dummy02 -> EXIT

	tkGprs_state = gST_OFF;
	pvGPRS_printExitMsg(95);
	return(gSST_OFF_Entry);

}
/*------------------------------------------------------------------------------------*/
static void pvGPRS_printExitMsg(u08 code)
{
	// Fijo el tiempo maximo a estar en el sigiente estado.
	// e imprimo un mensaje de salida.
	switch (code) {

	case 1:
		counterCtlState = 864000;	// 1 dia de espera maxima en estado IDLE
		break;
	case 3:
		counterCtlState = getTimeToNextDial() * 10 * 1.1;
		break;
	case 16:
		counterCtlState = getTimeToNextDial() * 10 * 1.1;
		break;
	default:
		counterCtlState =  MAXTIMEINASTATE; // 3 minutos por default.
		break;
	}

	if ( (systemVars.debugLevel & D_GPRS) != 0) {
		tickCount = xTaskGetTickCount();
		snprintf_P( gprs_printfBuff,CHAR256,PSTR(".[%06lu] tkGprs::exit TR%02d\r\n"), tickCount,code);
		TERMrprintfStr(  gprs_printfBuff );
	}
}
//------------------------------------------------------------------------------------

// TIMER GENERAL

static s08 ac_gprsTimer1(t_GprsTimer1 action, u16 secs, s08 *retStatus)
{
// Timer de uso general, configurado para modo one-shot.

static s08 f_GPRSt1 = FALSE;
s08 ret;

	switch (action ) {
	case INIT_GPRST1:
		// Configuro el timer pero no lo prendo
		ret = sTimerCreate(FALSE,  gprsT1_TimerCallBack , &xTimer_gprsT1 );
		ret = sTimerChangePeriod(xTimer_gprsT1, 5 );
		break;

	case CALLBACK_GPRST1:
		f_GPRSt1 = TRUE;
		ret = TRUE;

		break;

	case START_GPRST1:
		// Arranco el timer que me va a sacar de este modo
		f_GPRSt1 = FALSE;
		ret = sTimerChangePeriod(xTimer_gprsT1, (u32)(secs));
		ret = sTimerRestart(xTimer_gprsT1);
		break;

	case STOP_GPRST1:
		f_GPRSt1 = FALSE;
		ret = sTimerStop(xTimer_gprsT1);
		break;

	case  EXPIRED_GPRST1:
		break;
	}

	*retStatus = ret;
	return(f_GPRSt1);
}

void gprsT1_TimerCallBack ( void )
{
	ac_gprsTimer1(CALLBACK_GPRST1, NULL , (s08) NULL);
}

//------------------------------------------------------------------------------------
// TIMER DIAL

static s08 ac_gprsTimerDial(t_GprsTimerDial action, u32 secs)
{
// Timer para marcar cuando debo discar, configurado para modo one-shot.

static s08 f_GPRSdial = FALSE;

	switch (action ) {
	case INIT_GPRSTd:
		// Configuro el timer pero no lo prendo
		sTimerCreate(FALSE,  gprsTDial_TimerCallBack , &xTimer_gprsTDial );
		sTimerChangePeriod(xTimer_gprsTDial, 5 );
		break;

	case CALLBACK_GPRSTd:
		f_GPRSdial = TRUE;
		break;

	case START_GPRSTd:
		// Arranco el timer que me va a sacar de este modo
		f_GPRSdial = FALSE;
		sTimerChangePeriod(xTimer_gprsTDial, secs );
		sTimerRestart(xTimer_gprsTDial);
		break;

	case STOP_GPRSTd:
		f_GPRSdial = FALSE;
		sTimerStop(xTimer_gprsTDial);
		break;

	case  EXPIRED_GPRSTd:
		return(f_GPRSdial);
		break;
	}

	return(f_GPRSdial);
}

void gprsTDial_TimerCallBack ( void )
{
	ac_gprsTimerDial(CALLBACK_GPRSTd, NULL);
}

s32 getTimeToNextDial(void)
{
s32 retVal = 0;

	// Lo determina en base al time elapsed y el timerPoll.

	if ( systemVars.wrkMode == WK_NORMAL ) {
		retVal = sTimerTimeRemains(xTimer_gprsTDial) ;
	}
	return (retVal);
}

//------------------------------------------------------------------------------------
static s08 pvGPRS_RspIs(const char *rsp)
{
s08 retS = FALSE;

	if (strstr(  getGprsRxBufferPtr(), rsp) != NULL) {
		retS = TRUE;
	}
	return(retS);
}
//------------------------------------------------------------------------------------
static void pvGPRS_sendCmd(char *s, const int clearbuff , const int partialFrame )
{
	/* Utlilizo esta funcion ya que como el modem tiene echo y no lo quiero
	 * deshabilitar, cada comando me llena el buffer de trasmision entonces
	 * de este modo, lo borro antes del CR
	 * clearbuff = TRUE: hace que borre la cola de recepcion antes del CR.
	 *                   Con esto borro el echo.
	 * partialFrame = TRUE: Indica que el frame a enviar es mas largo asi que no debo
	 *                      poner el CR.
	 */

	GPRSrsp = RSP_NONE;
	xUart0QueueFlush();
	rprintfStr(GPRS_UART, s);
	if ( clearbuff ) xUart0QueueFlush();
	if ( ! partialFrame ) rprintfCRLF( GPRS_UART);

}
/*------------------------------------------------------------------------------------*/

static void pvGPRS_openSocket(void)
{

	snprintf_P( gprs_printfBuff,CHAR256,PSTR("AT*E2IPO=1,\"%s\",%s"),systemVars.serverAddress,systemVars.serverPort);
	pvGPRS_sendCmd (gprs_printfBuff, FALSE, FALSE );

}
/*------------------------------------------------------------------------------------*/

static void pvGPRS_sendInitFrame(void)
{
// Send Data Frame
// GET /cgi-bin/sp5K/sp5K.pl?DLGID=SPY001&PASSWD=spymovil123&&INIT&PWRM=CONT&TPOLL=23&TDIAL=234&PWRS=1,1230,2045&A0=pZ,1,20,3,10&D0=qE,3.24&OUT=1,1,0,0,1,1234,927,1,3 HTTP/1.1
// Host: www.spymovil.com
// Connection: close

u16 pos = 0;
u08 i;

	// Trasmision: 1r.Parte.
	// HEADER:
	memset( gprs_printfBuff, '\0', sizeof(gprs_printfBuff));
	pos = snprintf_P( gprs_printfBuff,CHAR256,PSTR("GET " ));
	pos += snprintf_P( &gprs_printfBuff[pos],( CHAR256 - pos ),PSTR("%s"), systemVars.serverScript );
	pos += snprintf_P( &gprs_printfBuff[pos],( CHAR256 - pos ),PSTR("?DLGID=%s"), systemVars.dlgId );
	pos += snprintf_P( &gprs_printfBuff[pos],( CHAR256 - pos ),PSTR("&PASSWD=%s"), systemVars.passwd );
	pos += snprintf_P( &gprs_printfBuff[pos],( CHAR256 - pos ),PSTR("&VER=%s"), SP5K_REV );

	pvGPRS_sendCmd (gprs_printfBuff, FALSE, TRUE );		// Envio parcial ( no CR )
	if ( (systemVars.debugLevel & D_GPRS) != 0) {
		TERMrprintfStr( gprs_printfBuff );
		TERMrprintfCRLF();
	}

	// BODY ( 1a parte) :
	memset(gprs_printfBuff, '\0', sizeof(gprs_printfBuff));
	pos = snprintf_P( gprs_printfBuff ,CHAR256,PSTR("&INIT"));
	// timerpoll
	pos += snprintf_P( &gprs_printfBuff[pos],( CHAR256 - pos ),PSTR("&TPOLL=%d"), systemVars.timerPoll);
	// timerdial
	pos += snprintf_P( &gprs_printfBuff[pos],( CHAR256 - pos ),PSTR("&TDIAL=%d"), systemVars.timerDial);
	// pwrMode
	if ( systemVars.pwrMode == PWR_CONTINUO) { pos += snprintf_P( &gprs_printfBuff[pos],( CHAR256 - pos ),PSTR("&PWRM=CONT")); }
	if ( systemVars.pwrMode == PWR_DISCRETO) { pos += snprintf_P( &gprs_printfBuff[pos],( CHAR256 - pos ),PSTR("&PWRM=DISC")); }
	// pwrSave
	pos += snprintf_P( &gprs_printfBuff[pos],( CHAR256 - pos ),PSTR("&PWRS=%d,%d,%d"),systemVars.pwrSave, pv_convertMINS2hhmm( systemVars.pwrSaveStartTime ), pv_convertMINS2hhmm( systemVars.pwrSaveEndTime) );
	// csq
	pos += snprintf_P( &gprs_printfBuff[pos],( CHAR256 - pos ),PSTR("&CSQ=%d"), systemVars.csq);

	pvGPRS_sendCmd (gprs_printfBuff, FALSE, TRUE );		// Envio parcial ( no CR )
	if ( (systemVars.debugLevel & D_GPRS) != 0) {
		TERMrprintfStr( gprs_printfBuff );
		TERMrprintfCRLF();
	}

	// BODY ( 2a parte) :
	memset(gprs_printfBuff, '\0', sizeof(gprs_printfBuff));
	pos = 0;
	// Configuracion de canales analogicos
	for ( i = 0; i < NRO_CHANNELS; i++) {
		pos += snprintf_P( &gprs_printfBuff[pos],( CHAR256 - pos ),PSTR("&A%d=%s,%d,%d,%d,%.1f"), i,systemVars.aChName[i],systemVars.Imin[i], systemVars.Imax[i], systemVars.Mmin[i], systemVars.Mmax[i]);
	}
	// Configuracion de canales digitales
	pos += snprintf_P( &gprs_printfBuff[pos],( CHAR256 - pos ),PSTR("&D0=%s,%.2f"),systemVars.dChName[0],systemVars.magPP[0]);
	pos += snprintf_P( &gprs_printfBuff[pos],( CHAR256 - pos ),PSTR("&D1=%s,%.2f"),systemVars.dChName[1],systemVars.magPP[1]);
	// Configuracion de salidas
	pos += snprintf_P( &gprs_printfBuff[pos],( CHAR256 - pos ),PSTR("&OUT=%d,%d,%d,%d,%d,"),systemVars.outputs.wrkMode,systemVars.outputs.dout[0],systemVars.outputs.dout[1],systemVars.outputs.dout[2],systemVars.outputs.dout[3]);
	pos += snprintf_P( &gprs_printfBuff[pos],( CHAR256 - pos ),PSTR("%d,%d,"),pv_convertMINS2hhmm(systemVars.outputs.horaConsDia),pv_convertMINS2hhmm(systemVars.outputs.horaConsNoc));
	pos += snprintf_P( &gprs_printfBuff[pos],( CHAR256 - pos ),PSTR("%d,%d"),systemVars.outputs.chVA,systemVars.outputs.chVB );
	pvGPRS_sendCmd (gprs_printfBuff, FALSE, TRUE );		// Envio parcial ( no CR )
	if ( (systemVars.debugLevel & D_GPRS) != 0) {
		TERMrprintfStr( gprs_printfBuff );
		TERMrprintfCRLF();
	}

	// TAIL ( No mando el close):
	memset(gprs_printfBuff, '\0', sizeof(gprs_printfBuff));
	pos = snprintf_P( gprs_printfBuff, ( CHAR256 - pos ),PSTR(" HTTP/1.1\n") );
	pos += snprintf_P( &gprs_printfBuff[pos], ( CHAR256 - pos ),PSTR("Host: www.spymovil.com\n" ));
	//pos += snprintf_P( &gprs_printfBuff[pos], CHAR256,PSTR("\n\n" ));

	// Trasmito
	pvGPRS_sendCmd (gprs_printfBuff, FALSE, TRUE);
	taskYIELD();
	if ( (systemVars.debugLevel & D_GPRS) != 0) {
		TERMrprintfStr( gprs_printfBuff );
		TERMrprintfCRLF();
	}

	// Fin del Frame
	pvGPRS_sendCmd ("\n\n", TRUE, TRUE);

}
/*------------------------------------------------------------------------------------*/
static s08 pvGPRS_analizeRXframe(void)
{
s16 nro;
char localStr[32];
char *stringp = NULL;
char *token;
char *delim = ",=:><";
char *p1,*p2,*p3,*p4;


	// ANALIZO:
	// Lineas comunes de datos: tag: RX_OK:,<h1>RX_OK:7</h1> busco el nro. de linea para su borrado.
	// Es la confirmacion que el frame llego OK al server.
	// Determino el nro.de linea ACK
	// !! Borro todo el ultimo window frame, sin importar el nro de linea ack por el server !!!!
	stringp = strstr(getGprsRxBufferPtr(), "RX_OK:");
	if ( stringp != NULL ) {
		nro = atoi(stringp);
		if ( nro >= 0 ) {
			while ( MEM_getRcds4del() > MEM_getRcds4rd() ) {
				MEM_erase();
			}

			// reinicio el contador para habilitar mas trasmisiones de windows.
			TxWinTryes = 0;
			if ( (systemVars.debugLevel & D_GPRS) != 0) {
				tickCount = xTaskGetTickCount();
				snprintf_P( gprs_printfBuff,CHAR256,PSTR("\n.[%06lu] tkGprs::gTR88 memStatus: pWr=%d,pRd=%d,pDel=%d,free=%d,4rd=%d,4del=%d  \r\n"),tickCount, MEM_getWrPtr(), MEM_getRdPtr(), MEM_getDELptr(), MEM_getRcdsFree(),MEM_getRcds4rd(),MEM_getRcds4del() );
				TERMrprintfStr( gprs_printfBuff );
			}
		}
	}

	// EL server puede mandar reconfigurar las salidas.
	// Lineas comunes de datos: tag: RX_OK:,<h1>RX_OK:7:OUT=1,0,1,1</h1>.
	stringp = NULL;
	stringp = strstr(getGprsRxBufferPtr(), "OUT=");
	if ( stringp != NULL ) {
		// Copio el mensaje enviado a un buffer local porque la funcion strsep lo modifica.
		memcpy(localStr,'\0',32);
		memcpy(localStr,stringp,31);

		stringp = localStr;
		token = strsep(&stringp,delim);	//OUT
		p1 = strsep(&stringp,delim);	// o0
		p2 = strsep(&stringp,delim);	// o1
		p3 = strsep(&stringp,delim);	// o2
		p4 = strsep(&stringp,delim);	// o3

		// Si estoy en modo manual, reconfiguro las salidas
		if ( systemVars.outputs.wrkMode == OUTPUT_MANUAL ) {
			pb_setParamOutputs(systemVars.outputs.wrkMode,p1,p2,p3,p4,NULL,NULL,NULL,NULL);
		}
	}

	// Si el server mando un RESET, debo salir y volver a reconectarme para actualizar la configuracion
	stringp = NULL;
	stringp = strstr(getGprsRxBufferPtr(), "RESET");
	if ( stringp != NULL) {
		if ( (systemVars.debugLevel & D_GPRS) != 0) {
			tickCount = xTaskGetTickCount();
			snprintf_P( gprs_printfBuff,CHAR256,PSTR("\n.[%06lu] tkGprs::gTR88 RESET \r\n"),tickCount );
			TERMrprintfStr( gprs_printfBuff );
		}
		// Reseteo el sistema
		cli();
		wdt_reset();
		/* Start timed equence */
		WDTCSR |= (1<<WDCE) | (1<<WDE);
		/* Set new prescaler(time-out) value = 64K cycles (~0.5 s) */
		WDTCSR = (1<<WDE) | (1<<WDP2) | (1<<WDP0);
		sei();

		wdt_enable(WDTO_30MS);
		while(1) {}
	}
}
/*------------------------------------------------------------------------------------*/
static s08 pvGPRS_qySocketIsOpen(void)
{
s08 socketStatus;

	if ( systemVars.dcd == 0 ) {
		// dcd=ON
		socketStatus = TRUE;
	} else {
		// dcd OFF
		socketStatus = FALSE;
	}

	return(socketStatus);
}
/*------------------------------------------------------------------------------------*/
static void pvGPRS_closeSocket(void)
{
	// Cierro el socket

	// Deshabilito los mensajes
	GPRSrsp = RSP_NONE;
	xUart0QueueFlush();
	rprintfStr(GPRS_UART, "+++AT\r\n");
	vTaskDelay( (portTickType)( 500 / portTICK_RATE_MS ) );

}
/*------------------------------------------------------------------------------------*/
static void pvGPRS_processServerClock(void)
{
/* Extraigo el srv clock del string mandado por el server y si el drift con la hora loca
 * es mayor a 5 minutos, ajusto la hora local
 * La linea recibida es del tipo: <h1>INIT_OK:CLOCK=1402251122:PWRM=DISC:</h1>
 *
 */

char *ptr;
unsigned long serverDate, localDate;
char clkdata[12];
s08 retS = FALSE;
RtcTimeType clock;

	ptr = strstr(getGprsRxBufferPtr(), "CLOCK");
	if ( ptr == NULL ) {
		return;
	}
	// Incremento para que apunte al str.con la hora.
	ptr = ptr + 6;
	serverDate = atol(ptr);

	rtcGetTime(&clock);
	clock.year -= 2000;
	snprintf_P( clkdata,CHAR256,PSTR("%02d%02d%02d%02d%02d\0"), clock.year,clock.month, clock.day,clock.hour,clock.min);
	localDate = atol(clkdata);

	if ( (systemVars.debugLevel & D_GPRS) != 0) {
		tickCount = xTaskGetTickCount();
		snprintf_P( gprs_printfBuff,CHAR256,PSTR(".[%06lu] tkGprs::processServerClock:: SrvTime: %lu, LocalTime: %lu\r\n\0"),tickCount, serverDate, localDate);
		TERMrprintfStr( gprs_printfBuff );
	}

	if ( abs( serverDate - localDate ) > 5 ) {
		clkdata[0] = *ptr++; clkdata[1] = *ptr++; clkdata[2] = '\0'; clock.year = atoi(clkdata);
		clkdata[0] = *ptr++; clkdata[1] = *ptr++; clkdata[2] = '\0'; clock.month = atoi(clkdata);
		clkdata[0] = *ptr++; clkdata[1] = *ptr++; clkdata[2] = '\0'; clock.day = atoi(clkdata);
		clkdata[0] = *ptr++; clkdata[1] = *ptr++; clkdata[2] = '\0'; clock.hour = atoi(clkdata);
		clkdata[0] = *ptr++; clkdata[1] = *ptr++; clkdata[2] = '\0'; clock.min = atoi(clkdata);
		clock.sec = 0;

		retS = rtcSetTime(&clock);
		if ( (systemVars.debugLevel & D_GPRS) != 0) {
			tickCount = xTaskGetTickCount();
			if (retS ) {
				snprintf_P( gprs_printfBuff,CHAR256,PSTR(".[%06lu] tkGprs::processServerClock:: Rtc update OK\r\n\0"),tickCount);
			} else {
				snprintf_P( gprs_printfBuff,CHAR256,PSTR(".[%06lu] tkGprs::processServerClock:: Rtc update FAIL\r\n\0"),tickCount);
			}
			TERMrprintfStr( gprs_printfBuff );
		}
	}

}
/*------------------------------------------------------------------------------------*/
static u08 pvGPRS_processPwrMode(void)
{

u08 ret = 0;

	tickCount = xTaskGetTickCount();
	snprintf_P( gprs_printfBuff,CHAR256,PSTR(".[%06lu] tkGprs::processPwrMode:: WARN NO PWRM parameter\r\n\0"),tickCount);

	if (strstr( getGprsRxBufferPtr(), "PWRM=DISC") != NULL ) {
		pb_setParamPwrMode(PWR_DISCRETO);
		ret = 1;
		snprintf_P( gprs_printfBuff,CHAR256,PSTR(".[%06lu] tkGprs::processPwrMode:: Reconfig PWRM to DISC\r\n\0"),tickCount);
	}

	if (strstr( getGprsRxBufferPtr(), "PWRM=CONT") != NULL ) {
		pb_setParamPwrMode(PWR_CONTINUO);
		ret = 1;
		snprintf_P( gprs_printfBuff,CHAR256,PSTR(".[%06lu] tkGprs::processPwrMode:: Reconfig PWRM to CONT\r\n\0"),tickCount);
	}

	if ( (systemVars.debugLevel & D_GPRS) != 0) {
		TERMrprintfStr( gprs_printfBuff );
	}

	return(ret);
}
/*------------------------------------------------------------------------------------*/
static u08 pvGPRS_processTimerPoll(void)
{

//	La linea recibida es del tipo: <h1>INIT_OK:CLOCK=1402251122:TPOLL=600:PWRM=DISC:</h1>

u08 ret = 0;
char localStr[32];
char *stringp;
char *token;
char *delim = ",=:><";

	tickCount = xTaskGetTickCount();
	stringp = strstr(getGprsRxBufferPtr(), "TPOLL");
	if ( stringp == NULL ) {
		ret = 0;
		snprintf_P( gprs_printfBuff,CHAR256,PSTR(".[%06lu] tkGprs::processTimerPoll:: WARN NO TPOLL parameter\r\n\0"),tickCount);
		goto quit;
	}

	// Copio el mensaje enviado a un buffer local porque la funcion strsep lo modifica.
	memcpy(localStr,'\0',32);
	memcpy(localStr,stringp,31);

	stringp = localStr;
	token = strsep(&stringp,delim);	// TPOLL

	token = strsep(&stringp,delim);	// timerPoll
	pb_setParamTimerPoll(token);
	ret = 1;
	snprintf_P( gprs_printfBuff,CHAR256,PSTR(".[%06lu] tkGprs::processTimerPoll:: Reconfig TPOLL\r\n\0"),tickCount);

quit:
	if ( (systemVars.debugLevel & D_GPRS) != 0) {
		TERMrprintfStr( gprs_printfBuff );
	}

	return(ret);
}
/*------------------------------------------------------------------------------------*/
static u08 pvGPRS_processTimerDial(void)
{

//	La linea recibida es del tipo: <h1>INIT_OK:CLOCK=1402251122:TPOLL=600:TDIAL=10300:PWRM=DISC:CD=1230:CN=0530</h1>

u08 ret = 0;
char localStr[32];
char *stringp;
char *token;
char *delim = ",=:><";

	tickCount = xTaskGetTickCount();
	stringp = strstr(getGprsRxBufferPtr(), "TDIAL");
	if ( stringp == NULL ) {
		ret = 0;
		snprintf_P( gprs_printfBuff,CHAR256,PSTR(".[%06lu] tkGprs::processTimerDial:: WARN NO TDIAL parameter\r\n\0"),tickCount);
		goto quit;
	}

	// Copio el mensaje enviado a un buffer local porque la funcion strsep lo modifica.
	memcpy(localStr,'\0',32);
	memcpy(localStr,stringp,31);

	stringp = localStr;
	token = strsep(&stringp,delim);	// TDIAL

	token = strsep(&stringp,delim);	// timerDial
	pb_setParamTimerDial(token);
	ret = 1;
	snprintf_P( gprs_printfBuff,CHAR256,PSTR(".[%06lu] tkGprs::processTimerDial:: Reconfig TDIAL\r\n\0"),tickCount);

quit:
	if ( (systemVars.debugLevel & D_GPRS) != 0) {
		TERMrprintfStr( gprs_printfBuff );
	}

	return(ret);

}
/*------------------------------------------------------------------------------------*/
static u08 pvGPRS_processPwrSave(void)
{
	//	La linea recibida es del tipo:
	//	<h1>INIT_OK:CLOCK=1402251122:TPOLL=600:TDIAL=10300:PWRS=1,2230,0600:DISC:A0=pA,0,20,0,6:A1=pB,0,20,0,10:A2=pC,0,20,0,10:D0=q0,1.00:D1=q1,1.00</h1>
	//  Las horas estan en formato HHMM.

u08 ret = 0;
char localStr[32];
char *stringp;
char *token;
char *delim = ",=:><";
char *p1,*p2;
u08 modo;

	tickCount = xTaskGetTickCount();
	// Vemos si el sp5K mando configuracion de pwrs
	stringp = strstr(getGprsRxBufferPtr(), "PWRS");
	if ( stringp == NULL ) {
		ret = 0;
		snprintf_P( gprs_printfBuff,CHAR256,PSTR(".[%06lu] tkGprs::processPwrSave:: WARN NO PWRS parameter\r\n\0"),tickCount);
		goto quit;
	}

	// Copio el mensaje enviado a un buffer local porque la funcion strsep lo modifica.
	memcpy(localStr,'\0',32);
	memcpy(localStr,stringp,31);

	stringp = localStr;
	token = strsep(&stringp,delim);	//PWRS

	token = strsep(&stringp,delim);	// modo
	modo = atoi(token);

	p1 = strsep(&stringp,delim);	// startTime
	p2 = strsep(&stringp,delim); // endTime

	pb_setParamPwrSave(modo, p1, p2);
	snprintf_P( gprs_printfBuff,CHAR256,PSTR(".[%06lu] tkGprs::processPwrSave:: Reconfig PWRS\r\n\0"),tickCount );
	ret = 1;

quit:
	if ( (systemVars.debugLevel & D_GPRS) != 0) {
		TERMrprintfStr( gprs_printfBuff );
	}

	return(ret);

}
/*------------------------------------------------------------------------------------*/
static u08 pvGPRS_processAnalogChannels(u08 channel)
{
//	La linea recibida es del tipo:
//	<h1>INIT_OK:CLOCK=1402251122:TPOLL=600:TDIAL=10300:PWRM=DISC:A0=pA,0,20,0,6:A1=pB,0,20,0,10:A2=pC,0,20,0,10:D0=q0,1.00:D1=q1,1.00</h1>

u08 ret = 0;
char localStr[32];
char *stringp;
char *token;
char *delim = ",=:><";
char *p1,*p2,*p3,*p4,*p5;

	switch (channel) {
	case 0:
		stringp = strstr(getGprsRxBufferPtr(), "A0=");
		break;
	case 1:
		stringp = strstr(getGprsRxBufferPtr(), "A1=");
		break;
	case 2:
		stringp = strstr(getGprsRxBufferPtr(), "A2=");
		break;
	default:
		ret = 0;
		goto quit;
		break;
	}

	if ( stringp == NULL ) {
		ret = 0;
		goto quit;
	}

	// Copio el mensaje enviado a un buffer local porque la funcion strsep lo modifica.
	memcpy(localStr,'\0',32);
	memcpy(localStr,stringp,31);

	stringp = localStr;
	token = strsep(&stringp,delim);	//A0

	p1 = strsep(&stringp,delim);	//name
	p2 = strsep(&stringp,delim);	//iMin
	p3 = strsep(&stringp,delim);	//iMax
	p4 = strsep(&stringp,delim);	//mMin
	p5 = strsep(&stringp,delim);	//mMax

	pb_setParamAnalogCh(channel, p1,p2,p3,p4,p5);
	ret = 1;

	if ( (systemVars.debugLevel & D_GPRS) != 0) {
		tickCount = xTaskGetTickCount();
		snprintf_P( gprs_printfBuff,CHAR256,PSTR(".[%06lu] tkGprs::processAnalogCh: A%d\r\n\0"),tickCount, channel);
		TERMrprintfStr( gprs_printfBuff );
	}

quit:
	return(ret);
}
/*------------------------------------------------------------------------------------*/
static u08 pvGPRS_processDigitalChannels(u08 channel)
{

//	La linea recibida es del tipo:
//	<h1>INIT_OK:CLOCK=1402251122:TPOLL=600:TDIAL=10300:PWRM=DISC:A0=pA,0,20,0,6:A1=pB,0,20,0,10:A2=pC,0,20,0,10:D0=q0,1.00:D1=q1,1.00</h1>

u08 ret = 0;
char localStr[32];
char *stringp;
char *token;
char *delim = ",=:><";
char *p1,*p2;

	switch (channel) {
	case 0:
		stringp = strstr(getGprsRxBufferPtr(), "D0=");
		break;
	case 1:
		stringp = strstr(getGprsRxBufferPtr(), "D1=");
		break;
	default:
		ret = 0;
		goto quit;
		break;
	}

	if ( stringp == NULL ) {
		ret = 0;
		goto quit;
	}

	// Copio el mensaje enviado a un buffer local porque la funcion strsep lo modifica.
	memcpy(localStr,'\0',32);
	memcpy(localStr,stringp,31);

	stringp = localStr;
	token = strsep(&stringp,delim);	//D0

	p1 = strsep(&stringp,delim);	//name
	p2 = strsep(&stringp,delim);	//magPp
	pb_setParamDigitalCh( channel, p1, p2 );
	ret = 1;
	if ( (systemVars.debugLevel & D_GPRS) != 0) {
		tickCount = xTaskGetTickCount();
		snprintf_P( gprs_printfBuff,CHAR256,PSTR(".[%06lu] tkGprs::processDigitalCh: D%d channel\r\n\0"),tickCount, channel );
		TERMrprintfStr( gprs_printfBuff );
	}

quit:

	return(ret);

}
/*------------------------------------------------------------------------------------*/
static u08 pvGPRS_processOuputs(void)
{
	//	La linea recibida es del tipo:
	//	<h1>INIT_OK:CLOCK=1402251122:TPOLL=600:TDIAL=10300:OUT=modo,x,x,x,x,hhmm1,y,y,y,y,hhmm2,z,z,z,z:A0=pA,0,20,0,6:A1=pB,0,20,0,10:A2=pC,0,20,0,10:D0=q0,1.00:D1=q1,1.00</h1>
	// El string de configuracion de las salidas es OUT=0,0,0,0,0,2230,530,0,1:
	// Modo,valores,consigna1, consigna2

u08 ret = 0;
char localStr[32];
char *stringp;
char *token;
char *delim = ",=:><";
char *p1,*p2,*p3,*p4,*p5,*p6,*p7,*p8;
u08 modo;

	tickCount = xTaskGetTickCount();
	stringp = strstr(getGprsRxBufferPtr(), "OUT=");
	if ( stringp == NULL ) {
		snprintf_P( gprs_printfBuff,CHAR256,PSTR(".[%06lu] tkGprs::processOutputs:: WARN NO OUT parameter\r\n\0"),tickCount);
		ret = 0;
		goto quit;
	}

	// Copio el mensaje enviado a un buffer local porque la funcion strsep lo modifica.
	memcpy(localStr,'\0',32);
	memcpy(localStr,stringp,31);

	stringp = localStr;
	token = strsep(&stringp,delim);	//OUT

	token = strsep(&stringp,delim);	// modo
	modo = atoi(token);

	p1 = strsep(&stringp,delim);	// o0
	p2 = strsep(&stringp,delim);	// o1
	p3 = strsep(&stringp,delim);	// o2
	p4 = strsep(&stringp,delim);	// o3
	p5 = strsep(&stringp,delim);	// hhmm1
	p6 = strsep(&stringp,delim);	// hhmm2
	p7 = strsep(&stringp,delim);	// chV1
	p8 = strsep(&stringp,delim);	// chV2

	pb_setParamOutputs(modo,p1,p2,p3,p4,p5,p6,p7,p8 );
	snprintf_P( gprs_printfBuff,CHAR256,PSTR(".[%06lu] tkGprs::processOuputs:: Reconfig OUT \r\n\0"),tickCount);
	ret = 1;

quit:
	if ( (systemVars.debugLevel & D_GPRS) != 0) {
		TERMrprintfStr( gprs_printfBuff );
	}

	return(ret);

}
/*------------------------------------------------------------------------------------*/
s08 gprsAllowSleep(void)
{

s08 ret = FALSE;
	// TRUE si puede sleep.
	if ( ( modemStatus == M_OFF) || ( modemStatus == M_OFF_IDLE ) ) {
		ret = TRUE;
	}

	return(ret);
}
/*------------------------------------------------------------------------------------*/
u08 getGprsModemStatus(void)
{
	return( modemStatus);
}
/*------------------------------------------------------------------------------------*/

/*
 * sp5K_tkCmd.c
 *
 *  Created on: 27/12/2013
 *      Author: root
 */

#include <sp5KV3.h>

char cmd_printfBuff[CHAR128];
char *argv[16];

static void pv_snprintfP_OK(void );
static void pv_snprintfP_ERR(void);

static u08 pvMakeargv(void);
static void cmdClearScreen(void);
static void cmdHelpFunction(void);
static void cmdResetFunction(void);
static void cmdStatusFunction(void);
static void cmdReadFunction(void);
static void cmdWriteFunction(void);
static void cmdRedialFunction(void);

extern s08 pvMCP_read( u16 i2c_address, u08 dev_addr, u08 length, u08 *data);
extern s08 pvMCP_write( u16 i2c_address, u08 dev_addr, u08 length, u08 *data);

s08 pvSetParamRtc(u08 *s);
s08 pvSetParamGsmBand(u08 *s);
s08 pvSetParamDebugLevel(u08 *s);
s08 pvWriteMcp( u08 *s0, u08 *s1, u08 *s2 );
s08 pvSetParamWrkMode(u08 *s0, u08 *s1);

/*------------------------------------------------------------------------------------*/
void tkCmd(void * pvParameters)
{

u08 c;
( void ) pvParameters;

	vTaskDelay( ( TickType_t)( 1000 / portTICK_RATE_MS ) );
	vt100Init( CMD_UART );

	cmdlineInit();
	cmdlineSetOutputFunc(pvTERMoutChar);

	cmdlineAddCommand("cls", cmdClearScreen );
	cmdlineAddCommand("help", cmdHelpFunction);
	cmdlineAddCommand("reset", cmdResetFunction);
	cmdlineAddCommand("read", cmdReadFunction);
	cmdlineAddCommand("write", cmdWriteFunction);
	cmdlineAddCommand("status", cmdStatusFunction);
	cmdlineAddCommand("redial", cmdRedialFunction);

	// Espero la notificacion para arrancar
	while ( startToken != STOK_CMD ) {
		vTaskDelay( ( TickType_t)( 100 / portTICK_RATE_MS ) );
	}
	TERMrprintfProgStrM("starting tkCmd..\r\n");
	startToken++;

	// send a CR to cmdline input to stimulate a prompt
	//cmdlineInputFunc('\r');

	// loop
	for( ;; )
	{
		clearWdg(WDG_CMD);

		c = '\0';	// Lo borro para que luego del un CR no resetee siempre el timer.

		while(xUartGetChar(CMD_UART, &c, (50 / portTICK_RATE_MS ) ) ) {
			cmdlineInputFunc(c);
		}

		// Mientras escribo comandos no apago la terminal
		if (c=='\r') {
			terminal_restartTimer(SECS2TERMOFF );
		}

		/* run the cmdline execution functions */
		cmdlineMainLoop();
	}
}
/*------------------------------------------------------------------------------------*/
static u08 pvMakeargv(void)
{
// A partir de la linea de comando, genera un array de punteros a c/token
//
char *token = NULL;
char parseDelimiters[] = " ";
int i = 0;

	// inicialmente todos los punteros deben apuntar a NULL.
	memset(argv, NULL, sizeof(argv) );

	// Genero los tokens delimitados por ' '.
	token = strtok(SP5K_CmdlineBuffer, parseDelimiters);
	//snprintf_P( cmd_printfBuff,CHAR128,PSTR("[%d][%s]\r\n\0]"), i, token);
	//TERMrprintfStr( cmd_printfBuff );

	argv[i++] = token;
	while ( (token = strtok(NULL, parseDelimiters)) != NULL ) {

		//snprintf_P( cmd_printfBuff,CHAR128,PSTR("[%d][%s]\r\n\0]"), i, token);
		//TERMrprintfStr( cmd_printfBuff );

		argv[i++] = token;
		if (i == 16) break;
	}



	return(( i - 1));
}
/*------------------------------------------------------------------------------------*/
static void pv_snprintfP_OK(void )
{
	TERMrprintfProgStrM("OK\r\n");
}
/*------------------------------------------------------------------------------------*/
static void pv_snprintfP_ERR(void)
{
	TERMrprintfProgStrM("ERROR\r\n");
}
/*------------------------------------------------------------------------------------*/
static void cmdClearScreen(void)
{
	 vt100ClearScreen(CMD_UART);

}
/*------------------------------------------------------------------------------------*/
static void cmdHelpFunction(void)
{

	memset( &cmd_printfBuff, '\0', CHAR128);
	snprintf_P( cmd_printfBuff,CHAR128,PSTR("\r\nSpymovil %s %s %dch %s %s\r\n\0"), SP5K_MODELO, SP5K_VERSION, NRO_CHANNELS, SP5K_REV, SP5K_DATE);
	TERMrprintfStr( cmd_printfBuff );

	TERMrprintfProgStrM("Available commands are:\r\n");
	TERMrprintfProgStrM("-cls\r\n\0");
	TERMrprintfProgStrM("-help\r\n\0");
	TERMrprintfProgStrM("-reset {default,memory}\r\n\0");
	TERMrprintfProgStrM("-status\r\n\0");
	TERMrprintfProgStrM("-redial\r\n\0");
	TERMrprintfProgStrM("-write rtc YYMMDDhhmm\r\n");
	TERMrprintfProgStrM("       param { wrkmode [service | monitor {sqe|frame}], pwrmode [continuo|discreto] } \r\n");
	TERMrprintfProgStrM("              timerpoll, timerdial, dlgid, gsmband \r\n");
	TERMrprintfProgStrM("              pwrsave [modo {on|off}, {hhmm1}, {hhmm2}\r\n");
	TERMrprintfProgStrM("              debuglevel +/-{none,basic,mem,eventos,data,gprs,digital,all} \r\n");
	TERMrprintfProgStrM("              loglevel (none, info, all)\r\n");
	TERMrprintfProgStrM("              A{0..3} aname imin imax mmin mmax\r\n");
	TERMrprintfProgStrM("              D{0..1} dname magp\r\n");
	TERMrprintfProgStrM("              apn, port, ip, script, passwd\r\n");
	TERMrprintfProgStrM("              output [consigna|manual] o0,01,02,03,hhmm1,hhmm2,chVA,chVB  \r\n");
	TERMrprintfProgStrM("       save\r\n");
	TERMrprintfProgStrM("       mcp devId regAddr {xxxxxxxx}\r\n");
	TERMrprintfProgStrM("       clearq0,clearq1\r\n");
	TERMrprintfProgStrM("       ee addr string\r\n");
	TERMrprintfProgStrM("       led {0|1},gprspwr {0|1},gprssw {0|1},termpwr {0|1},sensorpwr {0|1},analogpwr {0|1}\r\n");
	TERMrprintfProgStrM("       sms {nro,msg}, atcmd {atcmd,timeout}\r\n");
	TERMrprintfProgStrM("       output {reset|noreset|sleep|nosleep},{enable|disable} {0..3}\r\n");
	TERMrprintfProgStrM("       output phase {0..3} {01|10}\r\n");
	TERMrprintfProgStrM("       output pulse {0..3} {01|10} {ms}\r\n");
	TERMrprintfProgStrM("       output consigna{diurna|nocturna} ms\r\n");
	TERMrprintfProgStrM("-read rtc\r\n");
	TERMrprintfProgStrM("      mcp devId regAddr\r\n");
	TERMrprintfProgStrM("      dcd,ri,termsw,din0,din1\r\n");
	TERMrprintfProgStrM("      frame\r\n");
	TERMrprintfProgStrM("      ee addr lenght\r\n");
	TERMrprintfProgStrM("      memory \r\n");
	TERMrprintfCRLF();

}
/*------------------------------------------------------------------------------------*/
static void cmdResetFunction(void)
{

	pvMakeargv();

	if (!strcmp_P( strupr(argv[1]), PSTR("MEMORY\0"))) {
		MEM_drop();
	}

	wdt_enable(WDTO_30MS);
	while(1) {}

}
/*------------------------------------------------------------------------------------*/
static void cmdRedialFunction(void)
{
	// Envio un mensaje a la tk_Gprs para que recargue la configuracion y disque al server
	// Notifico en modo persistente. Si no puedo me voy a resetear por watchdog. !!!!
	while ( xTaskNotify(xHandle_tkGprs, GPRSMSG_RELOAD , eSetBits ) != pdPASS ) {
		vTaskDelay( ( TickType_t)( 100 / portTICK_RATE_MS ) );
	}

}
/*------------------------------------------------------------------------------------*/
static void cmdStatusFunction(void)
{

RtcTimeType rtcDateTime;
u16 pos;
u08 channel;
u08 modemStatus;
frameDataType Cframe;

	memset( &cmd_printfBuff, '\0', CHAR128);
	snprintf_P( cmd_printfBuff,CHAR128,PSTR("\r\nSpymovil %s %s %dch %s %s\r\n\0"), SP5K_MODELO, SP5K_VERSION, NRO_CHANNELS, SP5K_REV, SP5K_DATE);
	TERMrprintfStr( cmd_printfBuff );

	/* DlgId */
	snprintf_P( cmd_printfBuff,CHAR128,PSTR("dlgid: %s\r\n\0"), systemVars.dlgId );
	TERMrprintfStr( cmd_printfBuff );

	/* Fecha y Hora */
	rtcGetTime(&rtcDateTime);
	pos = snprintf_P( cmd_printfBuff,CHAR128,PSTR("rtc: %02d/%02d/%04d "),rtcDateTime.day,rtcDateTime.month, rtcDateTime.year );
	pos += snprintf_P( &cmd_printfBuff[pos],CHAR128,PSTR("%02d:%02d:%02d\r\n\0"),rtcDateTime.hour,rtcDateTime.min, rtcDateTime.sec );
	TERMrprintfStr( cmd_printfBuff );

	/* SERVER */
	snprintf_P( cmd_printfBuff,CHAR128,PSTR(">Server:\r\n\0"));
	TERMrprintfStr( cmd_printfBuff );

	/* APN */
	snprintf_P( cmd_printfBuff,CHAR128,PSTR("  apn: %s\r\n\0"), systemVars.apn );
	TERMrprintfStr( cmd_printfBuff );

	/* SERVER IP:SERVER PORT */
	snprintf_P( cmd_printfBuff,CHAR128,PSTR("  server ip:port: %s:%s\r\n\0"), systemVars.serverAddress,systemVars.serverPort );
	TERMrprintfStr( cmd_printfBuff );

	/* SERVER SCRIPT */
	snprintf_P( cmd_printfBuff,CHAR128,PSTR("  server script: %s\r\n\0"), systemVars.serverScript );
	TERMrprintfStr( cmd_printfBuff );

	/* SERVER PASSWD */
	snprintf_P( cmd_printfBuff,CHAR128,PSTR("  passwd: %s\r\n\0"), systemVars.passwd );
	TERMrprintfStr( cmd_printfBuff );

	// MODEM ---------------------------------------------------------------------------------------
	snprintf_P( cmd_printfBuff,CHAR128,PSTR(">Modem:\r\n\0"));
	TERMrprintfStr( cmd_printfBuff );

	/* Modem band */
	pos = snprintf_P( cmd_printfBuff,CHAR128,PSTR("  band: "));
	switch ( systemVars.gsmBand) {
	case 0:
		pos += snprintf_P( &cmd_printfBuff[pos],CHAR128,PSTR("(900)"));
		break;
	case 1:
		pos += snprintf_P( &cmd_printfBuff[pos],CHAR128,PSTR("(1800)"));
		break;
	case 2:
		pos += snprintf_P( &cmd_printfBuff[pos],CHAR128,PSTR("dual band (900/1800)"));
		break;
	case 3:
		pos += snprintf_P( &cmd_printfBuff[pos],CHAR128,PSTR("pcs (1900)"));
		break;
	case 4:
		pos += snprintf_P( &cmd_printfBuff[pos],CHAR128,PSTR("gsm (850)"));
		break;
	case 5:
		pos += snprintf_P( &cmd_printfBuff[pos],CHAR128,PSTR("dual band (1900/850)"));
		break;
	case 6:
		pos += snprintf_P( &cmd_printfBuff[pos],CHAR128,PSTR("triband (900/1800/1900)"));
		break;
	case 7:
		pos += snprintf_P( &cmd_printfBuff[pos],CHAR128,PSTR("triband (850/1800/1900)"));
		break;
	case 8:
		pos += snprintf_P( &cmd_printfBuff[pos],CHAR128,PSTR("cuatriband (850/900/1800/1900)"));
		break;
	}
	pos += snprintf_P( &cmd_printfBuff[pos],CHAR128,PSTR("\r\n\0"));
	TERMrprintfStr( cmd_printfBuff );

	/* Modem status */
	pos = snprintf_P( cmd_printfBuff,CHAR128,PSTR("  modem: "));
	modemStatus = getGprsModemStatus();
	switch ( modemStatus ) {
	case M_OFF:
		pos += snprintf_P( &cmd_printfBuff[pos],CHAR128,PSTR("OFF\r\n\0"));
		break;
	case M_OFF_IDLE:
		pos += snprintf_P( &cmd_printfBuff[pos],CHAR128,PSTR("OFF IDLE\r\n\0"));
		break;
	case M_ON_CONFIG:
		pos += snprintf_P( &cmd_printfBuff[pos],CHAR128,PSTR("ON CONFIG\r\n\0"));
		break;
	case M_ON_READY:
		pos += snprintf_P( &cmd_printfBuff[pos],CHAR128,PSTR("ON READY\r\n\0"));
		break;
	}
	TERMrprintfStr( cmd_printfBuff );

	/* DLGIP */
	snprintf_P( cmd_printfBuff,CHAR128,PSTR("  dlg ip: %s\r\n\0"), systemVars.dlgIp );
	TERMrprintfStr( cmd_printfBuff );

	/* CSQ */
	snprintf_P( cmd_printfBuff,CHAR128,PSTR("  signalQ: csq=%d, dBm=%d\r\n\0"), systemVars.csq, systemVars.dbm );
	TERMrprintfStr( cmd_printfBuff );

	// DCD/RI/TERMSW
	if ( systemVars.dcd == 0 ) { pos = snprintf_P( cmd_printfBuff,CHAR128,PSTR("  pines: dcd=ON,\0")); }
	if ( systemVars.dcd == 1 ) { pos = snprintf_P( cmd_printfBuff,CHAR128,PSTR("  pines: dcd=OFF,\0"));}
	if ( systemVars.ri == 0 ) { pos += snprintf_P( &cmd_printfBuff[pos],CHAR128,PSTR("ri=ON,\0")); }
	if ( systemVars.ri == 1 ) { pos += snprintf_P( &cmd_printfBuff[pos],CHAR128,PSTR("ri=OFF,\0"));}
	if ( systemVars.termsw == 1 ) { pos += snprintf_P( &cmd_printfBuff[pos],CHAR128,PSTR("term=ON\r\n\0")); }
	if ( systemVars.termsw == 0 ) { pos += snprintf_P( &cmd_printfBuff[pos],CHAR128,PSTR("term=OFF\r\n\0"));}
	TERMrprintfStr( cmd_printfBuff );

	// SYSTEM ---------------------------------------------------------------------------------------
	snprintf_P( cmd_printfBuff,CHAR128,PSTR(">System:\r\n\0"));
	TERMrprintfStr( cmd_printfBuff );

	/* Memoria */
	snprintf_P( cmd_printfBuff,CHAR256,PSTR("  memory: pWr=%d,pRd=%d,pDel=%d,free=%d,4rd=%d,4del=%d  \r\n"), MEM_getWrPtr(), MEM_getRdPtr(), MEM_getDELptr(), MEM_getRcdsFree(),MEM_getRcds4rd(),MEM_getRcds4del() );
	TERMrprintfStr( cmd_printfBuff );

	/* WRK mode (NORMAL / SERVICE) */
	pos = snprintf_P( cmd_printfBuff,CHAR128,PSTR("  wrkmode: "));
	/* WRK mode (NORMAL / SERVICE) */
	switch (systemVars.wrkMode) {
	case WK_NORMAL:
		pos += snprintf_P( &cmd_printfBuff[pos],CHAR128,PSTR("normal\r\n"));
		break;
	case WK_SERVICE:
		pos += snprintf_P( &cmd_printfBuff[pos],CHAR128,PSTR("service\r\n"));
		break;
	case WK_MONITOR_FRAME:
		pos += snprintf_P( &cmd_printfBuff[pos],CHAR128,PSTR("monitor_frame\r\n"));
		break;
	case WK_MONITOR_SQE:
		pos += snprintf_P( &cmd_printfBuff[pos],CHAR128,PSTR("monitor_sqe\r\n"));
		break;
	default:
		pos += snprintf_P( &cmd_printfBuff[pos],CHAR128,PSTR("ERROR\r\n"));
		break;
	}
	TERMrprintfStr( cmd_printfBuff );

	/* PWR mode (CONTINUO / DISCRETO) */
	pos = snprintf_P( cmd_printfBuff,CHAR128,PSTR("  pwrmode: "));
	switch (systemVars.pwrMode) {
	case PWR_CONTINUO:
		pos += snprintf_P( &cmd_printfBuff[pos],CHAR128,PSTR("continuo\r\n"));
		break;
	case PWR_DISCRETO:
		pos += snprintf_P( &cmd_printfBuff[pos],CHAR128,PSTR("discreto\r\n"));
		break;
	default:
		pos += snprintf_P( &cmd_printfBuff[pos],CHAR128,PSTR("ERROR\r\n"));
		break;
	}
	TERMrprintfStr( cmd_printfBuff );

	/* Pwr.Save */
	if ( systemVars.pwrSave == modoPWRSAVE_ON ) {
		pos = snprintf_P( cmd_printfBuff,CHAR128,PSTR("  pwrSave ON: "));
		pos += snprintf_P( &cmd_printfBuff[pos],CHAR128,PSTR("%04d->"),pv_convertMINS2hhmm( systemVars.pwrSaveStartTime));
		pos += snprintf_P( &cmd_printfBuff[pos],CHAR128,PSTR("%04d\r\n"),pv_convertMINS2hhmm( systemVars.pwrSaveEndTime));
	} else {
		snprintf_P( cmd_printfBuff,CHAR128,PSTR("  pwrSave OFF\r\n"));
	}
	TERMrprintfStr( cmd_printfBuff );

	/* Timers */
	snprintf_P( cmd_printfBuff,CHAR128,PSTR("  timerPoll: %d | %d\r\n\0"),systemVars.timerPoll, getTimeToNextPoll() );
	TERMrprintfStr( cmd_printfBuff );
	snprintf_P( cmd_printfBuff,CHAR128,PSTR("  timerDial: %lu | %lu\r\n\0"), systemVars.timerDial, getTimeToNextDial() );
	TERMrprintfStr( cmd_printfBuff );

	/* DebugLevel */
	pos = snprintf_P( cmd_printfBuff,CHAR128,PSTR("  debugLevel: "));
	if ( systemVars.debugLevel == D_NONE) {
		pos += snprintf_P( &cmd_printfBuff[pos],CHAR128,PSTR("none") );
	} else {
		if ( (systemVars.debugLevel & D_BASIC) != 0) { pos += snprintf_P( &cmd_printfBuff[pos],CHAR128,PSTR("+basic")); }
		if ( (systemVars.debugLevel & D_DATA) != 0) { pos += snprintf_P( &cmd_printfBuff[pos],CHAR128,PSTR("+data")); }
		if ( (systemVars.debugLevel & D_GPRS) != 0) { pos += snprintf_P( &cmd_printfBuff[pos],CHAR128,PSTR("+gprs")); }
		if ( (systemVars.debugLevel & D_MEM) != 0)   { pos += snprintf_P( &cmd_printfBuff[pos],CHAR128,PSTR("+mem")); }
		if ( (systemVars.debugLevel & D_EVENTOS) != 0)   { pos += snprintf_P( &cmd_printfBuff[pos],CHAR128,PSTR("+ev")); }
		if ( (systemVars.debugLevel & D_DIGITAL) != 0)  { pos += snprintf_P( &cmd_printfBuff[pos],CHAR128,PSTR("+digital")); }
	}
	snprintf_P( &cmd_printfBuff[pos],CHAR128,PSTR("\r\n\0"));
	TERMrprintfStr( cmd_printfBuff );

	/* CONFIG */
	snprintf_P( cmd_printfBuff,CHAR128,PSTR(">Config:\r\n\0"));
	TERMrprintfStr( cmd_printfBuff );

	/* Configuracion del sistema */
	switch ( getMcpDevice() ) {
	case MCP23008:
		snprintf_P( cmd_printfBuff,CHAR128,PSTR("  analog=%d, digital=2.\r\n\0"),NRO_CHANNELS);
		break;
	case MCP23018:
		pos = snprintf_P( cmd_printfBuff,CHAR128,PSTR("  analog=%d, digital=2, valves=4.\r\n\0"),NRO_CHANNELS);
		break;
	default:
		snprintf_P( cmd_printfBuff,CHAR128,PSTR("ERROR !! No analog system detected\r\n\0"));
		break;
	}
	TERMrprintfStr( cmd_printfBuff );

	// Bateria
	snprintf_P( cmd_printfBuff,CHAR128,PSTR("  batt{0-15V}\r\n\0"));
	TERMrprintfStr( cmd_printfBuff );

	for ( channel = 0; channel < NRO_CHANNELS; channel++) {
		snprintf_P( cmd_printfBuff,CHAR128,PSTR("  a%d{%d-%dmA/%d-%.1f, %.2f/%.2f}(%s)\r\n\0"),channel, systemVars.Imin[channel],systemVars.Imax[channel],systemVars.Mmin[channel],systemVars.Mmax[channel], systemVars.offmmin[channel], systemVars.offmmax[channel], systemVars. aChName[channel] );
		TERMrprintfStr( cmd_printfBuff );
	}
	/* Configuracion de canales digitales */
	snprintf_P( cmd_printfBuff,CHAR128,PSTR("  d0{%.2f p/p} (%s)\r\n\0"), systemVars.magPP[0],systemVars.dChName[0]);
	TERMrprintfStr( cmd_printfBuff );
	snprintf_P( cmd_printfBuff,CHAR128,PSTR("  d1{%.2f p/p} (%s)\r\n\0"), systemVars.magPP[1],systemVars.dChName[1]);
	TERMrprintfStr( cmd_printfBuff );

	/* Salidas */
	// Modo
	if ( systemVars.outputs.wrkMode == OUTPUT_CONSIGNA ) {
		pos = snprintf_P( cmd_printfBuff,CHAR128,PSTR("  outputs mode:consigna [chVA=%d] [chVB=%d]"),systemVars.outputs.chVA,systemVars.outputs.chVB);
		pos += snprintf_P( &cmd_printfBuff[pos],CHAR128,PSTR("  C.Dia:[%04d]"), pv_convertMINS2hhmm( systemVars.outputs.horaConsDia ) );
		pos += snprintf_P( &cmd_printfBuff[pos],CHAR128,PSTR("  C.Noc:[%04d]\r\n\0"), pv_convertMINS2hhmm( systemVars.outputs.horaConsNoc ));
		TERMrprintfStr( cmd_printfBuff );
	}
	if ( systemVars.outputs.wrkMode == OUTPUT_MANUAL ) {
		pos = snprintf_P( cmd_printfBuff,CHAR128,PSTR("  outputs mode:manual"));
		pos += snprintf_P( &cmd_printfBuff[pos],CHAR128,PSTR("  VAL[%d %d %d %d]\r\n\0"),systemVars.outputs.dout[0],systemVars.outputs.dout[1],systemVars.outputs.dout[2],systemVars.outputs.dout[3]);
		TERMrprintfStr( cmd_printfBuff );
	}

	/* VALUES --------------------------------------------------------------------------------------- */
	getAnalogFrame (&Cframe);
	snprintf_P( cmd_printfBuff,CHAR128,PSTR(">Values:\r\n\0"));
	TERMrprintfStr( cmd_printfBuff );

	pos = snprintf_P( cmd_printfBuff, CHAR128, PSTR("  "));
	// TimeStamp.
	pos += snprintf_P( &cmd_printfBuff[pos], CHAR128,PSTR( "%04d%02d%02d,"),Cframe.rtc.year,Cframe.rtc.month,Cframe.rtc.day );
	pos += snprintf_P( &cmd_printfBuff[pos], CHAR128, PSTR("%02d%02d%02d,"),Cframe.rtc.hour,Cframe.rtc.min, Cframe.rtc.sec );
	// Valores analogicos
	for ( channel = 0; channel < NRO_CHANNELS; channel++) {
		pos += snprintf_P( &cmd_printfBuff[pos], CHAR128, PSTR("%s=%.2f,"),systemVars.aChName[channel],Cframe.analogIn[channel] );
	}
	// Valores digitales
	pos += snprintf_P( &cmd_printfBuff[pos], CHAR128, PSTR("%sP=%.2f,%sL=%d,%sW=%d,"), systemVars.dChName[0],Cframe.dIn.din0_pulses,systemVars.dChName[0],Cframe.dIn.din0_level,systemVars.dChName[0], Cframe.dIn.din0_width);
	pos += snprintf_P( &cmd_printfBuff[pos], CHAR128, PSTR("%sP=%.2f,%sL=%d,%sW=%d"), systemVars.dChName[1],Cframe.dIn.din1_pulses,systemVars.dChName[1],Cframe.dIn.din1_level,systemVars.dChName[1], Cframe.dIn.din1_width);
	// Bateria
	pos += snprintf_P( &cmd_printfBuff[pos], CHAR128, PSTR(",bt=%.2f\r\n\0"),Cframe.batt );
	TERMrprintfStr( cmd_printfBuff );


}
/*------------------------------------------------------------------------------------*/
static void cmdReadFunction(void)
{

u08 devId, address, regValue, pin;
u08 length = 10;
s08 retS;
u08 mcp_address;
RtcTimeType rtcDateTime;
char eeRdBuffer[EERDBUFLENGHT];
u16 eeAddress;
u08 pos;
u08 b[9];
u08 eeRdMode;
frameDataType Rframe;
u08 channel;

	pvMakeargv();

	// TIMERS
	// Muestra el estado de todos los timers definidos
	if (!strcmp_P( strupr(argv[1]), PSTR("TIMERS\0"))) {
		sTimersShowInfo();
		return;
	}
	// RTC
	// Lee la hora del RTC.
	if (!strcmp_P( strupr(argv[1]), PSTR("RTC\0"))) {
		retS = rtcGetTime(&rtcDateTime);
		if (retS ) {
			pos = snprintf_P( cmd_printfBuff,CHAR128,PSTR("OK\r\n"));
			pos += snprintf_P( &cmd_printfBuff[pos],CHAR128,PSTR("%02d/%02d/%04d "),rtcDateTime.day,rtcDateTime.month, rtcDateTime.year );
			pos += snprintf_P( &cmd_printfBuff[pos],CHAR128,PSTR("%02d:%02d:%02d\r\n\0"),rtcDateTime.hour,rtcDateTime.min, rtcDateTime.sec );
		} else {
			snprintf_P( cmd_printfBuff,CHAR128,PSTR("ERROR\r\n\0"));
		}
		TERMrprintfStr( cmd_printfBuff );
		return;
	}

	// DCD
	if (!strcmp_P( strupr(argv[1]), PSTR("DCD\0"))) {
		retS = MCP_queryDcd(&pin);
		if (retS ) {
			pos = snprintf_P( cmd_printfBuff,CHAR128,PSTR("OK\r\n"));
			if ( pin == 1 ) { pos += snprintf_P( &cmd_printfBuff[pos],CHAR128,PSTR("DCD ON\r\n\0")); }
			if ( pin == 0 ) { pos += snprintf_P( &cmd_printfBuff[pos],CHAR128,PSTR("DCD OFF\r\n\0")); }
		} else {
			snprintf_P( cmd_printfBuff,CHAR128,PSTR("ERROR\r\n\0"));
		}
		TERMrprintfStr( cmd_printfBuff );
		return;
	}

	// RI
	if (!strcmp_P( strupr(argv[1]), PSTR("RI\0"))) {
		retS = MCP_queryRi(&pin);
		if (retS ) {
			pos = snprintf_P( cmd_printfBuff,CHAR128,PSTR("OK\r\n"));
			if ( pin == 1 ) { pos += snprintf_P( &cmd_printfBuff[pos],CHAR128,PSTR("RI ON\r\n\0")); }
			if ( pin == 0 ) { pos += snprintf_P( &cmd_printfBuff[pos],CHAR128,PSTR("RI OFF\r\n\0")); }
		} else {
			snprintf_P( cmd_printfBuff,CHAR128,PSTR("ERROR\r\n\0"));
		}
		TERMrprintfStr( cmd_printfBuff );
		return;
	}

	// DIN0
	if (!strcmp_P( strupr(argv[1]), PSTR("DIN0\0"))) {
		retS = MCP_queryDin0(&pin);
		if (retS ) {
			pos = snprintf_P( cmd_printfBuff,CHAR128,PSTR("OK\r\n"));
			snprintf_P( &cmd_printfBuff[pos],CHAR128,PSTR("DIN0 %d\r\n\0"), pin);
		} else {
			snprintf_P( cmd_printfBuff,CHAR128,PSTR("ERROR\r\n\0"));
		}
		TERMrprintfStr( cmd_printfBuff );
		return;
	}

	// DIN1
	if (!strcmp_P( strupr(argv[1]), PSTR("DIN1\0"))) {
		retS = MCP_queryDin1(&pin);
		if (retS ) {
			pos = snprintf_P( cmd_printfBuff,CHAR128,PSTR("OK\r\n"));
			snprintf_P( &cmd_printfBuff[pos],CHAR128,PSTR("DIN1 %d\r\n\0"), pin);
		} else {
			snprintf_P( cmd_printfBuff,CHAR128,PSTR("ERROR\r\n\0"));
		}
		TERMrprintfStr( cmd_printfBuff );
		return;
	}

	// TERMSW
	if (!strcmp_P( strupr(argv[1]), PSTR("TERMSW\0"))) {
		pin = ( TERMSW_PIN & _BV(TERMSW_BIT) ) >> TERMSW_BIT;
		pos = snprintf_P( cmd_printfBuff,CHAR128,PSTR("OK\r\n"));
		if ( pin == 1 ) { pos += snprintf_P( &cmd_printfBuff[pos],CHAR128,PSTR("TERMSW ON\r\n\0")); }
		if ( pin == 0 ) { pos += snprintf_P( &cmd_printfBuff[pos],CHAR128,PSTR("TERMSW OFF\r\n\0")); }
		TERMrprintfStr( cmd_printfBuff );
		return;
	}

	// MCP
	// read mcp 0|1|2 addr
	if (!strcmp_P( strupr(argv[1]), PSTR("MCP\0"))) {
		devId = atoi(argv[2]);
		address = atoi(argv[3]);

		if ( devId == 0 ) { mcp_address = MCP_ADDR0; }
		if ( devId == 1 ) { mcp_address = MCP_ADDR1; }
		if ( devId == 2 ) { mcp_address = MCP_ADDR2; }

		retS = pvMCP_read( address, mcp_address, 1, &regValue);
		if (retS ) {
			// Convierto el resultado a binario.
			pos = snprintf_P( cmd_printfBuff,CHAR128,PSTR("OK\r\n"));
			strcpy(b,byte_to_binary(regValue));
			pos += snprintf_P( &cmd_printfBuff[pos],CHAR128,PSTR("mcp %d: reg=[%d] data=[0X%03x] [%s] \r\n\0"),devId, address,regValue, b);
		} else {
			snprintf_P( cmd_printfBuff,CHAR128,PSTR("ERROR\r\n\0"));
		}
		TERMrprintfStr( cmd_printfBuff );
		return;
	}

	// FRAME
	if (!strcmp_P( strupr(argv[1]), PSTR("FRAME\0"))) {
		xTaskNotify(xHandle_tkAIn, AINMSG_POLL , eSetBits );
		return;
	}

	// EEPROM
	// read ee addr length
	if (!strcmp_P( strupr(argv[1]), PSTR("EE\0"))) {
		eeAddress = atol(argv[2]);
		length = atoi(argv[3]);

		retS = EE_read( eeAddress, length, &eeRdBuffer);
		if (retS ) {
			pos = snprintf_P( &cmd_printfBuff,CHAR128,PSTR("OK\r\n"));
			pos += snprintf_P( &cmd_printfBuff[pos],CHAR128,PSTR("addr=[%d] data=[%s]\r\n\0"),eeAddress,eeRdBuffer);
		} else {
			snprintf_P( &cmd_printfBuff,CHAR128,PSTR("ERROR\r\n\0"));
		}
		TERMrprintfStr( cmd_printfBuff );
		return;
	}

	// MEMORY DUMP
	if (!strcmp_P( strupr(argv[1]), PSTR("MEMORY\0"))) {

		// El primer paso es solo leer la memoria.
		retS = MEM_OK;
		eeRdMode = 0;	// Leo siempre desde el ppio.

		for(;;) {

			retS = MEM_read( &Rframe, sizeof(Rframe), eeRdMode );

			switch(retS) {
			case MEM_OK:
				eeRdMode = 1;
				pos = snprintf_P( cmd_printfBuff, CHAR128, PSTR(">" ));
				pos += snprintf_P( &cmd_printfBuff[pos], CHAR128,PSTR( "%04d%02d%02d,"),Rframe.rtc.year,Rframe.rtc.month,Rframe.rtc.day );
				pos += snprintf_P( &cmd_printfBuff[pos], CHAR128, PSTR("%02d%02d%02d,"),Rframe.rtc.hour,Rframe.rtc.min, Rframe.rtc.sec );
				// Valores analogicos
				for ( channel = 0; channel < NRO_CHANNELS; channel++) {
					pos += snprintf_P( &cmd_printfBuff[pos], CHAR128, PSTR("%s=%.2f,"),systemVars.aChName[channel],Rframe.analogIn[channel] );
				}
				// Valores digitales
				pos += snprintf_P( &cmd_printfBuff[pos], CHAR128, PSTR("%sP=%.2f,%sL=%d,%sW=%d,"), systemVars.dChName[0],Rframe.dIn.din0_pulses,systemVars.dChName[0],Rframe.dIn.din0_level,systemVars.dChName[0], Rframe.dIn.din0_width);
				pos += snprintf_P( &cmd_printfBuff[pos], CHAR128, PSTR("%sP=%.2f,%sL=%d,%sW=%d"), systemVars.dChName[1],Rframe.dIn.din1_pulses,systemVars.dChName[1],Rframe.dIn.din1_level,systemVars.dChName[1], Rframe.dIn.din1_width);

				// Bateria
				pos += snprintf_P( &cmd_printfBuff[pos], CHAR128, PSTR(",bt=%.2f<\r\n\0"),Rframe.batt );

				TERMrprintfStr( cmd_printfBuff );
				taskYIELD();
				break;
			case MEM_EMPTY:
				snprintf_P( cmd_printfBuff, CHAR128, PSTR("Mem.empty\r\n" ));
				TERMrprintfStr( cmd_printfBuff );
				return;
				break;
			default:
				snprintf_P( cmd_printfBuff, CHAR128, PSTR("Mem.RD Error %d\r\n"),retS);
				TERMrprintfStr( cmd_printfBuff );
				return;
				break;
			}
		}
	}

	// CMD NOT FOUND
	snprintf_P( &cmd_printfBuff,CHAR128,PSTR("ERROR\r\nCMD NOT DEFINED\r\n"));
	TERMrprintfStr( cmd_printfBuff );
	return;


}
/*------------------------------------------------------------------------------------*/
char *byte_to_binary(int x)
{
    static char b[9];
    b[0] = '\0';

    int z;
    for (z = 128; z > 0; z >>= 1)
    {
        strcat(b, ((x & z) == z) ? "1" : "0");
    }

    return b;
}
/*------------------------------------------------------------------------------------*/
static void cmdWriteFunction(void)
{

s08 retS = FALSE;
u32 param1 = 0;
u32 param2 = 0;
u08 p1 = 0;
u16 delay;
u08 argc;
u16 eeAddress;
u08 length = 10;
char *p;

	argc = pvMakeargv();

	// SAVE
	if (!strcmp_P( strupr(argv[1]), PSTR("SAVE\0"))) {
		retS = pvSaveSystemParamsInEE( &systemVars );
		retS ? pv_snprintfP_OK() : 	pv_snprintfP_ERR();
		return;
	}

	// Parametros:
	if (!strcmp_P( strupr(argv[1]), PSTR("PARAM\0")))
	{

		// ++++++++++++++++++++++++++++++++++++++++++++++++
		// Parametros que solo se cambian desde el modo CMD
		// ++++++++++++++++++++++++++++++++++++++++++++++++

		// PASSWD
		if (!strcmp_P( strupr(argv[2]), PSTR("PASSWD\0"))) {
			memset(systemVars.passwd, '\0', sizeof(systemVars.passwd));
			memcpy(systemVars.passwd, argv[3], sizeof(systemVars.passwd));
			systemVars.passwd[PASSWD_LENGTH - 1] = '\0';
			pv_snprintfP_OK();
			return;
		}

		// DLGID
		if (!strcmp_P( strupr(argv[2]), PSTR("DLGID\0"))) {
			memcpy(systemVars.dlgId, argv[3], sizeof(systemVars.dlgId));
			systemVars.dlgId[DLGID_LENGTH - 1] = '\0';
			pv_snprintfP_OK();
			return;
		}

		// APN
		if (!strcmp_P( strupr(argv[2]), PSTR("APN\0"))) {
			memset(systemVars.apn, '\0', sizeof(systemVars.apn));
			memcpy(systemVars.apn, argv[3], sizeof(systemVars.apn));
			systemVars.apn[APN_LENGTH - 1] = '\0';
			pv_snprintfP_OK();
			return;
		}

		// SERVER PORT
		if (!strcmp_P( strupr(argv[2]), PSTR("PORT\0"))) {
			memset(systemVars.serverPort, '\0', sizeof(systemVars.serverPort));
			memcpy(systemVars.serverPort, argv[3], sizeof(systemVars.serverPort));
			systemVars.serverPort[PORT_LENGTH - 1] = '\0';
			pv_snprintfP_OK();
			return;
		}

		// SERVER IP
		if (!strcmp_P( strupr(argv[2]), PSTR("IP\0"))) {
			memset(systemVars.serverAddress, '\0', sizeof(systemVars.serverAddress));
			memcpy(systemVars.serverAddress, argv[3], sizeof(systemVars.serverAddress));
			systemVars.serverAddress[IP_LENGTH - 1] = '\0';
			pv_snprintfP_OK();
			return;
		}

		// SERVER SCRIPT
		if (!strcmp_P( strupr(argv[2]), PSTR("SCRIPT\0"))) {
			memset(systemVars.serverScript, '\0', sizeof(systemVars.serverScript));
			memcpy(systemVars.serverScript, argv[3], sizeof(systemVars.serverScript));
			systemVars.serverScript[SCRIPT_LENGTH - 1] = '\0';
			pv_snprintfP_OK();
			return;
		}

		/* GSMBAND */
		if (!strcmp_P( strupr(argv[2]), PSTR("GSMBAND\0"))) {
			retS = pvSetParamGsmBand(argv[3]);
			retS ? pv_snprintfP_OK() : 	pv_snprintfP_ERR();
			return;
		}

		/* DEBUGLEVEl */
		if (!strcmp_P( strupr(argv[2]), PSTR("DEBUGLEVEL\0"))) {
			retS = pvSetParamDebugLevel(argv[3]);
			retS ? pv_snprintfP_OK() : 	pv_snprintfP_ERR();
			return;
		}

		/* WRKMODE */
		if (!strcmp_P( strupr(argv[2]), PSTR("WRKMODE\0"))) {
			retS = pvSetParamWrkMode(argv[3],argv[4]);
			retS ? pv_snprintfP_OK() : 	pv_snprintfP_ERR();
			return;
		}

		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		// Parametros que se cambian desde el modo CMD o REMOTAMENTE
		// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

		// SALIDAS
		if (!strcmp_P( strupr(argv[2]), PSTR("OUTPUT\0"))) {

			// [consigna|manual] o0,01,02,03,hhmm1,hhmm2,ch1,ch2

			if (!strcmp_P( strupr(argv[3]), PSTR("CONSIGNA"))) { p1 = OUTPUT_CONSIGNA; }
			if (!strcmp_P( strupr(argv[3]), PSTR("MANUAL"))) { p1 = OUTPUT_MANUAL; }

			retS = pb_setParamOutputs(p1,argv[4],argv[5],argv[6],argv[7],argv[8],argv[9],argv[10],argv[11]);
			retS?  pv_snprintfP_OK() : 	pv_snprintfP_ERR();
			return;
		}

		// PWRSAVE
		if (!strcmp_P( strupr(argv[2]), PSTR("PWRSAVE\0"))) {

			if (!strcmp_P( strupr(argv[3]), PSTR("ON"))) { p1 = modoPWRSAVE_ON; }
			if (!strcmp_P( strupr(argv[3]), PSTR("OFF"))) { p1 = modoPWRSAVE_OFF; }
			pb_setParamPwrSave ( p1, argv[4], argv[5] );
			pv_snprintfP_OK();
			return;
		}

		// CANALES ANALOGICOS
		if (!strcmp_P( strupr(argv[2]), PSTR("A0\0"))) {
			retS = pb_setParamAnalogCh( 0, argv[3],argv[4],argv[5],argv[6],argv[7]);
			retS ? pv_snprintfP_OK() : 	pv_snprintfP_ERR();
			return;
		}

		if (!strcmp_P( strupr(argv[2]), PSTR("A1\0"))) {
			retS = pb_setParamAnalogCh( 1, argv[3],argv[4],argv[5],argv[6],argv[7]);
			retS ? pv_snprintfP_OK() : 	pv_snprintfP_ERR();
			return;
		}

		if (!strcmp_P( strupr(argv[2]), PSTR("A2\0"))) {
			retS = pb_setParamAnalogCh( 2, argv[3],argv[4],argv[5],argv[6],argv[7]);
			retS ? pv_snprintfP_OK() : 	pv_snprintfP_ERR();
			return;
		}

		// CANALES DIGITALES
		if (!strcmp_P( strupr(argv[2]), PSTR("D0\0"))) {
			pb_setParamDigitalCh( 0, argv[3],argv[4]);
			pv_snprintfP_OK();
			return;
		}

		if (!strcmp_P( strupr(argv[2]), PSTR("D1\0"))) {
			pb_setParamDigitalCh( 1, argv[3],argv[4]);
			pv_snprintfP_OK();
			return;
		}

		/* TimerPoll */
		if (!strcmp_P( strupr(argv[2]), PSTR("TIMERPOLL\0"))) {
			retS = pb_setParamTimerPoll(argv[3]);
			retS ? pv_snprintfP_OK() : 	pv_snprintfP_ERR();
			return;
		}

		/* TimerDial */
		if (!strcmp_P( strupr(argv[2]), PSTR("TIMERDIAL\0"))) {
			retS = pb_setParamTimerDial(argv[3]);
			retS ? pv_snprintfP_OK() : 	pv_snprintfP_ERR();
			return;
		}

		/* Pwrmode */
		if (!strcmp_P( strupr(argv[2]), PSTR("PWRMODE\0"))) {

			if ((!strcmp_P(strupr(argv[3]), PSTR("CONTINUO")))) {
				retS = pb_setParamPwrMode(PWR_CONTINUO);
			}

			if ((!strcmp_P(strupr(argv[3]), PSTR("DISCRETO")))) {
				retS = pb_setParamPwrMode(PWR_DISCRETO);
			}
			retS ? pv_snprintfP_OK() : 	pv_snprintfP_ERR();
			return;
		}
	}

	// ++++++++++++++++++++++++++++++++++++++++++++++
	// Parametros de servicio tecnico. Solo desde CMD
	// ++++++++++++++++++++++++++++++++++++++++++++++

	// RTC
	if (!strcmp_P( strupr(argv[1]), PSTR("RTC\0"))) {
		/* YYMMDDhhmm */
		retS = pvSetParamRtc(argv[2]);
		retS ? pv_snprintfP_OK() : 	pv_snprintfP_ERR();
		return;
	}

	// gprsPWR
	if (!strcmp_P( strupr(argv[1]), PSTR("GPRSPWR\0"))) {
		param1 = atoi(argv[2]);
		retS = MCP_setGprsPwr( (u08) param1 );
		retS ? pv_snprintfP_OK() : 	pv_snprintfP_ERR();
		return;
	}

	// gprsSW
	if (!strcmp_P( strupr(argv[1]), PSTR("GPRSSW\0"))) {
		param1 = atoi(argv[2]);
		retS = MCP_setGprsSw( (u08) param1 );
		retS ? pv_snprintfP_OK() : 	pv_snprintfP_ERR();
		return;
	}

	// termPWR
	if (!strcmp_P( strupr(argv[1]), PSTR("TERMPWR\0"))) {
		param1 = atoi(argv[2]);
		retS = MCP_setTermPwr( (u08)param1 );
		retS ? pv_snprintfP_OK() : 	pv_snprintfP_ERR();
		return;
	}

	// sensorPWR
	if (!strcmp_P( strupr(argv[1]), PSTR("SENSORPWR\0"))) {
		param1 = atoi(argv[2]);
		retS = MCP_setSensorPwr( (u08)param1 );
		retS ? pv_snprintfP_OK() : 	pv_snprintfP_ERR();
		return;
	}

	// analogPWR
	if (!strcmp_P( strupr(argv[1]), PSTR("ANALOGPWR\0"))) {
		param1 = atoi(argv[2]);
		retS = MCP_setAnalogPwr( (u08)param1 );
		retS ? pv_snprintfP_OK() : 	pv_snprintfP_ERR();
		return;
	}

	// LED
	// write led 0|1
	if (!strcmp_P( strupr(argv[1]), PSTR("LED\0"))) {
		param1 = atoi(argv[2]);
		retS = MCP_setLed( (u08)param1 );
		if ( param1 == 1 ) {
			cbi(LED_KA_PORT, LED_KA_BIT);
		} else {
			sbi(LED_KA_PORT, LED_KA_BIT);
		}
		retS ? pv_snprintfP_OK() : 	pv_snprintfP_ERR();
		return;
	}

	// EEPROM
	if (!strcmp_P( strupr(argv[1]), PSTR("EE\0"))) {
		eeAddress = atoi(argv[2]);
		p = argv[3];
		while (*p != NULL) {
			p++;
			length++;
			if (length > CMDLINE_BUFFERSIZE )
				break;
		}

		retS = EE_write( eeAddress, length, argv[3] );
		if ( retS ) {
			pv_snprintfP_OK();
		} else {
			pv_snprintfP_ERR();
		}
		return;
	}

	// MCP
	// write mcp 0|1|2 addr value
	if (!strcmp_P( strupr(argv[1]), PSTR("MCP\0"))) {
		retS = pvWriteMcp( argv[2], argv[3], argv[4] );
		retS ? pv_snprintfP_OK() : 	pv_snprintfP_ERR();
		return;
	}

	if (!strcmp_P( strupr(argv[1]), PSTR("CLEARQ0\0"))) {
		cbi(Q_PORT, Q0_CTL_PIN);
		vTaskDelay( ( TickType_t)( 100 / portTICK_RATE_MS ) );
		sbi(Q_PORT, Q0_CTL_PIN);
		pv_snprintfP_OK();
		return;
	}

	if (!strcmp_P( strupr(argv[1]), PSTR("CLEARQ1\0"))) {
		cbi(Q_PORT, Q1_CTL_PIN);
		vTaskDelay( ( TickType_t)( 100 / portTICK_RATE_MS ) );
		sbi(Q_PORT, Q1_CTL_PIN);
		pv_snprintfP_OK();
		return;
	}

	 //output {reset|noreset|sleep|nosleep},{enable|disable}{0..3},consigna{diurna|nocturna}ms
	if (!strcmp_P( strupr(argv[1]), PSTR("OUTPUT\0"))) {

		// reset
		if (!strcmp_P( strupr(argv[2]), PSTR("RESET\0"))) {
			retS = MCP_outputsReset();
			retS ? pv_snprintfP_OK() : 	pv_snprintfP_ERR();
			return;
		}

		// noreset
		if (!strcmp_P( strupr(argv[2]), PSTR("NORESET\0"))) {
			retS = MCP_outputsNoReset();
			retS ? pv_snprintfP_OK() : 	pv_snprintfP_ERR();
			return;
		}

		// sleep
		if (!strcmp_P( strupr(argv[2]), PSTR("SLEEP\0"))) {
			retS = MCP_outputsSleep();
			retS ? pv_snprintfP_OK() : 	pv_snprintfP_ERR();
			return;
		}

		// nosleep
		if (!strcmp_P( strupr(argv[2]), PSTR("NOSLEEP\0"))) {
			retS = MCP_outputsNoSleep();
			retS ? pv_snprintfP_OK() : 	pv_snprintfP_ERR();
			return;
		}

		// enable 0,1,2,3
		if (!strcmp_P( strupr(argv[2]), PSTR("ENABLE\0"))) {
			param1 = atoi(argv[3]);
			switch(param1) {
			case 0:
				retS = MCP_output0Enable();
				break;
			case 1:
				retS = MCP_output1Enable();
				break;
			case 2:
				retS = MCP_output2Enable();
				break;
			case 3:
				retS = MCP_output3Enable();
				break;
			default:
				retS = FALSE;
				break;
			}
			retS ? pv_snprintfP_OK() : 	pv_snprintfP_ERR();
			return;
		}

		// disable 0,1,2,3
		if (!strcmp_P( strupr(argv[2]), PSTR("DISABLE\0"))) {
			param1 = atoi(argv[3]);
			switch(param1) {
			case 0:
				retS = MCP_output0Disable();
				break;
			case 1:
				retS = MCP_output1Disable();
				break;
			case 2:
				retS = MCP_output2Disable();
				break;
			case 3:
				retS = MCP_output3Disable();
				break;
			default:
				retS = FALSE;
				break;
			}
			retS ? pv_snprintfP_OK() : 	pv_snprintfP_ERR();
			return;
		}

		// phase {0,1,2,3} 01|10
		if (!strcmp_P( strupr(argv[2]), PSTR("PHASE\0"))) {
			param1 = atoi(argv[3]);
			switch(param1) {
			case 0:
				if (!strcmp_P( strupr(argv[4]), PSTR("01\0"))) { retS = MCP_output0Phase_01(); }
				if (!strcmp_P( strupr(argv[4]), PSTR("10\0"))) { retS = MCP_output0Phase_10(); }
				break;
			case 1:
				if (!strcmp_P( strupr(argv[4]), PSTR("01\0"))) { retS = MCP_output1Phase_01(); }
				if (!strcmp_P( strupr(argv[4]), PSTR("10\0"))) { retS = MCP_output1Phase_10(); }
				break;
			case 2:
				if (!strcmp_P( strupr(argv[4]), PSTR("01\0"))) { retS = MCP_output2Phase_01(); }
				if (!strcmp_P( strupr(argv[4]), PSTR("10\0"))) { retS = MCP_output2Phase_10(); }
				break;
			case 3:
				if (!strcmp_P( strupr(argv[4]), PSTR("01\0"))) { retS = MCP_output3Phase_01(); }
				if (!strcmp_P( strupr(argv[4]), PSTR("10\0"))) { retS = MCP_output3Phase_10(); }
				break;
			default:
				retS = FALSE;
				break;
			}
			retS ? pv_snprintfP_OK() : 	pv_snprintfP_ERR();
			return;
		}

		// pulse {0,1,2,3} 01|10 ms
		if (!strcmp_P( strupr(argv[2]), PSTR("PULSE\0"))) {
			param1 = atoi(argv[3]);	// canal
			if (!strcmp_P( strupr(argv[4]), PSTR("01\0"))) { param2 = 0; }	// fase
			if (!strcmp_P( strupr(argv[4]), PSTR("10\0"))) { param2 = 1; }
			delay = atoi(argv[5]);	// duracion
			if ( delay > 2000 ) delay = 2000;

			retS = pb_outPulse((u08) param1, (u08) param2, (u16) delay );

			retS ? pv_snprintfP_OK() : 	pv_snprintfP_ERR();
			return;
		}

		// consigna { diurna | nocturna }
		if (!strcmp_P( strupr(argv[2]), PSTR("CONSIGNA\0"))) {

			if (!strcmp_P( strupr(argv[3]), PSTR("DIURNA\0"))) {
				param1 = atoi(argv[4]);
				pb_setConsignaDiurna(param1);
				pv_snprintfP_OK();
				return;
			}

			if (!strcmp_P( strupr(argv[3]), PSTR("NOCTURNA\0"))) {
				param1 = atoi(argv[4]);
				pb_setConsignaNocturna(param1);
				pv_snprintfP_OK();
				return;
			}

			pv_snprintfP_ERR();
			return;
		}

	}

	// CMD NOT FOUND
	snprintf_P( &cmd_printfBuff,CHAR128,PSTR("ERROR\r\nCMD NOT DEFINED\r\n"));
	TERMrprintfStr( cmd_printfBuff );
	return;

}
/*------------------------------------------------------------------------------------*/
s08 pvSetParamRtc(u08 *s)
{
u08 dateTimeStr[11];
char tmp[3];
s08 retS;
RtcTimeType rtcDateTime;

	/* YYMMDDhhmm */
	memcpy(dateTimeStr, argv[2], 10);
	// year
	tmp[0] = dateTimeStr[0]; tmp[1] = dateTimeStr[1];	tmp[2] = '\0';
	rtcDateTime.year = atoi(tmp);
	// month
	tmp[0] = dateTimeStr[2]; tmp[1] = dateTimeStr[3];	tmp[2] = '\0';
	rtcDateTime.month = atoi(tmp);
	// day of month
	tmp[0] = dateTimeStr[4]; tmp[1] = dateTimeStr[5];	tmp[2] = '\0';
	rtcDateTime.day = atoi(tmp);
	// hour
	tmp[0] = dateTimeStr[6]; tmp[1] = dateTimeStr[7];	tmp[2] = '\0';
	rtcDateTime.hour = atoi(tmp);
	// minute
	tmp[0] = dateTimeStr[8]; tmp[1] = dateTimeStr[9];	tmp[2] = '\0';
	rtcDateTime.min = atoi(tmp);

	retS = rtcSetTime(&rtcDateTime);
	return(retS);
}
/*------------------------------------------------------------------------------------*/
s08 pvSetParamGsmBand(u08 *s)
{

	while ( xSemaphoreTake( sem_SYSVars, ( TickType_t ) 10 ) != pdTRUE )
		taskYIELD();
	systemVars.gsmBand = atoi(s);
	xSemaphoreGive( sem_SYSVars );
	return(TRUE);
}
/*------------------------------------------------------------------------------------*/
s08 pvSetParamDebugLevel(u08 *s)
{

	if ((!strcmp_P( strupr(s), PSTR("NONE")))) {
		systemVars.debugLevel = D_NONE;
		return(TRUE);
	}

	if ((!strcmp_P( strupr(s), PSTR("+BASIC")))) {
		systemVars.debugLevel += D_BASIC;
		return(TRUE);
	}

	if ((!strcmp_P( strupr(s), PSTR("-BASIC")))) {
		if ( ( systemVars.debugLevel & D_BASIC) != 0 ) {
			systemVars.debugLevel -= D_BASIC;
			return(TRUE);
		}
	}

	if ((!strcmp_P( strupr(s), PSTR("+DATA")))) {
		systemVars.debugLevel += D_DATA;
		return(TRUE);
	}

	if ((!strcmp_P( strupr(s), PSTR("-DATA")))) {
		if ( ( systemVars.debugLevel & D_DATA) != 0 ) {
			systemVars.debugLevel -= D_DATA;
			return(TRUE);
		}
	}

	if ((!strcmp_P( strupr(s), PSTR("+MEM")))) {
		systemVars.debugLevel += D_MEM;
		return(TRUE);
	}

	if ((!strcmp_P( strupr(s), PSTR("-MEM")))) {
		if ( ( systemVars.debugLevel & D_MEM) != 0 ) {
			systemVars.debugLevel -= D_MEM;
			return(TRUE);
		}
	}

	if ((!strcmp_P( strupr(s), PSTR("+EVENTOS")))) {
		systemVars.debugLevel += D_EVENTOS;
		return(TRUE);
	}

	if ((!strcmp_P( strupr(s), PSTR("-EVENTOS")))) {
		if ( ( systemVars.debugLevel & D_EVENTOS) != 0 ) {
			systemVars.debugLevel -= D_EVENTOS;
			return(TRUE);
		}
	}
	if ((!strcmp_P( strupr(s), PSTR("+GPRS")))) {
		systemVars.debugLevel += D_GPRS;
		return(TRUE);
	}

	if ((!strcmp_P( strupr(s), PSTR("-GPRS")))) {
		if ( ( systemVars.debugLevel & D_GPRS) != 0 ) {
			systemVars.debugLevel -= D_GPRS;
			return(TRUE);
		}
	}

	if ((!strcmp_P( strupr(s), PSTR("+DIGITAL")))) {
		systemVars.debugLevel += D_DIGITAL;
		return(TRUE);
	}

	if ((!strcmp_P( strupr(s), PSTR("-DIGITAL")))) {
		if ( ( systemVars.debugLevel & D_DIGITAL) != 0 ) {
			systemVars.debugLevel -= D_DIGITAL;
			return(TRUE);
		}
	}

	if ((!strcmp_P( strupr(s), PSTR("ALL")))) {
		systemVars.debugLevel = D_DATA + D_GPRS + D_MEM + D_DIGITAL + D_EVENTOS;
		return(TRUE);
	}

	return(FALSE);
}
/*------------------------------------------------------------------------------------*/
s08 pvWriteMcp( u08 *s0, u08 *s1, u08 *s2 )
{

u08 devId, address, regValue;
s08 retS = FALSE;
u08  mcp_address = 0;

	devId = atoi(s0);
	address = atoi(s1);
	regValue = strtol(s2,NULL,2);

	if ( devId == 0 ) { mcp_address = MCP_ADDR0; }
	if ( devId == 1 ) { mcp_address = MCP_ADDR1; }
	if ( devId == 2 ) { mcp_address = MCP_ADDR2; }

	retS = pvMCP_write( address,  mcp_address, 1, &regValue);
	return(retS);

}
/*------------------------------------------------------------------------------------*/
s08 pvSetParamWrkMode(u08 *s0, u08 *s1)
{

	if ((!strcmp_P(strupr(s0), PSTR("SERVICE")))) {
		while ( xSemaphoreTake( sem_SYSVars, ( TickType_t ) 10 ) != pdTRUE )
			taskYIELD();
		systemVars.wrkMode = WK_SERVICE;
		xSemaphoreGive( sem_SYSVars );

		// Aviso a la tarea de control que arranque a controlar el modo service
		xTaskNotify(xHandle_tkControl, CTLMSG_STARTEWM2N , eSetBits );

		// Aviso a las tareas que se pasen a modo SERVICE
		xTaskNotify(xHandle_tkAIn, AINMSG_RELOAD , eSetBits );
		xTaskNotify(xHandle_tkGprs, GPRSMSG_RELOAD , eSetBits );

		return(TRUE);
	}

	if ((!strcmp_P(strupr(s0), PSTR("MONITOR")))) {

		if ((!strcmp_P( strupr(s1), PSTR("SQE")))) {
			while ( xSemaphoreTake( sem_SYSVars, ( TickType_t ) 10 ) != pdTRUE )
				taskYIELD();
			systemVars.wrkMode = WK_MONITOR_SQE;
			xSemaphoreGive( sem_SYSVars );
			xTaskNotify(xHandle_tkAIn, AINMSG_RELOAD , eSetBits );
			xTaskNotify(xHandle_tkGprs, GPRSMSG_RELOAD , eSetBits );
			return(TRUE);
		}

		if ((!strcmp_P( strupr(s1), PSTR("FRAME")))) {
			while ( xSemaphoreTake( sem_SYSVars, ( TickType_t ) 10 ) != pdTRUE )
				taskYIELD();
			systemVars.wrkMode = WK_MONITOR_FRAME;
			xSemaphoreGive( sem_SYSVars );

			// Aviso a la tarea de control que arranque a controlar el modo service
			while ( xTaskNotify(xHandle_tkControl, CTLMSG_STARTEWM2N , eSetBits ) != pdPASS ) {
				vTaskDelay( ( TickType_t)( 100 / portTICK_RATE_MS ) );
			}
			// Aviso a las tareas que se pasen a este modo
			while ( xTaskNotify(xHandle_tkAIn, AINMSG_RELOAD , eSetBits ) != pdPASS ) {
				vTaskDelay( ( TickType_t)( 100 / portTICK_RATE_MS ) );
			}
			while ( xTaskNotify(xHandle_tkGprs, GPRSMSG_RELOAD , eSetBits ) != pdPASS ) {
				vTaskDelay( ( TickType_t)( 100 / portTICK_RATE_MS ) );
			}
			return(TRUE);
		}

	}

	return(FALSE);

}
//------------------------------------------------------------------------------------

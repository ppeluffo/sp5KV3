/*
 * sp5KV3_generalUse.c
 *
 *  Created on: 24/7/2015
 *      Author: pablo
 *
 *  Funciones de uso general utilizadas en varios modulos.
 */

#include <sp5KV3.h>

char gral_printfBuff[CHAR64];

s08 pb_setParamPwrMode(u08 pwrMode)
{

	while ( xSemaphoreTake( sem_SYSVars, ( TickType_t ) 10 ) != pdTRUE )
		taskYIELD();
	systemVars.pwrMode =  pwrMode;
	xSemaphoreGive( sem_SYSVars );

	// Aviso a las tareas que recargue la configuracion
	while (  xTaskNotify(xHandle_tkAIn, AINMSG_RELOAD , eSetBits ) != pdPASS ) {
		vTaskDelay( ( TickType_t)( 100 / portTICK_RATE_MS ) );
	}
	while (  xTaskNotify(xHandle_tkGprs, GPRSMSG_RELOAD , eSetBits ) != pdPASS ) {
		vTaskDelay( ( TickType_t)( 100 / portTICK_RATE_MS ) );
	}

	return(TRUE);
}
//------------------------------------------------------------------------------------
s08 pb_setParamTimerPoll(char *p1)
{

	while ( xSemaphoreTake( sem_SYSVars, ( TickType_t ) 10 ) != pdTRUE )
		taskYIELD();
	systemVars.timerPoll = (u16) ( atol(p1) );
	xSemaphoreGive( sem_SYSVars );

	// Notifico en modo persistente. Si no puedo me voy a resetear por watchdog. !!!!
	while ( xTaskNotify(xHandle_tkAIn, AINMSG_RELOAD , eSetBits ) != pdPASS ) {
		vTaskDelay( ( TickType_t)( 100 / portTICK_RATE_MS ) );
	}

	return(TRUE);
}
/*------------------------------------------------------------------------------------*/
s08 pb_setParamTimerDial(char *p1)
{

// Actualizo el timer de discado

	while ( xSemaphoreTake( sem_SYSVars, ( TickType_t ) 10 ) != pdTRUE )
		taskYIELD();
	systemVars.timerDial = atol(p1);
	xSemaphoreGive( sem_SYSVars );

	// Notifico en modo persistente. Si no puedo me voy a resetear por watchdog. !!!!
	while ( xTaskNotify(xHandle_tkGprs, GPRSMSG_RELOAD , eSetBits ) != pdPASS ) {
		vTaskDelay( ( TickType_t)( 100 / portTICK_RATE_MS ) );
	}

	return(TRUE);
}
/*------------------------------------------------------------------------------------*/
void pb_setParamPwrSave(u08 p1, char *p2, char *p3)
{
// Recibe como parametros el modo ( 0,1) y punteros a string con las horas de inicio y fin del pwrsave
// expresadas en minutos.

	while ( xSemaphoreTake( sem_SYSVars, ( TickType_t ) 10 ) != pdTRUE )
		taskYIELD();

	systemVars.pwrSave = p1;
	if ( p2 != NULL ) { systemVars.pwrSaveStartTime = pv_convertHHMM2min ( atol(p2) ); }
	if ( p3 != NULL ) { systemVars.pwrSaveEndTime = pv_convertHHMM2min ( atol(p3) ); }

	xSemaphoreGive( sem_SYSVars );

}
/*------------------------------------------------------------------------------------*/
s08 pb_setParamAnalogCh( u08 channel, char *p1, char *p2, char *p3, char *p4, char *p5 )
{
	// p1 = name, p2 = iMin, p3 = iMax, p4 = mMin, p5 = mMax

	while ( xSemaphoreTake( sem_SYSVars, ( TickType_t ) 10 ) != pdTRUE )
		taskYIELD();

	if ( p1 != NULL ) {
		memset ( systemVars.aChName[channel], '\0',   PARAMNAME_LENGTH );
		memcpy( systemVars.aChName[channel], p1 , ( PARAMNAME_LENGTH - 1 ));
	}

	if ( p2 != NULL ) { systemVars.Imin[channel] = atoi(p2); }
	if ( p3 != NULL ) {	systemVars.Imax[channel] = atoi(p3); }
	if ( p4 != NULL ) {	systemVars.Mmin[channel] = atoi(p4); }
	if ( p5 != NULL ) {	systemVars.Mmax[channel] = atof(p5); }

	xSemaphoreGive( sem_SYSVars );

	return(TRUE);

}
/*------------------------------------------------------------------------------------*/
void pb_setParamDigitalCh( u08 channel, char *p1, char *p2 )
{
	while ( xSemaphoreTake( sem_SYSVars, ( TickType_t ) 10 ) != pdTRUE )
		taskYIELD();

	if ( p1 != NULL ) {
		memset ( systemVars.dChName[channel], '\0',   PARAMNAME_LENGTH );
		memcpy( systemVars.dChName[channel], p1 , ( PARAMNAME_LENGTH - 1 ));
	}

	if ( p2 != NULL ) { systemVars.magPP[channel] = atof(p2); }

	xSemaphoreGive( sem_SYSVars );

}
/*------------------------------------------------------------------------------------*/
s08 pb_setParamOutputs( u08 modo,char *p1,char *p2,char *p3,char *p4,char *p5,char *p6,char *p7,char *p8 )
{

	// [consigna|manual] [timer] o0,01,02,03,hhmm1,hhmm2,ch1,ch2

	while ( xSemaphoreTake( sem_SYSVars, ( TickType_t ) 10 ) != pdTRUE )
		taskYIELD();

	systemVars.outputs.wrkMode = modo;

	if ( p1 != NULL ) { systemVars.outputs.dout[0] = atoi(p1); }
	if ( p2 != NULL ) { systemVars.outputs.dout[1] = atoi(p2); }
	if ( p3 != NULL ) { systemVars.outputs.dout[2] = atoi(p3); }
	if ( p4 != NULL ) { systemVars.outputs.dout[3] = atoi(p4); }
	if ( p5 != NULL ) { systemVars.outputs.horaConsDia =  pv_convertHHMM2min ( atol(p5) ); }
	if ( p6 != NULL ) { systemVars.outputs.horaConsNoc =  pv_convertHHMM2min ( atol(p6) ); }
	if ( p7 != NULL ) { systemVars.outputs.chVA = atoi(p7); }
	if ( p8 != NULL ) { systemVars.outputs.chVB = atoi(p8); }

	// Aviso a la tarea de salida que recargue la configuracion
	while( xTaskNotify(xHandle_tkOutput, MSG2OUT_UPDATE , eSetBits ) != pdPASS ) {
		vTaskDelay( ( TickType_t)( 100 / portTICK_RATE_MS ) );
	}
	xSemaphoreGive( sem_SYSVars );
	return(TRUE);

}
/*------------------------------------------------------------------------------------*/
s08 pb_outPulse( u08 channel, u08 phase, u16 delay )
{

s08 retS = FALSE;

//	snprintf_P( gral_printfBuff,CHAR64,PSTR("pb_outPulse:: CH=%d,PH=%d,T=%d\r\n\0"),channel,phase,delay );
//	TERMrprintfStr( gral_printfBuff );

	MCP_outputsNoSleep();
	MCP_outputsNoReset();
	switch(channel) {
	case 0:
		if ( phase == 0 ) { retS = MCP_output0Phase_01(); }
		if ( phase == 1 ) { retS = MCP_output0Phase_10(); }
		MCP_output0Enable();
		vTaskDelay( ( TickType_t)( delay / portTICK_RATE_MS ) );
		MCP_output0Disable();
		break;
	case 1:
		if ( phase == 0 ) { retS = MCP_output1Phase_01(); }
		if ( phase == 1 ) { retS = MCP_output1Phase_10(); }
		MCP_output1Enable();
		vTaskDelay( ( TickType_t)( delay / portTICK_RATE_MS ) );
		MCP_output1Disable();
		break;
	case 2:
		if ( phase == 0 ) { retS = MCP_output2Phase_01(); }
		if ( phase == 1 ) { retS = MCP_output2Phase_10(); }
		MCP_output2Enable();
		vTaskDelay( ( TickType_t)( delay / portTICK_RATE_MS ) );
		MCP_output2Disable();
		break;
	case 3:
		if ( phase == 0 ) { retS = MCP_output3Phase_01(); }
		if ( phase == 1 ) { retS = MCP_output3Phase_10(); }
		MCP_output3Enable();
		vTaskDelay( ( TickType_t)( delay / portTICK_RATE_MS ) );
		MCP_output3Disable();
		break;
	default:
		retS = FALSE;
		break;
	}

	MCP_outputsSleep();
	return(retS);

}
/*------------------------------------------------------------------------------------*/
u16 pv_convertHHMM2min(u16 HHMM )
{
u16 HH,MM,mins;

	HH = HHMM / 100;
	MM = HHMM % 100;
	mins = 60 * HH + MM;
	return(mins);
}
/*------------------------------------------------------------------------------------*/
u16 pv_convertMINS2hhmm ( u16 mins )
{
u16 HH,MM, hhmm;

	HH = mins / 60;
	MM = mins % 60;
	hhmm = HH * 100 + MM;
	return(hhmm);
}
/*------------------------------------------------------------------------------------*/
s08 pvSaveSystemParamsInEE( systemVarsType *sVars )
{

int EEaddr;
s08 retS = FALSE;

	// Almaceno en EE
	EEaddr = EEADDR_START_PARAMS;
	while ( xSemaphoreTake( sem_SYSVars, ( TickType_t ) 10 ) != pdTRUE )
		taskYIELD();
	paramStore(sVars, EEaddr, sizeof(systemVarsType));
	xSemaphoreGive( sem_SYSVars );

	// Confirmo la escritura.
	vTaskDelay( (100 / portTICK_RATE_MS) );

	while ( xSemaphoreTake( sem_SYSVars, ( TickType_t ) 10 ) != pdTRUE )
		taskYIELD();
	retS = paramLoad(sVars, EEaddr, sizeof(systemVarsType));
	xSemaphoreGive( sem_SYSVars );

	return(retS);

}
//------------------------------------------------------------------------------------
void pb_setConsignaDiurna ( u16 ms )
{
	// Una consigna es la activacion simultanea de 2 valvulas, en las cuales un
	// se abre y la otra se cierra.
	// Cierro la valvula 1
	// Abro la valvula 2
	// Para abrir una valvula debemos poner una fase 10.
	// Para cerrar es 01

	 pb_outPulse( systemVars.outputs.chVA , 0, ms );	// Cierro la valvula 1
	 vTaskDelay( ( TickType_t)( 1000 / portTICK_RATE_MS ) );
	 pb_outPulse( systemVars.outputs.chVB , 1, ms );	// Abro la valvula 2

	 pb_outs2standby();
	 TERMrprintfProgStrM("tkOutput:: consigna DIURNA\r\n");
}
/*------------------------------------------------------------------------------------*/
void pb_setConsignaNocturna ( u16 ms )
{
	// Abro la valvula 1
	// Cierro la valvula 2

	 pb_outPulse( systemVars.outputs.chVA , 1, ms );	// Abro la valvula 1
	 vTaskDelay( ( TickType_t)( 1000 / portTICK_RATE_MS ) );
	 pb_outPulse( systemVars.outputs.chVB , 0, ms );	// Cierro la valvula 2

	 pb_outs2standby();
	 TERMrprintfProgStrM("tkOutput:: consigna NOCTURNA\r\n");

}
/*------------------------------------------------------------------------------------*/
void pb_outs2standby(void)
{
	MCP_output0Disable();
	MCP_output1Disable();
	MCP_output2Disable();
	MCP_output3Disable();

	MCP_outputsSleep();
}
/*------------------------------------------------------------------------------------*/

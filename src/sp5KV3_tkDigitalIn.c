/*
 * sp5KV3_tkDigitalIn.c
 *
 *  Created on: 13/4/2015
 *      Author: pablo
 *
 *  La nueva modalidad es por poleo.
 *  Configuro el MCP para que no interrumpa
 *  C/250ms leo el registro GPIO del MCP.
 *  En reposo la salida de los latch es 1 por lo que debo detectar cuando se hizo 0.
 *  Para evitar poder quedar colgado, c/ciclo borro el latch.
 *
 *	Esta version solo puede usarse con placas SP5K_3CH que tengan latch para los pulsos, o sea
 *	version >= R003.
 *
 */


#include <sp5KV3.h>

static void pvClearQ0(void);
static void pvClearQ1(void);
static void pvPollQ(void);
static void pvPollDcd(void);

extern s08 pvMCP_read( u16 i2c_address, u08 dev_addr, u08 length, u08 *data);

char digitalIn_printfBuff[CHAR64];
u08 mcpDevice = MCP_NONE;

dinDataType dInputs;

/*------------------------------------------------------------------------------------*/
void tkDigitalIn(void * pvParameters)
{

( void ) pvParameters;

	// Los pines del micro que resetean los latches de caudal son salidas.
	sbi(Q_DDR, Q0_CTL_PIN);
	sbi(Q_DDR, Q1_CTL_PIN);

	// Espero la notificacion para arrancar
	while ( startToken != STOK_DIN ) {
		vTaskDelay( ( TickType_t)( 100 / portTICK_RATE_MS ) );
	}
	TERMrprintfProgStrM("starting tkDigitalIn..\r\n");
	startToken++;

	// Determino que tipo de placa analogica tengo.
	mcpDevice = getMcpDevice();

	// Inicializo los latches borrandolos
	pvClearQ0();
	pvClearQ1();
	dInputs.din0_level = 0;
	dInputs.din1_level = 0;
	dInputs.din0_pulses = 0;
	dInputs.din1_pulses = 0;
	dInputs.din0_width = 0;
	dInputs.din1_width = 0;

	MCP_queryDcd(&systemVars.dcd);

	for( ;; )
	{
		clearWdg(WDG_DIN);

		vTaskDelay( ( TickType_t)( 100 / portTICK_RATE_MS ) );

		pvPollDcd();

		if ( mcpDevice == MCP23018 )
			pvPollQ();

	}

}
/*------------------------------------------------------------------------------------*/
static void pvPollDcd(void)
{
s08 retS = FALSE;
u08 pin;
u32 tickCount;

	retS = MCP_queryDcd(&pin);
	// Solo indico los cambios.
	if ( systemVars.dcd != pin ) {
		systemVars.dcd = pin;
		tickCount = xTaskGetTickCount();
		if ( pin == 1 ) { snprintf_P( digitalIn_printfBuff,CHAR64,PSTR(".[%06lu] tkDigitalIn: DCD off(1)\r\n\0"), tickCount );	}
		if ( pin == 0 ) { snprintf_P( digitalIn_printfBuff,CHAR64,PSTR(".[%06lu] tkDigitalIn: DCD on(0)\r\n\0"), tickCount );	}

		if ( (systemVars.debugLevel & D_DIGITAL) != 0) {
			TERMrprintfStr( digitalIn_printfBuff );
		}
	}
}
/*------------------------------------------------------------------------------------*/
static void pvPollQ(void)
{

s08 retS = FALSE;
u08 dInt0 = 0;
u08 dInt1 = 0;
u08 mcpReg;
s08 debugQ = FALSE;
u32 tickCount;

	// Leo el GPIO.
	retS = pvMCP_read( MCP2_GPIOB, MCP_ADDR2 , 1, &mcpReg);

	// Determino si algun pin din esta en 0. (en reposo Q = 1 )
	dInt0 = ( mcpReg & _BV(MCP2_GPIO_DIN0) ) >> MCP2_GPIO_DIN0;	// Din 0
	dInt1 = ( mcpReg & _BV(MCP2_GPIO_DIN1) ) >> MCP2_GPIO_DIN1;	// Din 1

	// Levels
	dInputs.din0_level = dInt0;
	dInputs.din1_level = dInt1;

	// Counts
	debugQ = FALSE;
	if (dInt0 == 0 ) { dInputs.din0_pulses++ ; debugQ = TRUE;}
	if (dInt1 == 0 ) { dInputs.din1_pulses++ ; debugQ = TRUE;}

	// Widths
	if (dInt0 == 1 ) { dInputs.din0_width++ ; }
	if (dInt1 == 0 ) { dInputs.din1_width++ ; }

	// Siempre borro los latches para evitar la posibilidad de quedar colgado.
	pvClearQ0();
	pvClearQ1();

	if ( ((systemVars.debugLevel & D_DIGITAL) != 0) && debugQ ) {
		tickCount = xTaskGetTickCount();
		snprintf_P( digitalIn_printfBuff,CHAR64,PSTR(".[%06lu] tkDigitalIn: din0=%.0f,din1=%.0f\r\n\0"), tickCount, dInputs.din0_pulses,dInputs.din0_pulses);
		TERMrprintfStr( digitalIn_printfBuff );
	}

}
/*------------------------------------------------------------------------------------*/
static void pvClearQ0(void)
{
	// Pongo un pulso 0->1 en Q0 pin para resetear el latch
	// En reposo debe quedar en H.
	cbi(Q_PORT, Q0_CTL_PIN);
	taskYIELD();
	sbi(Q_PORT, Q0_CTL_PIN);

}
/*------------------------------------------------------------------------------------*/
static void pvClearQ1(void)
{
	// Pongo un pulso 0->1 en Q1 pin para resetear el latch
	cbi(Q_PORT, Q1_CTL_PIN);
	taskYIELD();
	sbi(Q_PORT, Q1_CTL_PIN);

}
/*------------------------------------------------------------------------------------*/
void getDigitalInputs( dinDataType *dIn , s08 resetCounters )
{
	memcpy( dIn, &dInputs, sizeof(dInputs)) ;	// copio los valores
	if ( resetCounters == TRUE ) {
		dInputs.din0_level = 0;
		dInputs.din1_level = 0;
		dInputs.din0_pulses = 0;
		dInputs.din1_pulses = 0;
		dInputs.din0_width = 0;
		dInputs.din1_width = 0;
	}
}
/*------------------------------------------------------------------------------------*/

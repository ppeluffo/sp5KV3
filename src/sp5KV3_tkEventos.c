/*
 * sp5KV3_tkEventos.c
 *
 *  Created on: 21/4/2015
 *      Author: pablo
 */



#include <sp5KV3.h>

char ev_printfBuff[CHAR128];

/*------------------------------------------------------------------------------------*/
void tkEventos(void * pvParameters)
{

( void ) pvParameters;
BaseType_t xResult;
uint32_t ulNotifiedValue;
u32 tickCount;

	vTaskDelay( ( TickType_t)( 1000 / portTICK_RATE_MS ) );

	// Espero la notificacion para arrancar
	while ( startToken != STOK_EV ) {
		vTaskDelay( ( TickType_t)( 100 / portTICK_RATE_MS ) );
	}
	TERMrprintfProgStrM("starting tkEventos..\r\n");
	startToken++;

	// Loop
	for( ;; )
	{

		clearWdg(WDG_EVN);

		// Genero una espera de 100ms por algun mensaje para hacer algo.
		xResult = xTaskNotifyWait( 0x00,          	 				/* Don't clear bits on entry. */
		                           ULONG_MAX,        				/* Clear all bits on exit. */
		                           &ulNotifiedValue, 				/* Stores the notified value. */
								   (100 / portTICK_RATE_MS ) );

		if( xResult == pdTRUE ) {
			// Frame Ready
			if ( ( ulNotifiedValue & FRAMERDY_BIT ) != 0 ) {

				if ( (systemVars.debugLevel & D_EVENTOS) != 0)  {
					tickCount = xTaskGetTickCount();
					snprintf_P( ev_printfBuff,CHAR64,PSTR(".[%06lu] tkEventos::Frame ready\r\n"), tickCount);
					TERMrprintfStr( ev_printfBuff );
				}

			}

		 }
	}
}
/*------------------------------------------------------------------------------------*/

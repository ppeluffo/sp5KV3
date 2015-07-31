/*------------------------------------------------------------------------------------
 * rtc_sp5KFRTOS.h
 * Autor: Pablo Peluffo @ 2015
 * Basado en Proycon AVRLIB de Pascal Stang.
 *
 * Son funciones que impelementan la API de acceso al RTC del sistema SP5K con FRTOS.
 * Para su uso debe estar inicializado el semaforo del bus I2C, que se hace llamando a i2cInit().
 *
 *
*/

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/twi.h>

#include "avrlibdefs.h"
#include "avrlibtypes.h"
#include "global.h"
#include "i2c_sp5KFRTOS.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"

#define DS1340_ID   	0xD0
#define DS1340_ADDR  	0x00

#ifndef AVRLIBFRTOS_RTC_SP5KFRTOS_H_
#define AVRLIBFRTOS_RTC_SP5KFRTOS_H_

typedef struct struct_RtcTime
{
	// Tamanio: 7 byte.
	// time of day
	u08 sec;
	u08 min;
	u08 hour;
	// date
	u08 day;
	u08 month;
	u16 year;

} RtcTimeType;

s08 rtcGetTime(RtcTimeType *rtc);
s08 rtcSetTime(RtcTimeType *rtc);


#endif /* AVRLIBFRTOS_RTC_SP5KFRTOS_H_ */

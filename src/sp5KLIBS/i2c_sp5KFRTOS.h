/*------------------------------------------------------------------------------------
 * i2c_sp5KFRTOS.h
 * Autor: Pablo Peluffo @ 2015
 * Basado en Proycon AVRLIB de Pascal Stang.
 *
 * Son funciones que impelementan el driver para el bus I2C ( TWI)  del ATmega1284P, adaptado al sistema SP5K
 * y al FRTOS.
 * Por ser drivers, no deberian accederse a estas funciones directamente sino que se hace por medio de
 * las APIs de rtc/mcp/eeprom
 * El unico acceso que se debe hacer es para inicializarlo por medio de las funciones de init. Esta
 * inicializa el bus y crea el semaforo.
 *
*/

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/twi.h>

#include "avrlibdefs.h"
#include "avrlibtypes.h"
#include "global.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"


#ifndef AVRLIBFRTOS_I2C_SP5KFRTOS_H_
#define AVRLIBFRTOS_I2C_SP5KFRTOS_H_


#define MSTOTAKEI2CSEMPH ((  TickType_t ) 10 )

// I2C

#define I2C_MAXTRIES	5
#define EERDBUFLENGHT	64

#define EE24X		0
#define DS1340		1
#define MPC23008	2
#define ADS7828		3
//
#define SCL		0
#define SDA		1
//
#define ACK 1
#define NACK 0
//
void i2c_init(void);
s08 i2c_masterWrite (u08 dev_type, u16 i2c_address, u08 dev_id, u08 dev_addr,u08 length, char *data);
s08 i2c_masterRead (u08 dev_type, u16 i2c_address, u08 dev_id, u08 dev_addr,u08 length, char *data);
s08 adc_masterRead (u08 dev_type, u16 i2c_address, u08 dev_id, u08 dev_addr,u08 length, u08 *data);
//
// Semaforos mutex
SemaphoreHandle_t semph_I2C;

//------------------------------------------------------------------------------------

#endif /* AVRLIBFRTOS_I2C_SP5KFRTOS_H_ */

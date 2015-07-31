/*
 * ee_sp5KFRTOS.h
 *
 *  Created on: 26/5/2015
 *      Author: pablo
 */

#include "avrlibdefs.h"
#include "avrlibtypes.h"
#include "global.h"
#include "i2c_sp5KFRTOS.h"
#include "FreeRTOS.h"
#include "semphr.h"

#ifndef SRC_SP5KLIBS_EE_SP5KFRTOS_H_
#define SRC_SP5KLIBS_EE_SP5KFRTOS_H_

#define EE_ID				0xA0
#define EE_ADDR				0x00
#define EEPAGESIZE			64

s08 EE_write( u16 i2c_address,u08 length, char *data);
s08 EE_read( u16 i2c_address,u08 length, char *data);
s08 EE_pageWrite( u16 i2c_address,u08 length, char *data);

#endif /* SRC_SP5KLIBS_EE_SP5KFRTOS_H_ */

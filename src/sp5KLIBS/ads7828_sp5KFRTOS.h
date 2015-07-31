/*
 * ads7828_sp5KFRTOS.h
 *
 *  Created on: 15/4/2015
 *      Author: pablo
 */

#ifndef SRC_SP5KLIBS_ADS7828_SP5KFRTOS_H_
#define SRC_SP5KLIBS_ADS7828_SP5KFRTOS_H_

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

#define 	ADS7828_ID	  	0x91
#define 	ADS7828_ADDR	0x00

#define 	ADS7828_CMD_SD   0x80		//ADS7828 Single-ended/Differential Select bit.

#define 	ADS7828_CMD_PDMODE0   0x00	//ADS7828 Power-down Mode 0.
#define 	ADS7828_CMD_PDMODE1   0x04	//ADS7828 Power-down Mode 1.
#define 	ADS7828_CMD_PDMODE2   0x08	//ADS7828 Power-down Mode 2.
#define 	ADS7828_CMD_PDMODE3   0x0C	//ADS7828 Power-down Mode 3.

void ADS7828_init(void);
s08 ADS7828_convert(u08 channel, u16 *value);


#endif /* SRC_SP5KLIBS_ADS7828_SP5KFRTOS_H_ */

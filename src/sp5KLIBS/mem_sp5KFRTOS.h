/*
 * mem_sp5KFRTOS.h
 *
 *  Created on: 26/5/2015
 *      Author: pablo
 */

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/twi.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include <avr/wdt.h>

#include "avrlibdefs.h"
#include "avrlibtypes.h"
#include "global.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"

#ifndef SRC_SP5KLIBS_MEM_SP5KFRTOS_H_
#define SRC_SP5KLIBS_MEM_SP5KFRTOS_H_

#define EESIZE  		32   // Tamanio en KB de la eeprom externa.
#define MEMRECDSIZE		64
#define EEADDR_START	0
#define MAXMEMRCS		512

#define MEM_OK			0x00
#define MEM_WREEFAIL	0x01
#define MEM_RDEEFAIL	0x02
#define MEM_DELEEFAIL	0x03
#define MEM_FULL		0x04
#define MEM_EMPTY		0x05
#define MEM_FULL4DEL	0x06
#define MEM_EMPTY4WR	0x07
#define MEM_EMPTY4RD	0x08
#define MEM_EMPTY4DEL 	0x09
#define MEM_CKSERR		0x10
#define MEM_RDSHIFT		0x11
#define MEM_DELSHIFT	0x12


s08 MEM_init( void );
u16 MEM_getRdPtr(void);
u16 MEM_getWrPtr(void);
u16 MEM_getDELptr(void);
u16 MEM_getRcdsFree(void);
s08 MEM_write( void *dataFrame, const u08 dataFrameSize );
s08 MEM_read( void *dataFrame, const u08 dataFrameSize, const s08 modo );
s08 MEM_erase( void );
void MEM_drop(void);

s08 MEMisFull(void);
s08 MEMisEmpty4Del(void);
s08 MEMisEmpty4Read(void);
u16 MEM_getRcds4del(void);
u16 MEM_getRcds4rd(void);

// Semaforos mutex
SemaphoreHandle_t semph_MEM;

#endif /* SRC_SP5KLIBS_MEM_SP5KFRTOS_H_ */

/*
 * mcp_sp5KFRTOS.c
 *
 *  Created on: 01/11/2013
 *      Author: root
 *
 * Funciones para uso de los MCP del SP5K modificadas para usarse con FRTOS.
 *
 *
 */
//------------------------------------------------------------------------------------

#include "avrlibdefs.h"
#include "avrlibtypes.h"
#include "global.h"
#include "i2c_sp5KFRTOS.h"
#include "FreeRTOS.h"
#include "semphr.h"


#ifndef AVRLIBFRTOS_MCP_SP5KFRTOS_H_
#define AVRLIBFRTOS_MCP_SP5KFRTOS_H_

//------------------------------------------------------------------------------------
// MCP23008

// MCP0: MCP23008 placa logica
// MCP1: MCP23008 placa analogica
// MCP2: MCP23018 placa analogica

// Pines del micro ATmega1284 conectado a la interrupcion de los MCP.
#define MCP0_PORT	PORTD
#define MCP0_PIN	PIND
#define MCP0_BIT	5
#define MCP0_DDR	DDRD
#define MCP0_MASK	0x20

#define MCP1_PORT	PORTB
#define MCP1_PIN	PINB
#define MCP1_BIT	2
#define MCP1_DDR	DDRB
#define MCP1_MASK	0x04

#define MCP2_PORT	PORTB
#define MCP2_PIN	PINB
#define MCP2_BIT	2
#define MCP2_DDR	DDRB
#define MCP2_MASK	0x04

//------------------------------------------------------------------------------------
// Identificacion en el bus I2C de los MCP
#define MCP_ID				0x40
#define MCP_ADDR0			0x00	// MCP23008
#define MCP_ADDR1			0x01	// MCP23008
#define MCP_ADDR2			0x07	// MCP23018

//------------------------------------------------------------------------------------
// Registros MCP0
#define MCP0_IODIR		0x00
#define MCP0_IPOL		0x01
#define MCP0_GPINTEN	0x02
#define MCP0_DEFVAL		0x03
#define MCP0_INTCON		0x04
#define MCP0_IOCON		0x05
#define MCP0_GPPU		0x06
#define MCP0_INTF		0x07
#define MCP0_INTCAP		0x08
#define MCP0_GPIO 		0x09
#define MCP0_OLAT 		0x0A

// Bits del MCP0
#define MCP0_GPIO_IGPRSDCD			1	// IN
#define MCP0_GPIO_IGPRSRI			2	// IN
#define MCP0_GPIO_OGPRSSW			3	// OUT
#define MCP0_GPIO_OTERMPWR			4
#define MCP0_GPIO_OGPRSPWR			5
#define MCP0_GPIO_OLED				6

//------------------------------------------------------------------------------------
// Registros MCP1
#define MCP1_IODIR		0x00
#define MCP1_IPOL		0x01
#define MCP1_GPINTEN	0x02
#define MCP1_DEFVAL		0x03
#define MCP1_INTCON		0x04
#define MCP1_IOCON		0x05
#define MCP1_GPPU		0x06
#define MCP1_INTF		0x07
#define MCP1_INTCAP		0x08
#define MCP1_GPIO 		0x09
#define MCP1_OLAT 		0x0A

// Bits del MCP1
#define MCP1_GPIO_DIN0				2	// IN
#define MCP1_GPIO_DIN1				3	// IN
#define MCP1_GPIO_PWRSENSORS		6	// OUT

//------------------------------------------------------------------------------------
// Registros MCP2
#define MCP2_IODIRA					0x00
#define MCP2_IODIRB					0x01
#define MCP2_GPINTENA				0x04
#define MCP2_GPINTENB				0x05
#define MCP2_DEFVALA				0x06
#define MCP2_DEFVALB				0x07
#define MCP2_INTCONA				0x08
#define MCP2_INTCONB				0x09
#define MCP2_IOCON					0x0A
#define MCP2_GPPUA					0x0C
#define MCP2_GPPUB					0x0D
#define MCP2_INTFA					0x0E
#define MCP2_INTFB					0x0F
#define MCP2_INTCAPA				0x10
#define MCP2_INTCAPB				0x11
#define MCP2_GPIOA					0x12
#define MCP2_GPIOB					0x13
#define MCP2_OLATA					0x14
#define MCP2_OLATB					0x15

// Bits del MCP2
#define MCP2_ENA2						0
#define MCP2_ENB2						1
#define MCP2_PHB2						2
#define MCP2_PHB1						3
#define MCP2_ENB1						4
#define MCP2_ENA1						5
#define MCP2_PHA1						6

#define MCP2_RESET						0
#define MCP2_SLEEP						1
#define MCP2_FAULT						2
#define MCP2_PHA2						3
#define MCP2_PWRSENSORS					4
#define MCP2_DIN0						5
#define MCP2_DIN1						6
#define MCP2_OANALOG					7

#define MCP2_GPIO_DIN0					6	// IN
#define MCP2_GPIO_DIN1					5	// IN
#define MCP2_GPIO_PWRSENSORS			4	// OUT
#define MCP2_GPIO_ANALOGPWR				7	// OUT
//------------------------------------------------------------------------------------

typedef enum { MCP_NONE, MCP23008, MCP23018 } t_mcpModules;

void MCP_init(void);
//
s08 MCP_queryDcd( u08 *pin);
s08 MCP_queryRi( u08 *pin);
s08 MCP_queryDin0( u08 *pin);
s08 MCP_queryDin1( u08 *pin);
//
s08 MCP_setLed( u08 value );
s08 MCP_setGprsPwr( u08 value );
s08 MCP_setGprsSw( u08 value );
s08 MCP_setTermPwr( u08 value );
s08 MCP_setSensorPwr( u08 value );

s08 MCP_setOutsResetPin( u08 value );
#define MCP_outputsReset() MCP_setOutsResetPin(0)
#define MCP_outputsNoReset() MCP_setOutsResetPin(1)

s08 MCP_setOutsSleepPin( u08 value );
#define MCP_outputsSleep() MCP_setOutsSleepPin(0)
#define MCP_outputsNoSleep() MCP_setOutsSleepPin(1)

s08 MCP_setOutsEnablePin(  u08 outId, u08 value );
#define MCP_output0Enable() MCP_setOutsEnablePin(0,1)
#define MCP_output0Disable() MCP_setOutsEnablePin(0,0)
#define MCP_output1Enable() MCP_setOutsEnablePin(1,1)
#define MCP_output1Disable() MCP_setOutsEnablePin(1,0)
#define MCP_output2Enable() MCP_setOutsEnablePin(2,1)
#define MCP_output2Disable() MCP_setOutsEnablePin(2,0)
#define MCP_output3Enable() MCP_setOutsEnablePin(3,1)
#define MCP_output3Disable() MCP_setOutsEnablePin(3,0)

s08 MCP_setOutsPhase(  u08 outId, u08 value );
#define MCP_output0Phase_01() MCP_setOutsPhase(0,1)
#define MCP_output0Phase_10() MCP_setOutsPhase(0,0)
#define MCP_output1Phase_01() MCP_setOutsPhase(1,0)
#define MCP_output1Phase_10() MCP_setOutsPhase(1,1)
#define MCP_output2Phase_01() MCP_setOutsPhase(2,1)
#define MCP_output2Phase_10() MCP_setOutsPhase(2,0)
#define MCP_output3Phase_01() MCP_setOutsPhase(3,0)
#define MCP_output3Phase_10() MCP_setOutsPhase(3,1)

u08 getMcpDevice(void);

#endif /* AVRLIBFRTOS_MCP_SP5KFRTOS_H_ */


#ifndef VT100_H
#define VT100_H

#include "avrlibdefs.h"
#include "avrlibtypes.h"
#include "global.h"
#include "rprintf_sp5KFRTOS.h"

// constants/macros/typdefs
// text attributes
#define VT100_ATTR_OFF		0
#define VT100_BOLD			1
#define VT100_USCORE		4
#define VT100_BLINK			5
#define VT100_REVERSE		7
#define VT100_BOLD_OFF		21
#define VT100_USCORE_OFF	24
#define VT100_BLINK_OFF		25
#define VT100_REVERSE_OFF	27

// functions

//! vt100Init() initializes terminal and vt100 library
///		Run this init routine once before using any other vt100 function.
void vt100Init(u08 nUart);

//! vt100ClearScreen() clears the terminal screen
void vt100ClearScreen(u08 nUart);

#endif

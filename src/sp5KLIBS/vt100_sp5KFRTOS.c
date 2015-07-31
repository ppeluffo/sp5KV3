/*! \file vt100.c \brief VT100 terminal function library. */
//*****************************************************************************
//
// File Name	: 'vt100.c'
// Title		: VT100 terminal function library
// Author		: Pascal Stang - Copyright (C) 2002
// Created		: 2002.08.27
// Revised		: 2002.08.27
// Version		: 0.1
// Target MCU	: Atmel AVR Series
// Editor Tabs	: 4
//
// NOTE: This code is currently below version 1.0, and therefore is considered
// to be lacking in some functionality or documentation, or may not be fully
// tested.  Nonetheless, you can expect most functions to work.
//
// This code is distributed under the GNU Public License
//		which can be found at http://www.gnu.org/licenses/gpl.txt
//
//*****************************************************************************

//#include "sp5KFRTOS.h"

// Program ROM constants

// Global variables

#include "vt100_sp5KFRTOS.h"

// Functions
void vt100Init(u08 nUart)
{
	// initializes terminal to "power-on" settings
	// ESC c
	rprintfProgStrM(nUart, "\x1B\x63");
}

void vt100ClearScreen(u08 nUart)
{
	// ESC [ 2 J
	rprintfProgStrM(nUart, "\x1B[2J");
}


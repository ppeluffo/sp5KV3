/*------------------------------------------------------------------------------------
 * rprintf_sp5KFRTOS.h
 * Autor: Pablo Peluffo @ 2015
 * Basado en Proycon AVRLIB de Pascal Stang.
 *
 * Modifico las funciones de impresion para adaptarlas al sistema SP5K
 * y al FRTOS.
 * Aseguro la exclusividad del acceso a los dispositivos UART por medio
 * de semaforos MUTEX, creados localmente por lo que quedan en un ambito privado.
 *
 * Solo implemento funciones de imprimir strings y no de formateo general como rprintf.
 * El modelo usado es que el formateo se haga con sprintf y luego se imprima el string
 * correspondiente.
 *
 * Las funciones son todas reentrantres.
 *
 * USO: antes que nada deben inicializarse con las funciones de init ya que estan crean los
 * semaforos.
 *
 */
// needed for use of PSTR below
#include <avr/pgmspace.h>

#include "avrlibdefs.h"
#include "avrlibtypes.h"
#include "global.h"
#include "uart2_sp5KFRTOS.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "queue.h"
#include "task.h"


// Semaforos mutex
SemaphoreHandle_t semph_GPRS, semph_TERM;

/*------------------------------------------------------------------------------------*/
/* Funciones de inicializacion. Deben ser usadas ya que son las que
 * crean los semaforos correpondientes.
 */
void rprintfInit(u08 nUart);
void GPRSrprintfInit(void);
void TERMrprintfInit(void);

/* Imprimen un solo caracter. Son las funciones basicas de impresion, utilizadas
 * por el resto de las funciones.
 * Como utilizan semaforos, defino 2 nuevas similares con prefijo pv que no usan
 * semaforos, y son las que deben usarse desde aquellas que imprimen strings
 */
void rprintfChar(u08 nUart, unsigned char c);
void GPRSrprintfChar(unsigned char c);
void pvGPRSrprintfChar(unsigned char c);
void TERMrprintfChar(unsigned char c);
void pvTERMrprintfChar(unsigned char c);

/* Imprimen un string terminado en NULL, almacenado en RAM
 */
void rprintfStr(u08 nUart, char str[]);
void GPRSrprintfStr( char str[]);
void TERMrprintfStr( char str[]);

/* Idem pero el string esta almacenado en ROM.
 * OJO: No almacena el string en ROM sino que asume que esta almacenado correctamente
 * (prefijar con PSTR)
 * Usando las funciones rprintfProgStrM(...), automaticamente hace que es string se
 * almacene en ROM, no desperdiciando RAM
 */
void rprintfProgStr(u08 nUart, const prog_char str[]);
void TERMrprintfProgStr( const prog_char str[]);
void GPRSrprintfProgStr( const prog_char str[]);

#define rprintfProgStrM(nUart, string)			(rprintfProgStr(nUart, PSTR(string)))
#define GPRSrprintfProgStrM(string)			(GPRSrprintfProgStr(PSTR(string)))
#define TERMrprintfProgStrM(string)			(TERMrprintfProgStr(PSTR(string)))


void rprintfCRLF(u08 nUart);
void GPRSrprintfCRLF(void);
void TERMrprintfCRLF(void);

/* Funciones que trasmiten un caracter sin hacer conversion CR-LF
 * Se utiliza en cmdline
 */
void pvGPRSoutChar(unsigned char c);
void pvTERMoutChar(unsigned char c);

/*------------------------------------------------------------------------------------*/

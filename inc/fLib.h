/***************************************************************************
* Copyright  Faraday Technology Corp 2002-2003.  All rights reserved.      *
*--------------------------------------------------------------------------*
* Name:fLib.h                                                              *
* Description: Faraday code library define                                 *
****************************************************************************/

#ifndef __fLib_h                /* Only include stuff once */
#define __fLib_h


#include "sl231x.h"
#include "sizes.h"
#include "typedef.h"
#include "except.h"
//#include "filectrl.h"
//#include "timer.h"

extern UINT32 CPU_TYPE; 

/*IP library define */


#define ROM_Start   (0x80000000) // after remap
#define ROM_End    	(0x80080000) // after remap


#define FILL_CONTENT_ADDRESS (0x00000001)
#define FILL_CONTENT_TYPE1   (0xffff0000)
#define FILL_CONTENT_TYPE2   (0x0000ffff)
#define FILL_CONTENT_TYPE3   (0xffffffff)
#define FILL_CONTENT_TYPE4   (0x5a5a5a5a)
#define FILL_CONTENT_TYPE5   (0xa5a5a5a5)


#define EXIT_TUBE_FAIL   0x66
#define EXIT_TUBE_PASS   0x88
#define EXIT_TUBE_FINISH 0x99


/*  -------------------------------------------------------------------------------
 *   define in boot.s
 *  -------------------------------------------------------------------------------
 */
 
extern void fLib_InitBSSMemory(void);

extern int  ReadMode(void);
extern void WriteMode(int);

extern void EnableFIQ(void);
extern void DisableFIQ(void);
extern void EnableIRQ(void);
extern void DisableIRQ(void);


/*  -------------------------------------------------------------------------------
 *   define in cache.c
 *  -------------------------------------------------------------------------------
 */
 
extern void fLib_EnableCache(void);
extern void fLib_DisableCache(void);
extern void fLib_EnableICache(void);
extern void fLib_DisableICache(void);
extern void fLib_EnableDCache(void);
extern void fLib_DisableDCache(void);
extern void fLib_EnableWriteBuffer(void);
extern void fLib_DisableWriteBuffer(void);
 

/*  -------------------------------------------------------------------------------
 *   define in board.c
 *  -------------------------------------------------------------------------------
 */

//extern fLib_Date Curr_Date;
//extern fLib_Time Curr_Time;

extern UINT32 inw(UINT32 *port);
extern void   outw(UINT32 *port,UINT32 data);

extern UINT16 inhw(UINT16 *port);
extern void outhw(UINT16 *port,UINT16 data);
extern UINT8 inb(UINT8 *port);
extern void outb(UINT8 *port,UINT8 data);

extern void   Show_Number(UINT32 number);
extern void   Show_HexNumber(UINT32 number);
extern void   Do_Delay(UINT32 num);

extern UINT32 fLib_ChangeCPUMode(UINT32 CPUMode);

extern UINT32 fLib_SysElapseTime(void);
extern void   fLib_HardwareInit(void);

extern void Timer1_Tick(void);
extern UINT64 fLib_CurrentT1Tick(void);
extern void Timer2_Tick(void);
extern UINT64 fLib_CurrentT2Tick(void);
extern void Timer3_Tick(void);
extern UINT64 fLib_CurrentT3Tick(void);
 

/*  -------------------------------------------------------------------------------
 *   define in string.c
 *  -------------------------------------------------------------------------------
 */

extern UINT32 s2hex(char *s);
extern int    c2hex(char c);
extern int    validhex(char *s);
extern INT32  catoi(const char c);


/*  -------------------------------------------------------------------------------
 *   define in debug.c
 *  -------------------------------------------------------------------------------
 */
 
void 	fLib_DebugPrintChar(INT8 ch);
void 	fLib_DebugPrintString(INT8 *str);
INT8 	fLib_DebugGetChar(void);
UINT32 	fLib_DebugGetUserCommand(INT8 *buffer, UINT32 Len);


/* -------------------------------------------------------------------------------
 *  LED  definitions
 * -------------------------------------------------------------------------------
 */
 
#define LED_ON                      	0
#define LED_OFF                     	1
#define NUM_OF_LEDS                 	16
#define DBG_LEDS                    	(CPE_GPIO_BASE + GPIO_DOUT_OFFSET)
#define LED_BASE                        DBG_LEDS

#undef LPC_IT8712						
//#define LPC_IT8712		1
#endif /* __fLib_h define if */

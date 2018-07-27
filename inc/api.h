/****************************************************************************
* Copyright  Storlink Corp 2005.  All rights reserved.                
*--------------------------------------------------------------------------
* Name			: api.h
* Description	: 
*		Define API
*
* History
*
*	Date		Writer		Description
*	-----------	-----------	-------------------------------------------------
*	04/25/2005	Gary Chen	Create
*
****************************************************************************/
#ifndef _API_H
#define _API_H

#define REG8(addr)      (*(volatile unsigned char * const)(addr))
#define REG16(addr)     (*(volatile unsigned short * const)(addr))
#define REG32(addr)     (*(volatile unsigned long  * const)(addr))

#define HAL_READ_UINT8(addr, value)		((value) = *((volatile UINT8 *)(addr)))
#define HAL_WRITE_UINT8(addr, value)	(*((volatile UINT8 *)(addr)) = (value))
#define HAL_READ_UINT16(addr, value)	((value) = *((volatile UINT16 *)(addr)))
#define HAL_WRITE_UINT16(addr, value)	(*((volatile UINT16 *)(addr)) = (value))
#define HAL_READ_UINT32(addr, value)	((value) = *((volatile UINT32 *)(addr)))
#define HAL_WRITE_UINT32(addr, value)	(*((volatile UINT32 *)(addr)) = (value))

#define min(x,y)	((x<=y) ? x : y) 
#define max(x,y)	((x>=y) ? x : y)

#define strcpy 			sys_strcpy
#define strlen			sys_strlen
#define memset 			sys_memset
#define memcpy 			sys_memcpy
#define memcmp 			sys_memcmp
#define memicmp 		sys_memicmp
#define malloc 			sys_malloc
#define calloc 			sys_calloc
#define free 			sys_free
#define toupper			sys_toupper
#define tolower			sys_lower
#define printf			sys_printf
#define sprintf			sys_sprintf
#define strcmp			sys_strcmp
#define strncmp			sys_strncmp
#define strncasecmp		sys_strncasecmp
#define stricmp			sys_strncasecmp
#define sscanf			sys_sscanf
#define strstr			sys_strstr
#define strrchr			sys_strrchr
#define strchr			sys_strchr

#define dbg_printf(arg)	{printf("[%s %d %s] ",__FILE__, __LINE__, __FUNCTION__); printf arg;}

#ifdef __ORIGINAL__
extern UINT64 hal_get_ticks(void);
#else
extern unsigned long long hal_get_ticks(void);
#endif
#define sys_get_ticks	hal_get_ticks

/*--------------------------------------------------------------
* 	sys_toupper
---------------------------------------------------------------*/
static inline char sys_toupper(char data)
{
	return ((data >= 'a') && (data <= 'z')) ? data-0x20 : data;
}

/*--------------------------------------------------------------
* 	sys_lower
---------------------------------------------------------------*/
static inline char sys_lower(char data)
{
	return ((data >= 'A') && (data <= 'Z')) ? data+0x20 : data;
}

#define isdigit(c) (c >= '0' && c <= '9') ? TRUE : FALSE
#define ishex(c) ((c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f')) ? TRUE : FALSE

#define BOOT_BREAK_KEY			0x03	// CTRL-C

#ifdef __ORIGINAL__
UINT32 char2hex(UINT8 c);
UINT32 char2decimal(UINT8 c);
UINT32 str2hex(UINT8 *cp);
UINT32 str2decimal(UINT8 *cp);
UINT32 str2value(UINT8 *strp);
UINT32 str2ip(UINT8 *cp);
#else
unsigned long char2hex(unsigned char c);
unsigned long char2decimal(unsigned char c);
unsigned long str2hex(unsigned char *cp);
unsigned long str2decimal(unsigned char *cp);
unsigned long str2value(unsigned char *strp);
unsigned long str2ip(unsigned char *cp);
#endif
char ui_getc(void);
int ui_get_confirm_key(void);
int ui_gets(char *buf, int size);

#endif // _API_H

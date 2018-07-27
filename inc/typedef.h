/****************************************************************************
* Copyright  Storlink Corp 2005.  All rights reserved.                
*--------------------------------------------------------------------------
* Name			: typedef.h
* Description	: 
*		Define data type
*
* History
*
*	Date		Writer		Description
*	-----------	-----------	-------------------------------------------------
*	04/18/2005	Gary Chen	Create
*
****************************************************************************/
#ifndef _TYPEDEF_H
#define _TYPEDEF_H

typedef	unsigned long	ULONG;
typedef	unsigned long	DWORD;
typedef	unsigned long	uint32;
typedef	unsigned long	u_int32;
typedef	unsigned long	UINT32;
typedef	unsigned long	UINT;
typedef	unsigned long	u32;
typedef	unsigned long	__u32;

typedef	unsigned short	USHORT;
typedef unsigned short  WORD;
typedef unsigned short  USS;
typedef unsigned short  uint16;
typedef unsigned short  u_int16;
typedef unsigned short  UINT16;
typedef	unsigned short	u16;
typedef	unsigned short	__u16;

typedef	unsigned char	UCHAR;
typedef	unsigned char	BYTE;
typedef	unsigned char	USC;
typedef	unsigned char	uint8;
typedef	unsigned char	u_int8;
typedef	unsigned char	UINT8;
typedef	unsigned char	u8;
typedef	unsigned char	__u8;
typedef unsigned char   BOOLEAN;
typedef unsigned char   INT8U;
typedef unsigned char	BOOL;

typedef	char			CHAR;
typedef	short			SHORT;
typedef	int				INT;
typedef long			INT32;
typedef short			INT16;
typedef	char			INT8;

typedef	long long				INT64;
typedef	unsigned long long		UINT64;
typedef	unsigned long long		u64;
typedef	unsigned long long		__u64;
typedef	unsigned long long		sector_t;

#define size_t			int

#define VOID            void
#define LONG            int
#define ULONGLONG       u64
typedef VOID            *PVOID;
typedef char            *PCHAR;
typedef UCHAR           *PUCHAR;
typedef LONG            *PLONG;
typedef ULONG           *PULONG;

#define BOOLEAN         u8
#define bool			int

#ifndef FALSE
#define FALSE			0
#define TRUE			1
#endif    

#ifndef false
#define false			0
#define true			1
#endif    

#define FAILED			0
#define	OK				1
#define NOT_CHANGED		2

#ifndef NULL
#define NULL    		(void *)0
#endif

#define BIT(x)			(0x1 << x)

#define __GNU_PACKED __attribute__ ((packed))

#define IPIV(a,b,c,d) 	((a<<24)+(b<<16)+(c<<8)+d)
#define IP1(a)        	((a>>24)&0xff)
#define IP2(a)        	((a>>16)&0xff)
#define IP3(a)        	((a>>8)&0xff)
#define IP4(a)        	((a)&0xff)
#define IPIV_NUM(x)		IP1(x),IP2(x),IP3(x),IP4(x)	

#endif // _TYPEDEF_H

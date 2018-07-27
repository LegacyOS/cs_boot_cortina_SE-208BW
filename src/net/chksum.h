/****************************************************************************
* Copyright  Storlink Corp 2005.  All rights reserved.                
*--------------------------------------------------------------------------
* Name			: chksum.h
* Description	: 
*		Handle checksum for inet
*
* History
*
*	Date		Writer		Description
*	-----------	-----------	-------------------------------------------------
*	04/26/2005	Gary Chen	Create
*
****************************************************************************/
#ifndef _CHKSUM_H
#define _CHKSUM_H

static inline u16 ip_csum(u16 *w, int len, int init_sum)
{
	int sum = init_sum;

    union {
		volatile u8 c[2];
		volatile u16 s;
	} su;

    union {
		volatile u16 s[2];
		volatile int i;
    } iu;

    while ((len -= 2) >= 0)
		sum += *w++;

    if (len == -1)
    {
		su.c[0] = *(char *)w;
		su.c[1] = 0;
		sum += su.s;
    }

    iu.i = sum;
    sum = iu.s[0] + iu.s[1];
    if (sum > 65535)
	sum -= 65535;

    su.s = ~sum;

    return (su.c[0] << 8) | su.c[1];
}

#endif // _CHKSUM_H


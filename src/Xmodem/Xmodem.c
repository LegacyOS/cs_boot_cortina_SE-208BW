//==========================================================================
//
//      xyzModem.c
//
//      RedBoot stream handler for xyzModem protocol
//
//==========================================================================
//####ECOSGPLCOPYRIGHTBEGIN####
// -------------------------------------------
// This file is part of eCos, the Embedded Configurable Operating System.
// Copyright (C) 1998, 1999, 2000, 2001, 2002 Red Hat, Inc.
// Copyright (C) 2002 Gary Thomas
//
// eCos is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 or (at your option) any later version.
//
// eCos is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
// for more details.
//
// You should have received a copy of the GNU General Public License along
// with eCos; if not, write to the Free Software Foundation, Inc.,
// 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
//
// As a special exception, if other files instantiate templates or use macros
// or inline functions from this file, or you compile this file and link it
// with other works to produce a work based on this file, this file does not
// by itself cause the resulting work to be covered by the GNU General Public
// License. However the source code for this file must still be made available
// in accordance with section (3) of the GNU General Public License.
//
// This exception does not invalidate any other reasons why a work based on
// this file might be covered by the GNU General Public License.
//
// Alternative licenses for eCos may be arranged by contacting Red Hat, Inc.
// at http://sources.redhat.com/ecos/ecos-license/
// -------------------------------------------
//####ECOSGPLCOPYRIGHTEND####
//==========================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):    gthomas
// Contributors: gthomas, tsmith, Yoshinori Sato
// Date:         2000-07-14
// Purpose:      
// Description:  
//              
// This code is part of RedBoot (tm).
//
//####DESCRIPTIONEND####
//
//==========================================================================
#include <define.h>
#include <board_config.h>
#include "Xmodem.h"

#define Xmodem_putc(x)					uart_putc(x)
#define Xmodem_delay_us(x)				hal_delay_us(x)

XMODEM_T	xmodem_info;

/*----------------------------------------------------------------------
* Xmodem_getc
*	To get a character form console port
*----------------------------------------------------------------------*/
static int Xmodem_getc(char *c)
{
	UINT64	delay_time;
	UINT32	delay_ticks;
	
	delay_ticks = (XMODEM_INPUT_TIMEOUT * BOARD_TPS) / 1000;
	delay_time = sys_get_ticks() + delay_ticks;
	
	while (sys_get_ticks() < delay_time)
	{
		if ((uart_scanc(c)))
		{
    		return(1);
    	}
	}
	return(0);
}

/*----------------------------------------------------------------------
* Xmodem_flush
*	To kill all input characters form console port
*----------------------------------------------------------------------*/
static void Xmodem_flush(void)
{
    char c;
    
    while (true) 
    {
        if (!Xmodem_getc(&c))
        	return;
    }
}

/*----------------------------------------------------------------------
* Xmodem_get_header
*	To get Xmodem header
*----------------------------------------------------------------------*/
static int Xmodem_get_header(XMODEM_T *xmodem)
{
	char c;
    int res;
    int hdr_found = FALSE;
    int i, can_total, hdr_chars;
    unsigned short cksum;

    can_total = 0;
    hdr_chars = 0;

    if (xmodem->tx_ack) 
    {
        Xmodem_putc(XMODEM_ACK);
        xmodem->tx_ack_cnt++;
        xmodem->tx_ack = false;
    }
    while (!hdr_found) 
    {
        res = Xmodem_getc(&c);
        if (res) 
		{
            hdr_chars++;
            switch (c) 
            {
            	case XMODEM_SOH:
                	xmodem->total_XMODEM_SOH++;
            	case XMODEM_STX:
                	if (c == XMODEM_STX) xmodem->total_XMODEM_STX++;
                	hdr_found = true;
                	break;
            	case XMODEM_CAN:
                	xmodem->total_XMODEM_CAN++;
                	if (++can_total == XMODEM_CAN_COUNT) 
                	{
                    	return XMODEM_ERR_CANCEL;
                	} 
                	else 
                	{
                    	// Wait for multiple XMODEM_CAN to avoid early quits
                    	break;
                	}
            	case XMODEM_EOT:
                	// XMODEM_EOT only supported if no noise
                	if (hdr_chars == 1) 
                	{
                    	Xmodem_putc(XMODEM_ACK);
        				xmodem->tx_ack_cnt++;
                    	return XMODEM_ERR_EOF;
                	}
            	default:
                	// Ignore, waiting for start of header
                	;
            }
        } else {
            // Data timed out
            Xmodem_flush();  // Toss any current input
            // Xmodem_delay_us((LONG)10000);
            return XMODEM_ERR_TIMEOUT;
        }
    }

    // Header found, now read the data
    if (!Xmodem_getc(&xmodem->blk))
        return XMODEM_ERR_CHKSUM;

    if (!Xmodem_getc(&xmodem->cblk))
        return XMODEM_ERR_CHKSUM;

    xmodem->len = (c == XMODEM_SOH) ? 128 : 1024;
    xmodem->bufp = xmodem->pkt;
    for (i = 0;  i < xmodem->len;  i++) 
    {
        if (Xmodem_getc(&c))
            xmodem->pkt[i] = c;
        else 
            return XMODEM_ERR_TIMEOUT;
    }
    if (!Xmodem_getc(&xmodem->crc1))
        return XMODEM_ERR_TIMEOUT;
    
    if (xmodem->crc_mode) 
    {
        if (!Xmodem_getc(&xmodem->crc2))
            return XMODEM_ERR_TIMEOUT;
    }
    
    // Validate the message
    if ((xmodem->blk ^ xmodem->cblk) != (unsigned char)0xFF) 
    {
        Xmodem_flush();
        return XMODEM_ERR_FRAME;
    }
    // Verify checksum/CRC
    if (xmodem->crc_mode) 
    {
        cksum = Xmodem_crc16(xmodem->pkt, xmodem->len);
        if (cksum != ((xmodem->crc1 << 8) | xmodem->crc2))
            return XMODEM_ERR_CHKSUM;
    } 
    else 
    {
        cksum = 0;
        for (i = 0;  i < xmodem->len;  i++)
            cksum += xmodem->pkt[i];
        
        if (xmodem->crc1 != (cksum & 0xFF))
            return XMODEM_ERR_CHKSUM;
    }
    // If we get here, the message passes [structural] muster
    xmodem->good_cnt++;
    return 0;
}

int Xmodem_open(int *err)
{
    int console_chan, stat;
    int retries = XMODEM_MAX_RETRIES;
    int crc_retries = XMODEM_MAX_RETRIES_WITH_CRC;
	XMODEM_T *xmodem;
	
	xmodem = (XMODEM_T *)&xmodem_info;
    memset((char *)xmodem, 0, sizeof(XMODEM_T));
    xmodem->crc_mode = TRUE;

    while (retries-- > 0)
    {
        stat = Xmodem_get_header(xmodem);
        if (stat == 0) 
        {
#if 0 // 2006/1/16 01:18PM
            if (xmodem->blk == 0) 
            {
                xmodem->tx_ack = true;
            	xmodem->next_blk = 1;
            	xmodem->len = 0;
            }
            else
#endif            
            {
				xmodem->next_blk = xmodem->blk + 1;
				xmodem->tx_ack = true;
            }
            return 0;
        } 
        else if (stat == XMODEM_ERR_TIMEOUT)
        {
            if (--crc_retries <= 0)
            	xmodem->crc_mode = false;
            Xmodem_delay_us(2*100000);   // Extra delay for startup
            Xmodem_putc(xmodem->crc_mode ? 'C' : XMODEM_NAK);
            xmodem->total_retries++;
        }
        // if (stat == XMODEM_ERR_CANCEL)
        else
        {
            break;
        }
    }
    *err = stat;
    return -1;
}

int Xmodem_read(char *buf, int size, int *err)
{
    int stat, total, len;
    int retries;
	XMODEM_T *xmodem;
	
	xmodem = (XMODEM_T *)&xmodem_info;

    total = 0;
    stat = XMODEM_ERR_CANCEL;
    // Try and get 'size' bytes into the buffer
    while (!xmodem->at_eof && (size > 0))
    {
        if (xmodem->len == 0)
        {
            retries = 5;	// XMODEM_MAX_RETRIES;
            while (retries-- > 0)
            {
                stat = Xmodem_get_header(xmodem);
                if (stat == 0)
                {
                    if (xmodem->blk == xmodem->next_blk)
                    {
                        xmodem->tx_ack = true;
                        xmodem->next_blk = (xmodem->next_blk + 1) & 0xFF;
                        // Data blocks can be padded with ^Z (XMODEM_EOF) characters
                        // This code tries to detect and remove them
                        if ((xmodem->bufp[xmodem->len-1] == XMODEM_EOF) &&
                             (xmodem->bufp[xmodem->len-2] == XMODEM_EOF) &&
                             (xmodem->bufp[xmodem->len-3] == XMODEM_EOF))
                        {
							while (xmodem->len && (xmodem->bufp[xmodem->len-1] == XMODEM_EOF))
							{
								xmodem->len--;
							}
                        }
                        break;
                    } 
                    else if (xmodem->blk == ((xmodem->next_blk - 1) & 0xFF))
                    {
                        // Just re-ACK this so sender will get on with it
                        Xmodem_putc(XMODEM_ACK);
						xmodem->tx_ack_cnt++;
                        continue;  // Need new header
                    } else {
                        stat = XMODEM_ERR_SEQUENCE;
                    }
                }
                else if (stat == XMODEM_ERR_EOF)
                {
                    Xmodem_putc(XMODEM_ACK);
        			xmodem->tx_ack_cnt++;
                    xmodem->at_eof = true;
                    break;
                }
                else if (stat != XMODEM_ERR_TIMEOUT)
                	break;
                	
                Xmodem_putc(xmodem->crc_mode ? 'C' : XMODEM_NAK);
                xmodem->total_retries++;
            }
            if (stat < 0)
            {
                *err = stat;
                xmodem->len = -1;
                return total;
            }
        }
        // Don't "read" data from the XMODEM_EOF protocol package
        if (!xmodem->at_eof)
        {
            len = xmodem->len;
            if (size < len) len = size;
            memcpy(buf, xmodem->bufp, len);
            size -= len;
            buf += len;
            total += len;
            xmodem->len -= len;
            xmodem->bufp += len;
        }
    }
    return total;
}

void
Xmodem_close(int *err)
{
#if 0	
	XMODEM_T *xmodem;
	
	xmodem = (XMODEM_T *)&xmodem_info;
    printf("Xmodem - %s mode, %d(XMODEM_SOH)/%d(XMODEM_STX)/%d(XMODEM_CAN) packets, %d retries\n", 
                xmodem->crc_mode ? "CRC" : "Cksum",
                xmodem->total_XMODEM_SOH, xmodem->total_XMODEM_STX, xmodem->total_XMODEM_CAN,
                xmodem->total_retries);
#endif                
}

// Need to be able to clean out the input buffer, so have to take the
// getc
void Xmodem_terminate(int abort)
{
	unsigned char c;
	XMODEM_T *xmodem;
	
	xmodem = (XMODEM_T *)&xmodem_info;

	if (abort) 
	{
		Xmodem_putc(XMODEM_CAN);
		Xmodem_putc(XMODEM_CAN);
		Xmodem_putc(XMODEM_CAN);
		Xmodem_putc(XMODEM_CAN);
		Xmodem_putc(XMODEM_BSP);
		Xmodem_putc(XMODEM_BSP);
		Xmodem_putc(XMODEM_BSP);
		Xmodem_putc(XMODEM_BSP);
		Xmodem_flush();
		xmodem->at_eof = true;
	}
	else
	{
		
		while (uart_scanc(&c) > 0) ;
		Xmodem_delay_us((LONG)250000);
	}
}

/*----------------------------------------------------------------------
* Xmodem_get_error_msg
*----------------------------------------------------------------------*/
char *Xmodem_get_error_msg(int err)
{
    switch (err) {
    case XMODEM_ERR_TIMEOUT:
        return "Timed out";
        break;
    case XMODEM_ERR_EOF:
        return "End of file";
        break;
    case XMODEM_ERR_CANCEL:
        return "Cancelled";
        break;
    case XMODEM_ERR_FRAME:
        return "Invalid framing";
        break;
    case XMODEM_ERR_CHKSUM:
        return "CRC/checksum error";
        break;
    case XMODEM_ERR_SEQUENCE:
        return "Block sequence error";
        break;
    default:
        return "Unknown error";
        break;
    }
}


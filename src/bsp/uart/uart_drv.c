/****************************************************************************
* Copyright  Storlink Corp 2005.  All rights reserved.                
*--------------------------------------------------------------------------
* Name			: uart_drv.c
* Description	: 
*		Handle UART functions
*
* History
*
*	Date		Writer		Description
*	-----------	-----------	-------------------------------------------------
*	04/19/2005	Gary Chen	Create and implement from Jason's Redboot code
*
****************************************************************************/
#include <stdarg.h>
#include <define.h>
#include <board_config.h>
#include <sl2312.h>
#include "uart.h"
#include "lpc.h"

typedef struct {
	UINT32	overrun_err_cnt;	// Overrun Error Counter
	UINT32	break_condet_cnt;	// Break Condition Detect Counter
	UINT32	parity_err_cnt;		// Parity Error Counter
	UINT32	frame_err_cnt;		// Frame Error Counter
	UINT32	no_rxbuf_cnt;		// No free rx buffer
} UART_STATISTICS_T;

#define UART_RXBUF_SIZE			256
typedef struct {
	UINT32	total;	
	UINT32	in_idx;		
	UINT32	out_idx;	
	UINT8	buf[UART_RXBUF_SIZE];
} UART_BUF_T;

typedef struct {
	int					IT8712_existed;
	UINT32				base;
	int					baud_rate;
	UINT32				baud_clk;
	int					isr_vector;
	UART_BUF_T			rxbuf;
	UART_STATISTICS_T	stat;
} UART_INFO_T;

UART_INFO_T uart_info;

static void uart_init_channel(UART_INFO_T *uart);
void uart_enable_interrupt(void);
void uart_disable_interrupt(void);
void uart_puts(unsigned char *datap);

#define UART_ENABLE_INTERRUPT()
#define UART_DISABLE_INTERRUPT()

extern int astel_serial_RS485;					// dhsul ASTEL

/*------------------------------------------------------------------------------
* 	uart_init
-----------------------------------------------------------------------------+*/
void uart_init(int baud_rate)
{
	UART_INFO_T *uart;
	UINT32		base;
	UINT8		lcr;
	
	uart = (UART_INFO_T *)&uart_info;
	memset((char *)uart, 0, sizeof(UART_INFO_T));
	uart->baud_rate = baud_rate;

	// change baud rate to clock
	switch (baud_rate)
	{
		case 9600:		uart->baud_clk = SL2312_BAUD_9600; break;
		case 14400:		uart->baud_clk = SL2312_BAUD_14400; break;
		case 19200:		uart->baud_clk = SL2312_BAUD_19200; break;
		case 38400:		uart->baud_clk = SL2312_BAUD_38400; break;
		case 57600: 	uart->baud_clk = SL2312_BAUD_57600; break;
		case 115200: 	uart->baud_clk = SL2312_BAUD_115200; break;
		default:		uart->baud_clk = SL2312_BAUD_19200; break;
	}
	
#ifdef LPC_IT8712
    // init channel 1
    uart->base = (UINT32)(LPC_BASE);

    if (SearchIT8712() == 0) 
    {
    	uart->IT8712_existed = 1;
    	LPCSetConfig(0, 0x02, 0x01);
    	LPCSetConfig(LDN_SERIAL1, 0x30, 1);
    	LPCSetConfig(LDN_SERIAL1, 0x23, 0x00);
    	uart->isr_vector = (unsigned int) LPCGetConfig(LDN_SERIAL1, 0x70) + LPC_IRQ_BASE;
    	base = (UINT32) LPCGetConfig(LDN_SERIAL1, 0x60);
    	base = (base << 8) | ((UINT32) LPCGetConfig(LDN_SERIAL1, 0x61));
    	base = uart->base = (UINT32)(LPC_BASE + base);
    	//uart->baud_rate = 0x38400 ;
    	
    	// Disable interrupts.
    	// UART_DISABLE_INTERRUPT();
    	
    	HAL_READ_UINT8(base + UART_LCR, lcr);
    	lcr &= ~UART_LCR_DLAB;
    	HAL_WRITE_UINT8(base + UART_LCR, UART_LCR_DLAB);
    	HAL_WRITE_UINT8(base + UART_DLM, 6 >> 8);
    	HAL_WRITE_UINT8(base + UART_DLL, 6);
    	lcr &= 0xc0;
    	lcr |= UART_LCR_STOP;
    	lcr |= UART_LCR_WLEN8;
    	HAL_WRITE_UINT8(base + UART_LCR, lcr);
    	HAL_WRITE_UINT8(base + UART_MCR, 0x08);
    	HAL_WRITE_UINT8(base + UART_FCR, 0x01);
    	HAL_WRITE_UINT8(base + UART_IER, 0x05);
    }
    else
#endif
    {
		uart->base = SL2312_UART_BASE;
	
    	// Disable interrupts.
		// UART_DISABLE_INTERRUPT();
    
		uart_init_channel(uart);
	}
	
	uart_puts("\x1B[0m\n\n");
}


/*------------------------------------------------------------------------------
* 	uart_init_channel
-----------------------------------------------------------------------------+*/
static void uart_init_channel(UART_INFO_T *uart)
{
    UINT32 base = uart->base;
#ifdef GEMINI_ASIC
	REG32(SL2312_GPIO_BASE + 0x08) |= SL2312_UART_DIR;
	REG32(SL2312_GPIO_BASE + 0x0C) |= SL2312_UART_PIN;
#else
	REG32(SL2312_GPIO_BASE + 0x0C) |= SL2312_UART_PIN;
#endif	
//  set uart mode
    HAL_WRITE_UINT32(base+SERIAL_MDR,SERIAL_MDR_UART);
    // 8-1-no parity.
    HAL_WRITE_UINT32(base+SERIAL_LCR, SERIAL_LCR_DLAB);
    HAL_WRITE_UINT32(base+SERIAL_DLM, (uart->baud_clk >> 8) & 0xff);
    HAL_WRITE_UINT32(base+SERIAL_DLL, uart->baud_clk & 0xff);
    HAL_WRITE_UINT32(base+SERIAL_LCR, SERIAL_LCR_LEN8);
    // enable FIFO mode
    HAL_WRITE_UINT32(base+SERIAL_FCR, SERIAL_FCR_FIFO_ENABLE);
    // enable RX interrupts - otherwise ISR cannot be polled. Actual
    // interrupt control of serial happens via INT_MASK
    // HAL_WRITE_UINT32(base+SERIAL_IER,(SERIAL_IER_DR|SERIAL_IER_RLS));
}

/*------------------------------------------------------------------------------
* 	uart_putc
-----------------------------------------------------------------------------+*/
void uart_putc(unsigned char c)
{
	UINT32		cnt=0;
	UART_INFO_T	*uart;

	if (astel_serial_RS485)			// dhsul ASTEL
		return;

	uart = (UART_INFO_T *)&uart_info;

#ifdef LPC_IT8712
	if (uart->IT8712_existed)
	{
    	UINT8 tsr;
    	do {
        	HAL_READ_UINT8(uart->base + UART_LSR, tsr);
    	} while(!((tsr & UART_LSR_THRE)==UART_LSR_THRE)
    			&& (cnt++<0x8000000));
    
    	HAL_WRITE_UINT8(uart->base + UART_TX, (UINT8)(unsigned char)c);
    }
    else
#endif    
    {
    	UINT32 tsr;
    	do {
        	HAL_READ_UINT32(uart->base + SERIAL_LSR, tsr);
        	// Wait for TXI flag to be set - or for the register to be
        	// zero (works around a HW bug it seems).
    	} while (!((tsr & SERIAL_LSR_THRE)==SERIAL_LSR_THRE)
    			&& (cnt++<0x8000000));

    	HAL_WRITE_UINT32(uart->base + SERIAL_THR, (UINT32)(unsigned char)c);
    }
    
}

/*------------------------------------------------------------------------------
* 	uart_puts
-----------------------------------------------------------------------------+*/
void uart_puts(unsigned char *datap)
{
	// watchdog_disable();
	while (*datap)
	{
		if (*datap==0x0a)
			uart_putc(0x0d);
		uart_putc(*datap++);
	}
	// watchdog_enable();
}

/*--------------------------------------------------------------
* 	uart_polling
*---------------------------------------------------------------
* DESCRIPTION: To poll and get a character from UART
* INPUT      : None
* OUTPUT     : None
---------------------------------------------------------------*/
void uart_polling (void)
{
	UINT8 				ch;
	UART_STATISTICS_T 	*stat;
	UART_BUF_T 			*rxbuf;
	UINT32				base;
	UART_INFO_T			*uart;
	
	uart = (UART_INFO_T *)&uart_info;
	
	base = uart->base;
	stat = (UART_STATISTICS_T *)&uart->stat;
	rxbuf = (UART_BUF_T *)&uart->rxbuf;

#ifdef LPC_IT8712
	if (uart->IT8712_existed)
	{
    	UINT8 lsr, data;

    	HAL_READ_UINT8(base + UART_LSR, lsr);
    	if ((lsr & UART_LSR_DR) == UART_LSR_DR) 
    	{
	  		HAL_READ_UINT8(base + UART_RX, data);
			if (rxbuf->total < UART_RXBUF_SIZE)
			{
				rxbuf->buf[rxbuf->in_idx++] = data;
				if (rxbuf->in_idx >= UART_RXBUF_SIZE)
					rxbuf->in_idx = 0;
				rxbuf->total++;
			}
			else
			{
				stat->no_rxbuf_cnt ++;
			}
		}
   	}
	else
#endif	
	{
    	UINT32 lsr, data;

    	HAL_READ_UINT32(base+SERIAL_LSR, lsr);
    	if ((lsr & SERIAL_LSR_DR)==SERIAL_LSR_DR) 
    	{
	  		HAL_READ_UINT32(base+SERIAL_RBR, data);
			if (rxbuf->total < UART_RXBUF_SIZE)
			{
				rxbuf->buf[rxbuf->in_idx++] = data & 0xff;
				if (rxbuf->in_idx >= UART_RXBUF_SIZE)
					rxbuf->in_idx = 0;
				rxbuf->total++;
			}
			else
			{
				stat->no_rxbuf_cnt ++;
			}
		}
	}
}

/*--------------------------------------------------------------
* 	uart_scanc
---------------------------------------------------------------*/
int uart_scanc(unsigned char *c)
{
	UART_BUF_T *rxbuf;
	unsigned char data;
    
	rxbuf = (UART_BUF_T *)&uart_info.rxbuf;
	
	uart_polling();
	
	if (rxbuf->total)
	{
		UART_DISABLE_INTERRUPT();
		data =rxbuf->buf[rxbuf->out_idx++];
		if (rxbuf->out_idx >= UART_RXBUF_SIZE)
			rxbuf->out_idx = 0;
		rxbuf->total--;
		*c = data;
		UART_ENABLE_INTERRUPT();
		return  1;
	}
	else
	{
		return  0;
	}
}

/*--------------------------------------------------------------
* 	uart_getc
---------------------------------------------------------------*/
UINT8 uart_getc(void)
{
	UINT8 key;

	while (!uart_scanc(&key));

	return key;
}



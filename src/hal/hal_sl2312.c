/****************************************************************************
* Copyright  Storlink Corp 2005.  All rights reserved.                
*--------------------------------------------------------------------------
* Name			: hal_sl2312.c
* Description	: 
*		Handle CPU setting
*
* History
*
*	Date		Writer		Description
*	-----------	-----------	-------------------------------------------------
*	04/19/2005	Gary Chen	Create and implement from Jason's Redboot code
*
****************************************************************************/
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <define.h>
#include <board_config.h>
#include <sl2312.h>
#include "hal_intr.h"
//#include "../bsp/usb/OTGController.h"
#ifdef BOARD_SUPPORT_WEB
#include <net.h>
#endif

#define GPIO_PCI_CLOCK		BIT(20)
#ifndef MIDWAY 
UINT32 ahb_speed_tbl[]={125000000, 116666666, 108333333, 100000000, 91666666, 83333333, 75000000, 0};
#else
UINT32 ahb_speed_tbl[]={130000000, 140000000, 150000000, 160000000, 170000000, 180000000, 190000000, 200000000,0};
#endif
UINT64 sys_ticks;
void (*emac_isr_handler)(void);

struct irqaction {
	void (*handler)(unsigned int);
};
struct irqaction irq_handle[32];

int ide_present=0;

//extern void OTG_INT_ISR(void);

void hal_delay_us(UINT32 us);
static UINT32 ahb_clock, cpu_rate, sys_clk_period;

void hal_interrupt_configure(int vector, int level, int up);
void hal_interrupt_mask(int vector);
void hal_interrupt_unmask(int vector);

#ifdef BOARD_SUPPORT_WEB
int web_wait = 0;
TCP_SOCK_T tcp_socks[TCP_SOCKET_MAX_NUM];
int chk_timeout(UINT64 time);
extern TCP_SOCK_T tcp_socks[TCP_SOCKET_MAX_NUM];

int chk_timeout(UINT64 time)
	{	UINT64 now;	
		now=sys_get_ticks();
		/* * 	Bit of fudge time */	
		//if (time>OZDAY && now<OZSPLIT1) now+=OZDAY;
		/* *	This will screw up big time over midnight... */	
		if	(now > time ) return(1);	
		return 0;
	}

#endif
int web_on = 0;
/*----------------------------------------------------------------------
* hal_get_ahb_bus_speed
*----------------------------------------------------------------------*/
UINT32 hal_get_ahb_bus_speed(void)
{
	return ahb_clock;
}

/*----------------------------------------------------------------------
* hal_get_cpu_rate
*----------------------------------------------------------------------*/
UINT32 hal_get_cpu_rate(void)
{
	return cpu_rate;
}

/*----------------------------------------------------------------------
* hal_hardware_init
*----------------------------------------------------------------------*/
void hal_hardware_init(void)
{
	UINT32 cr, status;

	emac_isr_handler = NULL;
	
	hal_mmu_init();
	
	HAL_READ_UINT32(SL2312_GLOBAL_BASE + GLOBAL_STATUS, status);
	
#ifndef MIDWAY 	
	ahb_clock = ahb_speed_tbl[status & 0x07];
	switch ((status >> 4) & 0x03)
	{
		case 0:	// 1:1
			cpu_rate = ahb_clock;
			break;
		case 1:	// 3:2
			cpu_rate = ((ahb_clock * 3) / 2);
			break;
		case 2:	// 2:1
			cpu_rate = (ahb_clock * 2);
			break;
	}
#else
	ahb_clock = ahb_speed_tbl[((status>>15) & 0x07)];
	switch ((status >> 18) & 0x03)
	{
		case 0:	// 1:1
			cpu_rate = ahb_clock;
			break;
		case 1:	// 3:2
			cpu_rate = ((ahb_clock * 3) / 2);
			break;
		case 2:	// 2:1
			cpu_rate = ((ahb_clock * 24) / 13);
			break;
		case 3:	// 2:1
			cpu_rate = (ahb_clock * 2);
			break;
	}
#endif	
	// External device reset
	// REG32(SL2312_GLOBAL_BASE + GLOBAL_RESET) = 0x80000000;
	
#ifndef LPC_IT8712
	// Bit 3: Disable GPIO pins switch to LPC PAD
	// Bit 4: Disable to drive LPC clock IO pins
    REG32(SL2312_GLOBAL_BASE + GLOBAL_MISC_CTRL) &=  ~0x00010040;
#endif

	// enable Timer 1 for delay function
	// APB clock = AHB / 4
	// if period  = 10ms (100 ticks per second)
	// Timer 1 clounter = ahb_clock / 4 / 100
    HAL_WRITE_UINT32(SL2312_TIMER_CTRL_BASE, 0); // disable timer
    HAL_WRITE_UINT32(SL2312_TIMER1_BASE + SL2312_TIMER_COUNT, 0);
#ifdef MIDWAY
	sys_clk_period = ahb_clock / 6 / BOARD_TICKS_PER_SECOND;
#else    
    sys_clk_period = ahb_clock / 4 / BOARD_TICKS_PER_SECOND;
#endif    
    HAL_WRITE_UINT32((SL2312_TIMER1_BASE+SL2312_TIMER_COUNT), sys_clk_period ); // autoload register
    HAL_WRITE_UINT32((SL2312_TIMER1_BASE+SL2312_TIMER_LOAD), sys_clk_period ); // counter register
    cr = (SL2312_TIMER1_ENABLE | SL2312_TIMER1_OVERFLOW); 
    HAL_WRITE_UINT32(SL2312_TIMER_CTRL_BASE, cr);
	hal_interrupt_configure(SL2312_INTERRUPT_TIMER1, 0, 0);
	
	
	
	HAL_WRITE_UINT8((SL2312_LPC_HOST_BASE+LPC_BUS_CTRL),0xc0); 	
	HAL_WRITE_UINT8((SL2312_LPC_HOST_BASE+LPC_SERIAL_IRQ_CTRL),0xc0); 	
	hal_delay_us(100*1000);   // delay 100ms
	HAL_WRITE_UINT8((SL2312_LPC_HOST_BASE+LPC_SERIAL_IRQ_CTRL),0x80); 	

	HAL_ENABLE_INTERRUPTS();
	hal_interrupt_unmask(SL2312_INTERRUPT_TIMER1);
	
	HAL_ICACHE_INVALIDATE_ALL();
	HAL_DCACHE_INVALIDATE_ALL();
	HAL_ICACHE_ENABLE();
	// HAL_DCACHE_ENABLE();
}

/*----------------------------------------------------------------------
* hal_clock_initialize
*----------------------------------------------------------------------*/
void hal_clock_initialize(UINT32 period)
{

    // Unmask timer 0 interrupt
}

/*----------------------------------------------------------------------
* hal_clock_reset
*----------------------------------------------------------------------*/
void hal_clock_reset(UINT32 vector, UINT32 period)
{

    // Clear pending interrupt bit
}

/*----------------------------------------------------------------------
* hal_clock_read
*----------------------------------------------------------------------*/
void hal_clock_read(UINT32 *pvalue)
{
    UINT32 ctr;

    HAL_READ_UINT32((SL2312_TIMER1_BASE+SL2312_TIMER_COUNT), ctr);
    *pvalue = ctr;
}

/*----------------------------------------------------------------------
* hal_delay_us_hw
*----------------------------------------------------------------------*/
static void hal_delay_us_hw(UINT32 us)
{
	UINT32 total_cnt;
	UINT32 t1, t2;
	UINT32 DeltaTime=0;

	if (us == 0)
		return;
#ifdef MIDWAY
	total_cnt = ahb_clock/6000000;
	total_cnt *= us ;
#else		
	total_cnt = us * (ahb_clock/4000000);
#endif	
	t1 = REG32(SL2312_TIMER1_BASE+SL2312_TIMER_COUNT);
	do
	{
		t2 = REG32(SL2312_TIMER1_BASE+SL2312_TIMER_COUNT);
		// Note: The system clock is down count
		if ( t2 < t1)
			DeltaTime = t1 - t2;        
		else        
            DeltaTime = t1 + sys_clk_period - t2;
	} while ( DeltaTime < total_cnt );
}

/*----------------------------------------------------------------------
* hal_delay_us
*----------------------------------------------------------------------*/
void hal_delay_us(UINT32 us)
{
	unsigned int ms;
	
	if (us == 0)
		return;
		
	if (us <= 1000)
		hal_delay_us_hw(us);
	else
	{
		ms = us / 1000;
		us = us % 1000;
		hal_delay_us_hw(us);
		
		while(ms>0)
		{
			hal_delay_us_hw(1000);
			ms--;
		}
	}
}

/*----------------------------------------------------------------------
* hal_interrupt_mask
*----------------------------------------------------------------------*/
void hal_interrupt_mask(int vector)
{
	REG32(SL2312_INTERRUPT_BASE + SL2312_IRQ_MASK) &= ~(1<<vector);
}

/*----------------------------------------------------------------------
* hal_interrupt_mask_all
*----------------------------------------------------------------------*/
void hal_interrupt_mask_all(void)
{
	REG32(SL2312_INTERRUPT_BASE + SL2312_IRQ_MASK) = 0;
}

/*----------------------------------------------------------------------
* hal_interrupt_unmask
*----------------------------------------------------------------------*/
void hal_interrupt_unmask(int vector)
{
	REG32(SL2312_INTERRUPT_BASE + SL2312_IRQ_MASK) |= (1<<vector);
}

/*----------------------------------------------------------------------
* hal_interrupt_acknowledge
*----------------------------------------------------------------------*/
void hal_interrupt_acknowledge(int vector)
{

	REG32(SL2312_INTERRUPT_BASE + SL2312_IRQ_CLEAR) |= (1<<vector);
}

/*----------------------------------------------------------------------
*	hal_interrupt_configure
*----------------------------------------------------------------------*/
void hal_interrupt_configure(int vector, int level, int up)
{
    // if(vector <= CYGNUM_HAL_ISR_IRQ_MAX)
    // assume vector is less CYGNUM_HAL_ISR_IRQ_MAX
    if (level) // level trigger
    	REG32(SL2312_INTERRUPT_BASE + SL2312_IRQ_MODE) &= ~(1<<vector);
    else // edge trigger
    	REG32(SL2312_INTERRUPT_BASE + SL2312_IRQ_MODE) |= (1<<vector);
    if (up) // High Active or rising edge trigger
    	REG32(SL2312_INTERRUPT_BASE + SL2312_IRQ_LEVEL) &= ~(1<<vector);
    else // Low Active or falling edge trigger
    	REG32(SL2312_INTERRUPT_BASE + SL2312_IRQ_LEVEL) |= (1<<vector);
}


/*----------------------------------------------------------------------
*	hal_irq_handler
*----------------------------------------------------------------------*/
void hal_irq_handler(void)
{
	UINT32 status;
	UINT32 vbus;
	
	status = REG32(SL2312_INTERRUPT_BASE + SL2312_IRQ_STATUS);
	
	if (status & (1 << SL2312_INTERRUPT_TIMER1))
	{
		sys_ticks++;
		hal_interrupt_acknowledge(SL2312_INTERRUPT_TIMER1);
#ifdef SATA_LED
		if(ide_present && ((sys_ticks&0x04)==0x00)){
                        vbus = REG32(SL2312_GPIO_BASE + 0x04)&BIT(18);
                        REG32(SL2312_GPIO_BASE + (vbus==0? 0x14:0x10))=BIT(GPIO_SATA0_LED);
		}
#endif
	}
	
#ifdef BOARD_SUPPORT_WEB
		if(web_on == 0)
		{
				status = (REG32(SL2312_GPIO_BASE+0x04) & BIT(18))>>18;	
    				if(status==0)
    						web_wait++;
    				else
    						web_wait = 0;
		}
		else
		{
			int				i;
			TCP_SOCK_T		*sock;
				
			sock = (TCP_SOCK_T *)&tcp_socks[0];
			for (i=0; i<TCP_SOCKET_MAX_NUM; i++, sock++)
			{
				
				if((sock->used==1) && (chk_timeout(sock->timeout)) && ((sock->state!=ESTABLISHED)&&(sock->state!=CLOSE_WAIT)))
					sock->del_timeout=1;
	}
		}
#endif
	
//#ifndef MIDWAY
//	if (status & (1 << SL2312_INTERRUPT_EMAC))
//#else
//	if (status & (1 << SL2312_INTERRUPT_GMAC0))
//#endif
        if (status & (1 << SL2312_INTERRUPT_USB1)) 
//        printf("@@@emac_isr_handler %x\n",emac_isr_handler);
	{
//		if (emac_isr_handler)
//			(*emac_isr_handler)();
                //emac_isr_handler();  
                irq_handle[11].handler(status);
//                hal_interrupt_acknowledge(SL2312_INTERRUPT_USB1);
                
	}
	
	if(status&(1<<4)&&irq_handle[4].handler) //IDE use
		irq_handle[4].handler(status);
	else if(status & (1<<5)&&irq_handle[5].handler)
		irq_handle[5].handler(status);
	
	
}

/*----------------------------------------------------------------------
*	hal_register_irq_entry
*----------------------------------------------------------------------*/
void hal_register_irq_entry(void *handler,unsigned int irq_no)
{
	irq_handle[irq_no].handler = (void*)handler ;
	
	//emac_isr_handler = handler;
	
//	printf("###hal_register_irq_entry emac_isr_handler %x\n",emac_isr_handler);
}

/*----------------------------------------------------------------------
*	hal_get_ticks
*----------------------------------------------------------------------*/
UINT64 hal_get_ticks(void)
{
	return sys_ticks;
}

/*----------------------------------------------------------------------
* hal_reset_device
*----------------------------------------------------------------------*/
void hal_reset_device(void)
{
	// External device reset
	REG32(SL2312_GLOBAL_BASE + GLOBAL_RESET) = 0x80000000;
}

/*----------------------------------------------------------------------
* hal_reset
*----------------------------------------------------------------------*/
void hal_reset(void)
{
    
    HAL_ICACHE_DISABLE();
    HAL_ICACHE_INVALIDATE_ALL();

	// External device reset
#if defined(GEMINI_ASIC) || defined(LEPUS_ASIC)
	REG32(SL2312_GLOBAL_BASE + GLOBAL_RESET) = 0xC0000000;
#else	
	REG32(SL2312_GLOBAL_BASE + GLOBAL_RESET) = 0x80000000;
	REG32(SL2312_WAQTCHDOG_BASE) = 0;

	// Disable watchdog, no interrupt enabled, use PCLK
	REG32(SL2312_WAQTCHDOG_BASE + 0x0c) = 0;
	
	// Set WdLoad Register
	REG32(SL2312_WAQTCHDOG_BASE + 0x04) = 0x3ef1480;
	
	// Restart watchdog by writing 0x5ab9 to Wdrestart register
	REG32(SL2312_WAQTCHDOG_BASE + 0x08) = 0x5ab9;
	
	// Select clock source: PCLK
	// Enable watchdog and reset system 
	REG32(SL2312_WAQTCHDOG_BASE + 0x0c) = 3;
	
	
    
#endif	
		while(1);
	
}

/*----------------------------------------------------------------------
* hal_flash_enable
*----------------------------------------------------------------------*/
void hal_flash_enable(void)
{
	hal_delay_us(10000);

	unsigned int    value;
	
	value = REG32(SL2312_FLASH_CTRL_BASE + 0x0c) & 0x1f00;
	if((value&0x1000)==0x1000)
		REG32(SL2312_GLOBAL_BASE + GLOBAL_MISC_CTRL) &=  ~(GLOBAL_LCD_EN_BIT|0x00000004); //~0x00000001;
	else if	((value&0x800)==0x800)	
	{
		REG32(SL2312_GLOBAL_BASE + GLOBAL_MISC_CTRL) &=  ~(GLOBAL_LCD_EN_BIT|0x00000002); //~0x00000001;
		REG32(SL2312_FLASH_CTRL_BASE + FLASH_ACCESS_OFFSET) |= FLASH_DIRECT_ACCESS;
	}
	else 
		REG32(SL2312_GLOBAL_BASE + GLOBAL_MISC_CTRL) &=  ~(GLOBAL_LCD_EN_BIT|0x00000001); //~0x00000001;
					
#if defined(MIDWAY) && defined(BOARD_SUPPORT_IDE)
	REG32(SL2312_GLOBAL_BASE + GLOBAL_MISC_CTRL) &= ~BIT(4);		// Disable Parall Flash PAD
#endif	    
 
	hal_delay_us(1000);
}

/*----------------------------------------------------------------------
* hal_flash_disable
*----------------------------------------------------------------------*/
void hal_flash_disable(void)
{
	hal_delay_us(10000);						//disable all flash
    REG32(SL2312_GLOBAL_BASE + GLOBAL_MISC_CTRL) |=  0x07;//GLOBAL_PFLASH_EN_BIT; //0x00000001;
#if	defined(MIDWAY) && defined(BOARD_SUPPORT_IDE)
	REG32(SL2312_GLOBAL_BASE + GLOBAL_MISC_CTRL) |= BIT(4);		// Disable Parall Flash PAD
#endif    

#ifdef FLASH_TYPE_PARALLEL
    REG32(SL2312_FLASH_CTRL_BASE + FLASH_ACCESS_OFFSET) &= ~FLASH_DIRECT_ACCESS;
#endif
	hal_delay_us(1000);
}

/*----------------------------------------------------------------------
* hal_detect_pci_clock
*----------------------------------------------------------------------*/
int hal_detect_pci_clock(void)
{
	int data,i,j,int_no=0;
	
	data = REG32(SL2312_GPIO_BASE + 0x08);
	data &= ~GPIO_PCI_CLOCK ;					// pin dir
	REG32(SL2312_GPIO_BASE + 0x08) = data;
	
	data = REG32(SL2312_GPIO_BASE + 0x34);
	data &= ~GPIO_PCI_CLOCK ;					// edge trig
	REG32(SL2312_GPIO_BASE + 0x34) = data;
	
	data = REG32(SL2312_GPIO_BASE + 0x3c) ;
	data &= ~GPIO_PCI_CLOCK ;					// rising edge
	REG32(SL2312_GPIO_BASE + 0x3c) = data;
	
	data = REG32(SL2312_GPIO_BASE + 0x38) ;
	data |= GPIO_PCI_CLOCK ;						// both edge
	REG32(SL2312_GPIO_BASE + 0x38) = data;
	
	data = REG32(SL2312_GPIO_BASE + 0x28);
	data |= GPIO_PCI_CLOCK ;						// mask
	REG32(SL2312_GPIO_BASE + 0x28) = data;
	
	data= REG32(SL2312_GPIO_BASE + 0x30) ;
	data |= GPIO_PCI_CLOCK ;						// Clear INT
	REG32(SL2312_GPIO_BASE + 0x30) = data;
	
	data = REG32(SL2312_GPIO_BASE + 0x20) ;
	data |= GPIO_PCI_CLOCK ;						// Enable interrupt
	REG32(SL2312_GPIO_BASE + 0x20) = data;	
	
	for(j=0;j<10;j++){
		for (i=0;i<0x20;i++);
		data = REG32(SL2312_GPIO_BASE + 0x24) ;
		if (data&GPIO_PCI_CLOCK)						// Detect interrupt
		{
			int_no++;
		}
		
		data = REG32(SL2312_GPIO_BASE + 0x30) ;
		data |= GPIO_PCI_CLOCK ;						// Clear INT
		REG32(SL2312_GPIO_BASE + 0x30) = data;
	}
	
	return int_no;
}



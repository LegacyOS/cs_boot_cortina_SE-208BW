/****************************************************************************
* Copyright  Storlink Corp 2005.  All rights reserved.                
*--------------------------------------------------------------------------
* Name			: emac_sl2312.c
* Description	: 
*		Ethernet device driver for Storlink SL2312 Chip
*
* History
*
*	Date		Writer		Description
*	-----------	-----------	-------------------------------------------------
*	04/25/2005	Gary Chen	Create and implement from Jason's Redboot code
*
****************************************************************************/
#include <define.h>
#include <board_config.h>
#include <sl2312.h>

#ifdef MIDWAY
#include "gmac_sl2312.h"

#define diag_printf			printf
    

/*************************************************************
 *         Global Variable
 *************************************************************/
unsigned int FLAG_SWITCH = 0;	/* if 1-->switch chip presented. if 0-->switch chip unpresented */
int vlan_enabled;

GMAC_INFO_T gmac_private_data;
//static unsigned int     tx_free_desc = TX_DESC_NUM;
static unsigned int     flow_control_enable = 0;
//static unsigned int     tx_desc_virtual_base = 0;
//static unsigned int     rx_desc_virtual_base = 0;
//static unsigned int     tx_buf_virtual_base = 0;
//static unsigned int     rx_buf_virtual_base = 0;
unsigned int     		full_duplex = 1;
unsigned int			chip_id=0;
void gmac_disable_tx_rx(void);
void gmac_enable_tx_rx(void);

#ifdef BOARD_DCACHE_ON
	typedef struct {
		unsigned char	tx_desc_array[TX_DESC_NUM * sizeof(GMAC_DESCRIPTOR_T)] __attribute__((aligned(32)));
		unsigned char	rx_desc_array[RX_DESC_NUM * sizeof(GMAC_DESCRIPTOR_T)]  __attribute__((aligned(32)));
		unsigned char	tx_buf_array[TX_BUF_TOT_LEN] __attribute__((aligned(32)));
		unsigned char	rx_buf_array[RX_BUF_TOT_LEN] __attribute__((aligned(32)));
		unsigned long	last_word;
	} GMAC_NONCACHE_MEM_T;
	GMAC_NONCACHE_MEM_T 	*gmac_noncache_mem = (GMAC_NONCACHE_MEM_T *)BOARD_DRAM_NONCACHE_BASE;
	static unsigned char    *tx_desc_array;
	static unsigned char    *rx_desc_array;
	static unsigned char	*rx_buf_array;
	static unsigned char    *tx_buf_array;
#else
	static unsigned char    tx_desc_array[TX_DESC_NUM * sizeof(GMAC_DESCRIPTOR_T)] __attribute__((aligned(32))); 
	static unsigned char    rx_desc_array[RX_DESC_NUM * sizeof(GMAC_DESCRIPTOR_T)] __attribute__((aligned(32)));
	static unsigned char    tx_buf_array[TX_BUF_TOT_LEN] __attribute__((aligned(32)));
	static unsigned char    rx_buf_array[RX_BUF_TOT_LEN] __attribute__((aligned(32)));
#endif

/************************************************/
/*                 function declare             */
/************************************************/
void gmac_set_mac_addr(unsigned char *mac1, unsigned char *mac2);

extern void gmac_set_phy_status(void);
extern void gmac_get_phy_status(void);
extern char *sys_get_mac_addr(int index);
static void gmac_sl2312_start(void);
void sl2312_gmac_deliver(void);
void sl2312_gmac_diag_poll(void);
void gmac_sl2312_send(char *bufp, int total_len);
inline void gmac_sl2312_send_diag(GMAC_INFO_T *tp, char *bufp, int total_len);

#define sl2312_eth_enable_interrupt()	REG32(SL2312_INTERRUPT_BASE + SL2312_IRQ_MASK) &= ~(1<<SL2312_INTERRUPT_GMAC0)
#define sl2312_eth_disable_interrupt()	REG32(SL2312_INTERRUPT_BASE + SL2312_IRQ_MASK) |=  (1<<SL2312_INTERRUPT_GMAC0)
/****************************************/
/*	SPI Function Declare		*/
/****************************************/
extern void SPI_default(int vlan_enabled);
extern unsigned int SPI_get_identifier(void);

extern void enet_get_mac_addr(unsigned char* mac);

/****************************************/
/*	VLAN Function Declare		        */
/****************************************/

/************************************************/
/*                 function body                */
/************************************************/

#define gmac_read_reg(offset)					REG32(GMAC_BASE_ADDR + offset)
#define gmac_write_reg(offset, data, mask)		REG32(GMAC_BASE_ADDR + offset) = 	\
												(gmac_read_reg(offset) & (~mask)) | \
												(data & mask)

//static int __init gmac_init_module(void)
int gmac_sl2312_init(void)
{
	chip_id = REG32(SL2312_GLOBAL_BASE + GLOBAL_ID) & 0xff;

	FLAG_SWITCH = 0 ;
	FLAG_SWITCH = SPI_get_identifier();
	if(FLAG_SWITCH)
	{	
		diag_printf("\nConfig ADM699X...\n");
		SPI_default(vlan_enabled);	//Add by jason for ADM6996 config
	}
#ifdef GEMINI_ASIC
	/* set GMAC global register */
    REG32(SL2312_GLOBAL_BASE+0x10)=0x00520000;
    REG32(SL2312_GLOBAL_BASE+0x1c)=0x077707f0;
    REG32(SL2312_GLOBAL_BASE+0x20)=0x77770000;
#endif	
	gmac_sl2312_start();
	
    return TRUE;
}	

/*----------------------------------------------------------------------
* gmac_set_mac_addr
*	set mac address
*----------------------------------------------------------------------*/
void gmac_set_mac_addr(unsigned char *mac1, unsigned char *mac2)
{
	UINT32 data;
	data = mac1[0] + (mac1[1]<<8) + (mac1[2]<<16) + (mac1[3]<<24);
	gmac_write_reg(GMAC_STA_ADD0, data, 0xffffffff);
	data = mac1[4] + (mac1[5]<<8) + (mac2[0]<<16) + (mac2[1]<<24);
	gmac_write_reg(GMAC_STA_ADD1, data, 0xffffffff);
	data = mac2[2] + (mac2[3]<<8) + (mac2[4]<<16) + (mac2[5]<<24);
	gmac_write_reg(GMAC_STA_ADD2, data, 0xffffffff);
}
        			
/*----------------------------------------------------------------------
* gmac_init_chip
*----------------------------------------------------------------------*/
static int gmac_init_chip(void)
{
	GMAC_RBNR_T		rbnr_val,rbnr_mask;
	GMAC_CONFIG2_T	config2_val;
	GMAC_CONFIG0_T	config0,config0_mask;
	GMAC_CONFIG1_T	config1;
	UINT32 data;

	/* chip reset */
	 
    /* set RMII mode for testing */
    if (full_duplex == 1)
    {
        gmac_write_reg(GMAC_STATUS,0x0000001b,0x0000001f);  /* 100M full duplex */
    }
    else
    {    
        gmac_write_reg(GMAC_STATUS,0x00000013,0x0000001f);    /* 100M half duplex */
	}

    gmac_get_phy_status();
    
    gmac_set_mac_addr(sys_get_mac_addr(0), sys_get_mac_addr(1));
    
    /* set RX_FLTR register to receive all multicast packet */
    gmac_write_reg(GMAC_RX_FLTR,0x0000000f,0x0000001f);
//    gmac_write_reg(GMAC_RX_FLTR,0x00000001,0x0000001f);

	/* set per packet buffer size */
	config1.bits32 = 0;
    config1.bits.buf_size = 11; /* buffer size = 2048-byte */
    gmac_write_reg(GMAC_CONFIG1,config1.bits32,0x0000000f);
	
	/* set flow control threshold */
	config2_val.bits32 = 0;
	config2_val.bits.set_threshold = RX_DESC_NUM/4;
	config2_val.bits.rel_threshold = RX_DESC_NUM*3/4;
	gmac_write_reg(GMAC_CONFIG2,config2_val.bits32,0xffffffff);

	/* init remaining buffer number register */
	rbnr_val.bits32 = 0;
	rbnr_val.bits.buf_remain = RX_DESC_NUM;
	rbnr_mask.bits32 = 0;
	rbnr_mask.bits.buf_remain = 0xffff;	 
	gmac_write_reg(GMAC_RBNR,rbnr_val.bits32,rbnr_mask.bits32);

    /* disable TX/RX and disable internal loop back */
    config0.bits32 = 0;
    config0_mask.bits32 = 0;
    config0.bits.max_len = 2;
    config0.bits.tx_fc_en = 1; /* enable tx flow control */
    config0.bits.rx_fc_en = 1; /* enable rx flow control */
    config0.bits.dis_rx = 1;  /* disable rx */
    config0.bits.dis_tx = 1;  /* disable tx */
    config0.bits.loop_back = 0; /* enable/disable GMAC loopback */
	config0.bits.inv_rx_clk = 0;
	config0.bits.rising_latch = 1;
    config0_mask.bits.max_len = 3;
    config0_mask.bits.tx_fc_en = 1;
    config0_mask.bits.rx_fc_en = 1;
    config0_mask.bits.dis_rx = 1;
    config0_mask.bits.dis_tx = 1;
    config0_mask.bits.loop_back = 1;
    config0_mask.bits.inv_rx_clk = 1;
	config0_mask.bits.rising_latch = 1;
    gmac_write_reg(GMAC_CONFIG0,config0.bits32,config0_mask.bits32);
    
	flow_control_enable = 1; /* enable flow control flag */
	//flow_control_enable = 0; /* enable flow control flag */
	return (0);
}

/*----------------------------------------------------------------------
* gmac_enable_tx_rx
*----------------------------------------------------------------------*/
void gmac_enable_tx_rx(void)
{
	GMAC_CONFIG0_T	config0,config0_mask;

    /* enable TX/RX */
    config0.bits32 = 0;
    config0_mask.bits32 = 0;
    config0.bits.dis_rx = 0;  /* enable rx */
    config0.bits.dis_tx = 0;  /* enable tx */
    config0_mask.bits.dis_rx = 1;
    config0_mask.bits.dis_tx = 1;
    gmac_write_reg(GMAC_CONFIG0,config0.bits32,config0_mask.bits32);    
}

/*----------------------------------------------------------------------
* gmac_disable_tx_rx
*----------------------------------------------------------------------*/
void gmac_disable_tx_rx(void)
{
	GMAC_CONFIG0_T	config0,config0_mask;

    /* enable TX/RX */
    config0.bits32 = 0;
    config0_mask.bits32 = 0;
    config0.bits.dis_rx = 1;  /* disable rx */
    config0.bits.dis_tx = 1;  /* disable tx */
    config0_mask.bits.dis_rx = 1;
    config0_mask.bits.dis_tx = 1;
    gmac_write_reg(GMAC_CONFIG0,config0.bits32,config0_mask.bits32);    
}
    
/*----------------------------------------------------------------------
* gmac_hw_start
*----------------------------------------------------------------------*/
static void gmac_hw_start(void)
{
	GMAC_INFO_T     		*tp = (GMAC_INFO_T *)&gmac_private_data;
	GMAC_TXDMA_CURR_DESC_T	tx_desc;
	GMAC_RXDMA_CURR_DESC_T	rx_desc;
    GMAC_TXDMA_CTRL_T       txdma_ctrl,txdma_ctrl_mask;
    GMAC_RXDMA_CTRL_T       rxdma_ctrl,rxdma_ctrl_mask;
	GMAC_DMA_STATUS_T       dma_status,dma_status_mask;
	
	/* program TxDMA Current Descriptor Address register for first descriptor */
	tx_desc.bits32 = (unsigned int)(tp->tx_desc_dma);
	tx_desc.bits.eofie = 1;
//	tx_desc.bits.dec = 0;
	tx_desc.bits.sof_eof = 0x03;
	gmac_write_reg(GMAC_TXDMA_CURR_DESC,tx_desc.bits32,0xffffffff);
	gmac_write_reg(0xff2c,tx_desc.bits32,0xffffffff);   /* tx next descriptor address */
	
	/* program RxDMA Current Descriptor Address register for first descriptor */
	rx_desc.bits32 = (unsigned int)(tp->rx_desc_dma);
	rx_desc.bits.eofie = 1;
//	rx_desc.bits.dec = 0;
	rx_desc.bits.sof_eof = 0x03;
	gmac_write_reg(GMAC_RXDMA_CURR_DESC,rx_desc.bits32,0xffffffff);
	gmac_write_reg(0xff3c,rx_desc.bits32,0xffffffff);   /* rx next descriptor address */
	    	
	/* enable GMAC interrupt & disable loopback */
	dma_status.bits32 = 0;
	dma_status.bits.loop_back = 0;  /* disable DMA loop-back mode */
	dma_status.bits.m_cnt_full = 1;
	dma_status.bits.m_rx_pause_on = 1;
	dma_status.bits.m_tx_pause_on = 1;
	dma_status.bits.m_rx_pause_off = 1;
	dma_status.bits.m_tx_pause_off = 1;
	dma_status.bits.m_rx_overrun = 1;
	dma_status_mask.bits32 = 0;
	dma_status_mask.bits.loop_back = 1;
	dma_status_mask.bits.m_cnt_full = 1;
	dma_status_mask.bits.m_rx_pause_on = 1;
	dma_status_mask.bits.m_tx_pause_on = 1;
	dma_status_mask.bits.m_rx_pause_off = 1;
	dma_status_mask.bits.m_tx_pause_off = 1;
	dma_status_mask.bits.m_rx_overrun = 1;
	gmac_write_reg(GMAC_DMA_STATUS,dma_status.bits32,dma_status_mask.bits32);

    /* program tx dma control register */	
	txdma_ctrl.bits32 = 0;
	txdma_ctrl.bits.td_start = 0;    
	txdma_ctrl.bits.td_continue = 0; 
	txdma_ctrl.bits.td_chain_mode = 1; /* chain mode */
	txdma_ctrl.bits.td_prot = 0;
	txdma_ctrl.bits.td_burst_size = 2;
	txdma_ctrl.bits.td_bus = 0;
	txdma_ctrl.bits.td_endian = 0;
	txdma_ctrl.bits.td_finish_en = 1;
	txdma_ctrl.bits.td_fail_en = 1;
	txdma_ctrl.bits.td_perr_en = 1;
	txdma_ctrl.bits.td_eod_en = 0; /* disable Tx End of Descriptor Interrupt */
	txdma_ctrl.bits.td_eof_en = 1;
	txdma_ctrl_mask.bits32 = 0;
	txdma_ctrl_mask.bits.td_start = 1;    
	txdma_ctrl_mask.bits.td_continue = 1; 
	txdma_ctrl_mask.bits.td_chain_mode = 1;
	txdma_ctrl_mask.bits.td_prot = 15;
	txdma_ctrl_mask.bits.td_burst_size = 3;
	txdma_ctrl_mask.bits.td_bus = 1;
	txdma_ctrl_mask.bits.td_endian = 1;
	txdma_ctrl_mask.bits.td_finish_en = 1;
	txdma_ctrl_mask.bits.td_fail_en = 1;
	txdma_ctrl_mask.bits.td_perr_en = 1;
	txdma_ctrl_mask.bits.td_eod_en = 1;
	txdma_ctrl_mask.bits.td_eof_en = 1;
	gmac_write_reg(GMAC_TXDMA_CTRL,txdma_ctrl.bits32,txdma_ctrl_mask.bits32);
	
    /* program rx dma control register */	
	rxdma_ctrl.bits32 = 0;
	rxdma_ctrl.bits.rd_start = 1;    /* start RX DMA transfer */
	rxdma_ctrl.bits.rd_continue = 1; /* continue RX DMA operation */
	rxdma_ctrl.bits.rd_chain_mode = 1;   /* chain mode */
	rxdma_ctrl.bits.rd_prot = 0;
	rxdma_ctrl.bits.rd_burst_size = 2;
	rxdma_ctrl.bits.rd_bus = 0;
	rxdma_ctrl.bits.rd_endian = 0;
	rxdma_ctrl.bits.rd_finish_en = 1;
	rxdma_ctrl.bits.rd_fail_en = 1;
	rxdma_ctrl.bits.rd_perr_en = 1;
	rxdma_ctrl.bits.rd_eod_en = 0; /* disable Rx End of Descriptor Interrupt */
	rxdma_ctrl.bits.rd_eof_en = 1;
	rxdma_ctrl_mask.bits32 = 0;
	rxdma_ctrl_mask.bits.rd_start = 1;    
	rxdma_ctrl_mask.bits.rd_continue = 1; 
	rxdma_ctrl_mask.bits.rd_chain_mode = 1;
	rxdma_ctrl_mask.bits.rd_prot = 15;
	rxdma_ctrl_mask.bits.rd_burst_size = 3;
	rxdma_ctrl_mask.bits.rd_bus = 1;
	rxdma_ctrl_mask.bits.rd_endian = 1;
	rxdma_ctrl_mask.bits.rd_finish_en = 1;
	rxdma_ctrl_mask.bits.rd_fail_en = 1;
	rxdma_ctrl_mask.bits.rd_perr_en = 1;
	rxdma_ctrl_mask.bits.rd_eod_en = 1;
	rxdma_ctrl_mask.bits.rd_eof_en = 1;
	gmac_write_reg(GMAC_RXDMA_CTRL,rxdma_ctrl.bits32,rxdma_ctrl_mask.bits32);
    return;	
}	

/*----------------------------------------------------------------------
* gmac_hw_stop
*----------------------------------------------------------------------*/
static void gmac_hw_stop(void)
{
    GMAC_TXDMA_CTRL_T       txdma_ctrl,txdma_ctrl_mask;
    GMAC_RXDMA_CTRL_T       rxdma_ctrl,rxdma_ctrl_mask;

    /* program tx dma control register */	
	txdma_ctrl.bits32 = 0;
	txdma_ctrl.bits.td_start = 0;    
	txdma_ctrl.bits.td_continue = 0; 
	txdma_ctrl_mask.bits32 = 0;
	txdma_ctrl_mask.bits.td_start = 1;    
	txdma_ctrl_mask.bits.td_continue = 1; 	
	gmac_write_reg(GMAC_TXDMA_CTRL,txdma_ctrl.bits32,txdma_ctrl_mask.bits32);
    /* program rx dma control register */	
	rxdma_ctrl.bits32 = 0;
	rxdma_ctrl.bits.rd_start = 0;    /* stop RX DMA transfer */
	rxdma_ctrl.bits.rd_continue = 0; /* stop continue RX DMA operation */
	rxdma_ctrl_mask.bits32 = 0;
	rxdma_ctrl_mask.bits.rd_start = 1;    
	rxdma_ctrl_mask.bits.rd_continue = 1; 
	gmac_write_reg(GMAC_RXDMA_CTRL,rxdma_ctrl.bits32,rxdma_ctrl_mask.bits32);
}

/*----------------------------------------------------------------------
* gmac_init_desc_buf
*----------------------------------------------------------------------*/
static int gmac_init_desc_buf(void)
{
	GMAC_INFO_T  *tp = (GMAC_INFO_T *)&gmac_private_data;
	dma_addr_t   tx_first_desc_dma;
	dma_addr_t   rx_first_desc_dma;
	dma_addr_t   tx_first_buf_dma;
	dma_addr_t   rx_first_buf_dma;
	int    i;

#ifdef BOARD_DCACHE_ON
	tx_desc_array = (unsigned char *)&gmac_noncache_mem->tx_desc_array[0];
	rx_desc_array = (unsigned char *)&gmac_noncache_mem->rx_desc_array[0];
	tx_buf_array  = (unsigned char *)&gmac_noncache_mem->tx_buf_array[0];
	rx_buf_array  = (unsigned char *)&gmac_noncache_mem->rx_buf_array[0];
#endif

	/* allocates TX/RX DMA packet buffer */
	tp->tx_bufs = (unsigned char *)tx_buf_array;
	// memset(tp->tx_bufs,0x00,TX_BUF_TOT_LEN);
	tp->rx_bufs = (unsigned char *)rx_buf_array;
	// memset(tp->rx_bufs,0x00,RX_BUF_TOT_LEN);
    tp->tx_bufs_dma = (unsigned int)tp->tx_bufs;
    tp->rx_bufs_dma = (unsigned int)tp->rx_bufs;
//    diag_printf("tx_bufs = %08x\n",(unsigned int)tp->tx_bufs);
//    diag_printf("rx_bufs = %08x\n",(unsigned int)tp->rx_bufs);
	
	/* allocates TX/RX descriptors */
	tp->tx_desc = (GMAC_DESCRIPTOR_T *)tx_desc_array;
    memset(tp->tx_desc,0x00,TX_DESC_NUM*sizeof(GMAC_DESCRIPTOR_T));
	tp->rx_desc = (GMAC_DESCRIPTOR_T *)rx_desc_array;
    memset(tp->rx_desc,0x00,RX_DESC_NUM*sizeof(GMAC_DESCRIPTOR_T));
   // diag_printf("tx_desc = %08x\n",(unsigned int)tp->tx_desc);
   // diag_printf("rx_desc = %08x\n",(unsigned int)tp->rx_desc);
	
	/* TX descriptors initial */
	tp->tx_cur_desc = tp->tx_desc;  /* virtual address */
	tp->tx_finished_desc = tp->tx_desc; /* virtual address */
	tp->tx_desc_dma = (unsigned int)tp->tx_desc;
	tx_first_desc_dma = tp->tx_desc_dma; /* physical address */
    tx_first_buf_dma = tp->tx_bufs_dma;
	for (i = 1; i < TX_DESC_NUM; i++)
	{
		tp->tx_desc->frame_ctrl.bits_tx_out.own = CPU; /* set owner to CPU */
		tp->tx_desc->frame_ctrl.bits_tx_out.buffer_size = TX_BUF_SIZE;  /* set tx buffer size for descriptor */
		tp->tx_desc->buf_adr = tp->tx_bufs_dma; /* set data buffer address */
		tp->tx_bufs_dma = tp->tx_bufs_dma + TX_BUF_SIZE; /* point to next buffer address */
		tp->tx_desc_dma = tp->tx_desc_dma + sizeof(GMAC_DESCRIPTOR_T); /* next tx descriptor DMA address */
		tp->tx_desc->next_desc.next_descriptor = tp->tx_desc_dma | 0x0000000b;
		tp->tx_desc = &tp->tx_desc[1] ; /* next tx descriptor virtual address */
	}
	/* the last descriptor will point back to first descriptor */
	tp->tx_desc->frame_ctrl.bits_tx_out.own = CPU;
	tp->tx_desc->frame_ctrl.bits_tx_out.buffer_size = TX_BUF_SIZE;
	tp->tx_desc->buf_adr = (unsigned int)tp->tx_bufs_dma;
	tp->tx_desc->next_desc.next_descriptor = tx_first_desc_dma | 0x0000000b;
	tp->tx_desc = tp->tx_cur_desc;
	tp->tx_desc_dma = tx_first_desc_dma;
	tp->tx_bufs_dma = tx_first_buf_dma;
	
	/* RX descriptors initial */
	tp->rx_cur_desc = tp->rx_desc;  /* virtual address */
	tp->rx_desc_dma = (unsigned int)tp->rx_desc;
	rx_first_desc_dma = tp->rx_desc_dma; /* physical address */
	rx_first_buf_dma = tp->rx_bufs_dma;
	for (i = 1; i < RX_DESC_NUM; i++)
	{
		tp->rx_desc->frame_ctrl.bits_rx.own = DMA;  /* set owner bit to DMA */
		tp->rx_desc->frame_ctrl.bits_rx.buffer_size = RX_BUF_SIZE; /* set rx buffer size for descriptor */
		tp->rx_desc->buf_adr = tp->rx_bufs_dma;   /* set data buffer address */
		tp->rx_bufs_dma = tp->rx_bufs_dma + RX_BUF_SIZE;    /* point to next buffer address */
		tp->rx_desc_dma = tp->rx_desc_dma + sizeof(GMAC_DESCRIPTOR_T); /* next rx descriptor DMA address */
		tp->rx_desc->next_desc.next_descriptor = tp->rx_desc_dma | 0x0000000b;
		tp->rx_desc = &tp->rx_desc[1]; /* next rx descriptor virtual address */
	}
	/* the last descriptor will point back to first descriptor */
	tp->rx_desc->frame_ctrl.bits_rx.own = DMA;
	tp->rx_desc->frame_ctrl.bits_rx.buffer_size = RX_BUF_SIZE;
	tp->rx_desc->buf_adr = tp->rx_bufs_dma;
	tp->rx_desc->next_desc.next_descriptor = rx_first_desc_dma | 0x0000000b;
	tp->rx_desc = tp->rx_cur_desc;
	tp->rx_desc_dma = rx_first_desc_dma;
	tp->rx_bufs_dma = rx_first_buf_dma;
	
	return (0);    
}    

/*----------------------------------------------------------------------
* gmac_clear_counter
*----------------------------------------------------------------------*/
static int gmac_clear_counter (void)
{
//	GMAC_INFO_T     *tp = (GMAC_INFO_T *)&gmac_private_data;

    /* clear counter */
    gmac_read_reg(GMAC_IN_DISCARDS);
    gmac_read_reg(GMAC_IN_ERRORS); 
//    tp->stats.tx_bytes = 0;
//    tp->stats.tx_packets = 0;
//	tp->stats.tx_errors = 0;
//    tp->stats.rx_bytes = 0;
//	tp->stats.rx_packets = 0;
//	tp->stats.rx_errors = 0;
//    tp->stats.rx_dropped = 0;    
	return (0);    
}
   			
/*----------------------------------------------------------------------
* gmac_sl2312_start
*----------------------------------------------------------------------*/
static void gmac_sl2312_start(void)
{

    /* allocates tx/rx descriptor and buffer memory */
    gmac_init_desc_buf();

    /* set PHY register to start autonegition process */
    gmac_set_phy_status();

	/* GMAC initialization */
	if ( gmac_init_chip() ) 
	{
		diag_printf ("GMAC init fail\n");
	}	

    /* enable tx/rx register */    
    gmac_enable_tx_rx();
    
    /* start DMA process */
	gmac_hw_start();

    /* clear statistic counter */
    gmac_clear_counter();
	
	return ;	
}


/*----------------------------------------------------------------------
* gmac_sl2312_stop
*----------------------------------------------------------------------*/
static void gmac_sl2312_stop(void)
{
    
    /* stop tx/rx packet */
    gmac_disable_tx_rx();

    /* stop the chip's Tx and Rx DMA processes */
	gmac_hw_stop();          
}

/*----------------------------------------------------------------------
* gmac_weird_interrupt
*----------------------------------------------------------------------*/
static void gmac_weird_interrupt(void)
{
}

/*----------------------------------------------------------------------
* gmac_sl2312_isr
*----------------------------------------------------------------------*/
void gmac_sl2312_isr(void)
{
	GMAC_RXDMA_FIRST_DESC_T	rxdma_busy;
	GMAC_TXDMA_FIRST_DESC_T	txdma_busy;
    GMAC_TXDMA_CTRL_T       txdma_ctrl,txdma_ctrl_mask;
    GMAC_RXDMA_CTRL_T       rxdma_ctrl,rxdma_ctrl_mask;
	GMAC_DMA_STATUS_T	    status;
		
	sl2312_eth_disable_interrupt();
		
    // for (;;)
    {
        /* read DMA status */
	    status.bits32 = gmac_read_reg(GMAC_DMA_STATUS);

	    /* clear DMA status */
        gmac_write_reg(GMAC_DMA_STATUS,status.bits32,status.bits32);	
        
        /* receive rx interrupt */
    	//if ( ((status.bits.rs_eofi==1)||(status.bits.rs_finish==1))||
    	//     ((status.bits.ts_eofi==1)||(status.bits.ts_finish==1)) )
    	if ((status.bits.rs_eofi==1) || (status.bits.rs_finish==1))
    	if (status.bits32 & 0x220)
    	{
			sl2312_gmac_deliver();
        }
        
        if ((status.bits32 & 0xffffc000)==0)
        {
            // break;
			sl2312_eth_enable_interrupt();
            return;
        }  
          
	    if (status.bits.rx_overrun == 1)
	    {
            /* if RX DMA process is stoped , restart it */    
	        rxdma_busy.bits32 = gmac_read_reg(GMAC_RXDMA_FIRST_DESC) ;
	        if (rxdma_busy.bits.rd_busy == 0)
	        {
	            /* restart Rx DMA process */
    	        rxdma_ctrl.bits32 = 0;
    	        rxdma_ctrl.bits.rd_start = 1;    /* start RX DMA transfer */
	            rxdma_ctrl.bits.rd_continue = 1; /* continue RX DMA operation */
	            rxdma_ctrl_mask.bits32 = 0;
    	        rxdma_ctrl_mask.bits.rd_start = 1;   
	            rxdma_ctrl_mask.bits.rd_continue = 1; 
	            gmac_write_reg(GMAC_RXDMA_CTRL,rxdma_ctrl.bits32,rxdma_ctrl_mask.bits32);
            }
	    }
	    
        /* receive tx interrupt */
//    	if ( ((status.bits.ts_eofi==1)||(status.bits.ts_finish==1)) )
//    	{
//    		gmac_tx_interrupt(dev);
//    	}
    	
    	/* check uncommon events */	
#if 0    	
        if ((status.bits32 & 0x633fc000)!=0)
        {
            gmac_weird_interrupt();
        }
#endif
	}
	
	sl2312_eth_enable_interrupt();
	return;
}

/*----------------------------------------------------------------------
* sl2312_gmac_deliver
*----------------------------------------------------------------------*/
void sl2312_gmac_deliver(void)
{
	GMAC_INFO_T 			*tp = (GMAC_INFO_T *)&gmac_private_data;
    GMAC_DESCRIPTOR_T   	*rx_desc;
	unsigned int 			pkt_len;
	unsigned int        	own;
	unsigned int        	desc_count;
	GMAC_RXDMA_FIRST_DESC_T	rxdma_busy;
	static UINT32 gmac_poll_phy_ticks=0;
	int	i;

	while (1)
	{
    	own = tp->rx_cur_desc->frame_ctrl.bits32 >> 31;
    	if (own == CPU) /* check owner bit */
    	{
    	    rx_desc = tp->rx_cur_desc;
    	    /* check error interrupt */
#if 0    	    
    	    if ( (rx_desc->frame_ctrl.bits_rx.derr==1)||(rx_desc->frame_ctrl.bits_rx.perr==1) )
    	    {
		        diag_printf("gmac_rx_interrupt::Rx Descriptor Processing Error !!!\n");
		    }
#endif		    
		    /* get frame information from the first descriptor of the frame */
    	    pkt_len = rx_desc->flag_status.bits_rx_status.frame_count-4;  /*total byte count in a frame*/
			desc_count = rx_desc->frame_ctrl.bits_rx.desc_count; /* get descriptor count per frame */
    	
			if ((pkt_len > 0) && (rx_desc->frame_ctrl.bits_rx.frame_state == 0x000)) /* good frame */
			{
    	        // (sc->funs->eth_drv->recv)(sc, pkt_len);
    	        // queue it
    	        	enet_input(rx_desc->buf_adr, pkt_len);
			}
		    
		    rx_desc->frame_ctrl.bits_rx.own = DMA; /* release rx descriptor to DMA */
    	    /* point to next rx descriptor */             
    	    tp->rx_cur_desc = (GMAC_DESCRIPTOR_T *)(tp->rx_cur_desc->next_desc.next_descriptor & 0xfffffff0);
 			
    	    /* release buffer to Remaining Buffer Number Register */
    	    if (flow_control_enable ==1)
    	    {
    	        gmac_write_reg(GMAC_BNCR, desc_count, 0x0000ffff);
    	    }
		}
		else
		{
			if ((sys_get_ticks() - gmac_poll_phy_ticks) >= (BOARD_TPS * 3))
			{
				gmac_get_phy_status();
				gmac_poll_phy_ticks = sys_get_ticks();
			}
			break;
		}
	}
	
    /* if RX DMA process is stoped , restart it */    
	rxdma_busy.bits.rd_first_des_ptr = gmac_read_reg(GMAC_RXDMA_FIRST_DESC);
	if (rxdma_busy.bits.rd_busy == 0)
	{
    	GMAC_RXDMA_CTRL_T rxdma_ctrl,rxdma_ctrl_mask;
	    rxdma_ctrl.bits32 = 0;
    	rxdma_ctrl.bits.rd_start = 1;    /* start RX DMA transfer */
	    rxdma_ctrl.bits.rd_continue = 1; /* continue RX DMA operation */
	    rxdma_ctrl_mask.bits32 = 0;
    	rxdma_ctrl_mask.bits.rd_start = 1;   
	    rxdma_ctrl_mask.bits.rd_continue = 1; 
	    gmac_write_reg(GMAC_RXDMA_CTRL,rxdma_ctrl.bits32,rxdma_ctrl_mask.bits32);
    }

}

/*----------------------------------------------------------------------
* gmac_sl2312_send
*----------------------------------------------------------------------*/
void gmac_sl2312_send(char *bufp, int total_len)
{
	GMAC_INFO_T     		*tp = (GMAC_INFO_T *)&gmac_private_data;
	GMAC_TXDMA_CTRL_T		tx_ctrl,tx_ctrl_mask;
	GMAC_TXDMA_FIRST_DESC_T	txdma_busy;

	// Share Pin issue
#ifndef MIDWAY
	REG32(SL2312_GLOBAL_BASE+GLOBAL_MISC_CTRL) |= GLOBAL_FLASH_EN_BIT; //0x00000001 ;
#endif
    
    if ((tp->tx_cur_desc->frame_ctrl.bits_tx_out.own == CPU) && (total_len < TX_BUF_SIZE))
	{
        if (FLAG_SWITCH==1 && (chip_id<0xA3) && vlan_enabled)
        {
   		   	tp->tx_cur_desc->frame_ctrl.bits_tx_out.vlan_enable = 1;      /* enable vlan TIC insertion */
		   	tp->tx_cur_desc->flag_status.bits_tx_flag.vlan_id = 1;		/* 	1:To LAN . 2:To WAN */		
	    }
	    else
	    {
   		   	tp->tx_cur_desc->frame_ctrl.bits_tx_out.vlan_enable = 0;      /* disable vlan TIC insertion */
        }

		{
			int bd_id;
			
			bd_id = ((unsigned long)tp->tx_cur_desc - (unsigned long)tx_desc_array) / sizeof(GMAC_DESCRIPTOR_T);
			tp->tx_cur_desc->buf_adr = (unsigned int)tx_buf_array + (TX_BUF_SIZE * bd_id);
			memcpy((unsigned char *)(tp->tx_cur_desc->buf_adr), bufp, total_len);
		}
		
		tp->tx_cur_desc->frame_ctrl.bits_tx_out.buffer_size = total_len;  /* descriptor byte count */
    	tp->tx_cur_desc->flag_status.bits_tx_flag.frame_count = total_len;    /* total frame byte count */
    	tp->tx_cur_desc->next_desc.bits.sof_eof = 0x03;                 /*only one descriptor*/
    	tp->tx_cur_desc->frame_ctrl.bits_tx_out.own = DMA;	                /* set owner bit */
    	tp->tx_cur_desc = (GMAC_DESCRIPTOR_T *)(tp->tx_cur_desc->next_desc.next_descriptor & 0xfffffff0);
	} 

 	/* if TX DMA process is stoped , restart it */    
	txdma_busy.bits32 = gmac_read_reg(GMAC_TXDMA_FIRST_DESC);
	if (txdma_busy.bits.td_busy == 0)
	{
		/* restart DMA process */
		tx_ctrl.bits32 = 0;
		tx_ctrl.bits.td_start = 1;
		tx_ctrl.bits.td_continue = 1;
		tx_ctrl_mask.bits32 = 0;
		tx_ctrl_mask.bits.td_start = 1;
		tx_ctrl_mask.bits.td_continue = 1;
		gmac_write_reg(GMAC_TXDMA_CTRL,tx_ctrl.bits32,tx_ctrl_mask.bits32);
	}	
	
}

/*----------------------------------------------------------------------
* gmac_sl2312_can_send
*----------------------------------------------------------------------*/
int gmac_sl2312_can_send(void)
{
	GMAC_INFO_T	*tp = (GMAC_INFO_T *)&gmac_private_data;

    if (tp->tx_cur_desc->frame_ctrl.bits_tx_out.own == CPU)
    {
        return (1);
    }
    else
    {
        return (0);
    }        
}

/*----------------------------------------------------------------------
* emac_reset_statistics
*----------------------------------------------------------------------*/
void gmac_reset_statistics(void)
{
#ifdef GMAC_STATISTICS
	GMAC_INFO_T *tp = (GMAC_INFO_T *)&gmac_private_data;
	
	tp->interrupt_cnt = 0;
	tp->rx_intr_cnt = 0;
	tp->tx_intr_cnt = 0;
	tp->rx_pkts = 0;
	tp->tx_pkts = 0;
	tp->rx_overrun = 0;
	tp->tx_underrun = 0;
	tp->tx_no_resource = 0;
#endif
}

/*----------------------------------------------------------------------
* emac_show_statistics
*----------------------------------------------------------------------*/
void gmac_show_statistics(char argc, char *argv[])
{
#ifdef GMAC_STATISTICS        	    
	GMAC_INFO_T *tp = (GMAC_INFO_T *)&gmac_private_data;
	EMAC_DESCRIPTOR_T *bd;
	int i, type, total;

	printf("EMAC Statistics:\n");
	printf("Interrupts    : %u\n", tp->interrupt_cnt);
	printf("Rx Intr       : %u\n", tp->rx_intr_cnt);
	printf("Tx Intr       : %u\n", tp->tx_intr_cnt);
	printf("Rx packets    : %u\n", tp->rx_pkts);
	printf("Tx packets    : %u\n", tp->tx_pkts);
	printf("Rx Overrun    : %u\n", tp->rx_overrun);
	printf("Tx Underrun   : %u\n", tp->tx_underrun);
	printf("Tx no resource: %u\n", tp->tx_no_resource);
	printf("rx_head       : %d\n", tp->rx_head);
	printf("rx_tail       : %d\n", tp->rx_tail);
	printf("tx_head       : %d\n", tp->tx_head);
	printf("tx_tail       : %d\n", tp->tx_tail);
	
	type = 0;
	total = RX_DESC_NUM;
	if (argc == 2)
	{
		if (strncasecmp(argv[1], "rx", 2)==0)
		{
			type = 0;
			total = RX_DESC_NUM;
		}
		else if (strncasecmp(argv[1], "tx", 2)==0)
		{
			type = 1;
			total = TX_DESC_NUM;
		}
	}
	
	switch (type)
	{
		case 0:
			bd = (EMAC_DESCRIPTOR_T *)&tp->rx_desc[0];
			printf("Receive Buffer Descriptors:\n");
			break;
		case 1:
			bd = (EMAC_DESCRIPTOR_T *)&tp->tx_desc[0];
			printf("Transmit Buffer Descriptors:\n");
			break;
	}
			
	
	printf("id   Frame Control   Status/Flag   Buffer Addr   Next Descriptor\n");
	for (i=0; i<total; i++)
	{
		printf("%-2d   0x%08X      0x%08X    0x%08X    0x%08X\n",
				i,
				bd->frame_ctrl.bits32,
				bd->flag_status.bits32,
				bd->buf_adr,
				bd->next_desc.next_descriptor);
				bd++;
	} 
#endif
}

#endif // MIDWAY

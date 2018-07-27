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

#if defined(LEPUS_FPGA) || defined(LEPUS_ASIC)
#include "sl_lepus_gmac.h"

#define SKB_RESERVE_BYTES	16	// to reserve SKB header pointer
#define RX_INSERT_BYTES		RX_INSERT_2_BYTE

#define GMAC_TXQ_NUM		6
#define hal_cache_consistent_sync
#define GMAC_EXISTED_FLAG	0x5566abcd

/*************************************************************
 *         Global Variable
 *************************************************************/
static int	gmac_initialized = 0;
static TOE_INFO_T toe_private_data;
//unsigned int FLAG_SWITCH;
extern char broadcast_mac[6];
int mac_dump_rxpkt;
int mac_dump_txpkt;
int mac_status_error;
int txbuf_ptr[TOE_SW_TXQ_NUM*TOE_GMAC1_SWTXQ_DESC_NUM];
static UINT32 gmac_poll_phy_ticks;

#define toe_gmac_enable_interrupt(irq)	REG32(SL2312_INTERRUPT_BASE + SL2312_IRQ_MASK) &= ~(1<<SL2312_INTERRUPT_GMAC0)
#define toe_gmac_disable_interrupt(irq)	REG32(SL2312_INTERRUPT_BASE + SL2312_IRQ_MASK) |=  (1<<SL2312_INTERRUPT_GMAC0)
void toe_gmac_enable_tx_rx(void);
void toe_gmac_disable_tx_rx(void);

void gmac_set_mac_address(GMAC_INFO_T *tp, unsigned char *mac1, unsigned char *mac2);
void toe_gmac_send(char *bufp, int total_len);

unsigned int gmac_num;

/*----------------------------------------------------------------------
*	gmac_read_reg
*----------------------------------------------------------------------*/
// static inline unsigned int gmac_read_reg(unsigned int base, unsigned int offset)
unsigned int gmac_read_reg(unsigned int base, unsigned int offset)
{
    volatile unsigned int reg_val;

    reg_val = readl(base + offset);
	return (reg_val);
}

/*----------------------------------------------------------------------
*	gmac_write_reg
*----------------------------------------------------------------------*/
// static inline void gmac_write_reg(unsigned int base, unsigned int offset,unsigned int data,unsigned int bit_mask)
void gmac_write_reg(unsigned int base, unsigned int offset,unsigned int data,unsigned int bit_mask)
{
	volatile unsigned int reg_val;
    unsigned int *addr;

	reg_val = ( gmac_read_reg(base, offset) & (~bit_mask) ) | (data & bit_mask);
	addr = (unsigned int *)(base + offset);
    writel(reg_val,addr);
	return;
}

/*----------------------------------------------------------------------
* dm_byte
*----------------------------------------------------------------------*/
void dm_byte(UINT32 location, int length)
{
	UINT8		*start_p, *end_p, *curr_p;
	int			in_flash_range = 0;
	char		*datap, *cp, data, *bufp;
	int			i;

	//if (length > 1024)
	//	length = 1024;
		
	start_p = (UINT8 *)location;
	end_p = start_p + length;
	
	bufp = datap = (char *)malloc(length+128);
	if (datap == NULL)
	{
		dbg_printf(("No free memory!\n"));
		return;
	}
	
	if (location >= (UINT32)SL2312_FLASH_SHADOW && location <= (UINT32)((UINT32)SL2312_FLASH_SHADOW + 0x10000000))
		in_flash_range = 1; 
		
	// read data
	if (in_flash_range) hal_flash_enable();
	curr_p=(UINT8 *)(location & 0xfffffff0);
	cp = datap;
	for (; curr_p<end_p;)
		*cp++ = *curr_p++;
	if (in_flash_range) hal_flash_disable();
	
	curr_p=(UINT8 *)(location & 0xfffffff0);
	while (curr_p < end_p)
	{
		UINT8 *p1, *p2;
        printf("0x%08x: ",(UINT32)curr_p & 0xfffffff0);
        p1 = curr_p;
        p2 = datap;
		// dump data		        
		for (i=0; i<16; i++)
        {
			if (curr_p < start_p || curr_p >= end_p)
				printf("   ");
			else
			{
				data = *datap;
				printf("%02X ", data);
			}
			if (i==7)
				printf("- ");
			curr_p++;
			datap++;
        }

		// dump ascii	        
		curr_p = p1;
		datap = p2;
		for (i=0; i<16; i++)
		{
			if (curr_p < start_p || curr_p >= end_p)
				printf(".");
			else
			{
				data = *datap ;
				if (data<0x20 || data>0x7f || data==0x25) 
					printf(".");
				else
					printf("%c", data);;
			}
			curr_p++;
			datap++;
		}
		printf("\n");
	} 
	
	free(bufp);
}

/*----------------------------------------------------------------------
*	toe_gmac_disable_tx_rx
*----------------------------------------------------------------------*/
void toe_gmac_disable_tx_rx(void)
{
	TOE_INFO_T		*toe;
	GMAC_INFO_T 	*tp;
	GMAC_CONFIG0_T	config0,config0_mask;
	
	toe = (TOE_INFO_T *)&toe_private_data;
	tp = (GMAC_INFO_T *)&toe->gmac;
	
	//GMAC_INFO_T		*tp = sc->driver_private;

    /* enable TX/RX */
    config0.bits32 = 0;
    config0_mask.bits32 = 0;
    config0.bits.dis_rx = 1;  /* disable rx */
    config0.bits.dis_tx = 1;  /* disable tx */
    config0_mask.bits.dis_rx = 1;
    config0_mask.bits.dis_tx = 1;
    gmac_write_reg(tp->base_addr, GMAC_CONFIG0, config0.bits32,config0_mask.bits32);
    
    
    //printf("toe_gmac_disable_tx_rx tp->base_addr %x  gmac_read_reg(tp->base_addr, GMAC_CONFIG0) : %x\n",tp->base_addr,gmac_read_reg(tp->base_addr, GMAC_CONFIG0));
    //printf("toe->gmac.port_id : %x\n",toe->gmac.port_id);
}
    

/*----------------------------------------------------------------------
* toe_gmac_sw_reset
*----------------------------------------------------------------------*/
static void toe_gmac_sw_reset(void)
{
    //REG32(GMAC_GLOBAL_BASE_ADDR + GLOBAL_RESET) |= 0x00000060;
    unsigned int	reg_val;
	reg_val = REG32(GMAC_GLOBAL_BASE_ADDR+GLOBAL_RESET) | 0x00000060;   /* GMAC0 S/W reset */
    REG32(GMAC_GLOBAL_BASE_ADDR+GLOBAL_RESET) = reg_val;
    hal_delay_us(100);
    return;
    
    return;
}

/*----------------------------------------------------------------------
*	toe_gmac_init_chip
*----------------------------------------------------------------------*/
static int toe_gmac_init_chip(GMAC_INFO_T *tp)
{
	
	GMAC_CONFIG0_T	config0,config0_mask;
	GMAC_CONFIG1_T	config1;
	GMAC_CONFIG2_T	config2_val;
	GMAC_CONFIG3_T	config3_val;
	unsigned int    status;
	GMAC_TX_WCR0_T	hw_weigh;
	GMAC_TX_WCR1_T	sw_weigh;
	 
	/* set PHY operation mode */
	status = tp->phy_mode<<5 | 0x11;
	if (tp->auto_nego_cfg)
    	status |= (tp->full_duplex_status<<3) | (tp->speed_status<<1);
	else
    	status |= (tp->full_duplex_cfg<<3) | (tp->speed_cfg<<1);
    if(gmac_num)
    	status = 0x7d;

	/* BIOS에서는 25M고정
	 */
#if __ORIGINAL__
    gmac_write_reg(tp->base_addr, GMAC_STATUS,status, 0x0000007f);
#else
   	status = 0x5b;
    gmac_write_reg(tp->base_addr, GMAC_STATUS,status, 0x0000007f);
#endif
    gmac_set_mac_address(tp, tp->mac_addr1, tp->mac_addr2);

    /* set RX_FLTR register to receive all multicast packet */
    // gmac_write_reg(tp->base_addr, GMAC_RX_FLTR, 0x0000001F,0x0000001f);
    //gmac_write_reg(tp->base_addr, GMAC_RX_FLTR, 0x000001f,0x0000001f);
    gmac_write_reg(tp->base_addr, GMAC_RX_FLTR,0x000000005,0x00000005);

	/* set flow control threshold */
	config1.bits32 = 0;
	config1.bits.set_threshold = 8;
	config1.bits.rel_threshold = 32;
    gmac_write_reg(tp->base_addr, GMAC_CONFIG1, config1.bits32,0xffffffff);

	/* set SW free queue flow control threshold */
	config2_val.bits32 = 0;
	config2_val.bits.set_threshold = TOE_SW_FREEQ_DESC_NUM/4;
	config2_val.bits.rel_threshold = TOE_SW_FREEQ_DESC_NUM*3/4;
	gmac_write_reg(tp->base_addr, GMAC_CONFIG2, config2_val.bits32,0xffffffff);

	/* set HW free queue flow control threshold */
	config3_val.bits32 = 0;
	config3_val.bits.set_threshold = TOE_HW_FREEQ_DESC_NUM/4;
	config3_val.bits.rel_threshold = TOE_HW_FREEQ_DESC_NUM*3/4;
	gmac_write_reg(tp->base_addr, GMAC_CONFIG3, config3_val.bits32,0xffffffff);

    /* disable TX/RX and disable internal loop back */
    config0.bits32 = 0;
    config0_mask.bits32 = 0;
    config0.bits.max_len = 2;

//
    if (tp->flow_control_enable==1)
    {
        config0.bits.tx_fc_en = 1; /* enable tx flow control */
        config0.bits.rx_fc_en = 1; /* enable rx flow control */
        //printk("Enable MAC Flow Control...\n");
    }
    else
    {
        config0.bits.tx_fc_en = 0; /* disable tx flow control */
        config0.bits.rx_fc_en = 0; /* disable rx flow control */
        //printk("Disable MAC Flow Control...\n");
    }
   config0.bits.dis_rx = 1;  /* disable rx */
    config0.bits.dis_tx = 1;  /* disable tx */
    config0.bits.loop_back = 0; /* enable/disable GMAC loopback */
	config0.bits.rgmii_en = 0;
#ifdef LEPUS_ASIC
	config0.bits.rgmm_edge = 1;
	config0.bits.rxc_inv = 0;
#else	
	config0.bits.rgmm_edge =0 ;
	config0.bits.rxc_inv = 0;
#endif	
	config0.bits.ipv4_rx_chksum = 1;  /* enable H/W to check ip checksum */
	config0.bits.ipv6_rx_chksum = 1;  /* enable H/W to check ip checksum */
	config0.bits.port0_chk_hwq = 1;
	config0.bits.port1_chk_hwq = 1;
	config0.bits.port0_chk_toeq = 1;
	config0.bits.port1_chk_toeq = 1;
	config0.bits.port0_chk_classq = 1;
	config0.bits.port1_chk_classq = 1;
	config0.bits.rx_err_detect = 1;

    config0_mask.bits.max_len = 7;
    config0_mask.bits.tx_fc_en = 1;
    config0_mask.bits.rx_fc_en = 1;
    config0_mask.bits.dis_rx = 1;
    config0_mask.bits.dis_tx = 1;
    config0_mask.bits.loop_back = 1;
    config0_mask.bits.rgmii_en = 1;
    config0_mask.bits.rgmm_edge = 1;
    config0_mask.bits.rxc_inv = 1;
	config0_mask.bits.ipv4_rx_chksum = 1;  
	config0_mask.bits.ipv6_rx_chksum = 1;  
	config0_mask.bits.port0_chk_hwq = 1;
	config0_mask.bits.port1_chk_hwq = 1;
	config0_mask.bits.port0_chk_toeq = 1;
	config0_mask.bits.port1_chk_toeq = 1;
	config0_mask.bits.port0_chk_classq = 1;
	config0_mask.bits.port1_chk_classq = 1;
	config0_mask.bits.rx_err_detect = 1;
     gmac_write_reg(tp->base_addr, GMAC_CONFIG0, config0.bits32, config0_mask.bits32);
     
    if(gmac_num)
    {
    	status = 0x796c200;
    	gmac_write_reg(tp->base_addr, GMAC_CONFIG0, status, 0xffffffff);
	}
#if 0
	hw_weigh.bits32 = 0;
	hw_weigh.bits.hw_tq3 = 1;
	hw_weigh.bits.hw_tq2 = 1;
	hw_weigh.bits.hw_tq1 = 1;
	hw_weigh.bits.hw_tq0 = 1;
    gmac_write_reg(tp->dma_base_addr, GMAC_TX_WEIGHTING_CTRL_0_REG, hw_weigh.bits32, 0xffffffff);
	
	sw_weigh.bits32 = 0;
	sw_weigh.bits.sw_tq5 = 1;
	sw_weigh.bits.sw_tq4 = 1;
	sw_weigh.bits.sw_tq3 = 1;
	sw_weigh.bits.sw_tq2 = 1;
	sw_weigh.bits.sw_tq1 = 1;
	sw_weigh.bits.sw_tq0 = 1;
    gmac_write_reg(tp->dma_base_addr, GMAC_TX_WEIGHTING_CTRL_1_REG, sw_weigh.bits32, 0xffffffff);
#endif
	if(gmac_num)
    {
    	tp->pre_phy_status == LINK_UP;
		toe_gmac_enable_tx_rx();
    }
	return (0);
}

/*----------------------------------------------------------------------
* gmac_set_mac_address
*	set mac address
*----------------------------------------------------------------------*/
void gmac_set_mac_address(GMAC_INFO_T *tp, unsigned char *mac1, unsigned char *mac2)
{
	
	UINT32 			data;

	memcpy(tp->mac_addr1, mac1, ETHER_ADDR_LEN);
	memcpy(tp->mac_addr2, mac2, ETHER_ADDR_LEN);
	data = mac1[0] + (mac1[1]<<8) + (mac1[2]<<16) + (mac1[3]<<24);
	gmac_write_reg(tp->base_addr, GMAC_STA_ADD0, data, 0xffffffff);
	data = mac1[4] + (mac1[5]<<8) + (mac2[0]<<16) + (mac2[1]<<24);
	gmac_write_reg(tp->base_addr, GMAC_STA_ADD1, data, 0xffffffff);
	data = mac2[2] + (mac2[3]<<8) + (mac2[4]<<16) + (mac2[5]<<24);
	gmac_write_reg(tp->base_addr, GMAC_STA_ADD2, data, 0xffffffff);

}

/*----------------------------------------------------------------------
*	toe_gmac_enable_tx_rx
*----------------------------------------------------------------------*/
void toe_gmac_enable_tx_rx(void)
{
	int tmp;
	TOE_INFO_T		*toe;
	GMAC_INFO_T 	*tp;
	GMAC_CONFIG0_T	config0,config0_mask;
	
	toe = (TOE_INFO_T *)&toe_private_data;
	tp = (GMAC_INFO_T *)&toe->gmac;
	//printf("toe_gmac_disable_tx_rx tp->base_addr %x\n",tp->base_addr);

    /* enable TX/RX */
    config0.bits32 = 0;
    config0_mask.bits32 = 0;
    config0.bits.dis_rx = 0;  /* enable rx */
    config0.bits.dis_tx = 0;  /* enable tx */
    config0_mask.bits.dis_rx = 1;
    config0_mask.bits.dis_tx = 1;
    gmac_write_reg(tp->base_addr, GMAC_CONFIG0, config0.bits32,config0_mask.bits32);
    
    //printf("toe_gmac_enable_tx_rx tp->base_addr %x  gmac_read_reg(tp->base_addr, GMAC_CONFIG0) : %x\n",tp->base_addr,gmac_read_reg(tp->base_addr, GMAC_CONFIG0));
    //printf("toe->gmac.port_id : %x\n",toe->gmac.port_id);
    //if(gmac_num)
    //{
    //	tmp = 0x796c200;
    //	gmac_write_reg(tp->base_addr, GMAC_CONFIG0, tmp,0xffffffff);
	//}
}

/*----------------------------------------------------------------------
*	toe_gmac_clear_counter
*----------------------------------------------------------------------*/
static int toe_gmac_clear_counter (GMAC_INFO_T *tp)
{
	

    /* clear counter */
    gmac_read_reg(tp->base_addr, GMAC_IN_DISCARDS);
    gmac_read_reg(tp->base_addr, GMAC_IN_ERRORS); 
    gmac_read_reg(tp->base_addr, GMAC_IN_MCAST); 
    gmac_read_reg(tp->base_addr, GMAC_IN_BCAST); 
    gmac_read_reg(tp->base_addr, GMAC_IN_MAC1); 
    gmac_read_reg(tp->base_addr, GMAC_IN_MAC2); 
	//	tp->stats.tx_bytes = 0;
	//	tp->stats.tx_packets = 0;
	//	tp->stats.tx_errors = 0;
	//	tp->stats.rx_bytes = 0;
	//	tp->stats.rx_packets = 0;
	//	tp->stats.rx_errors = 0;
	//	tp->stats.rx_dropped = 0;    
	return (0);    
}

/*----------------------------------------------------------------------
*	toe_gmac_hw_start
*----------------------------------------------------------------------*/
static void toe_gmac_hw_start(GMAC_INFO_T *tp)
{
	
	GMAC_DMA_CTRL_T			dma_ctrl, dma_ctrl_mask;
	
					
    /* program dma control register */	
	dma_ctrl.bits32 = 0;
	dma_ctrl.bits.rd_enable = 1;    
	dma_ctrl.bits.td_enable = 1;    
	dma_ctrl.bits.loopback = 0;    
	dma_ctrl.bits.drop_small_ack = 0;    
	dma_ctrl.bits.rd_prot = 0;    
	dma_ctrl.bits.rd_burst_size = 3;    
	dma_ctrl.bits.rd_insert_bytes = RX_INSERT_BYTES;
	dma_ctrl.bits.rd_bus = 3;    
	dma_ctrl.bits.td_prot = 0;    
	dma_ctrl.bits.td_burst_size = 3;    
	dma_ctrl.bits.td_bus = 3;    
	
	dma_ctrl_mask.bits32 = 0;
	dma_ctrl_mask.bits.rd_enable = 1;    
	dma_ctrl_mask.bits.td_enable = 1;    
	dma_ctrl_mask.bits.loopback = 1;    
	dma_ctrl_mask.bits.drop_small_ack = 1;    
	dma_ctrl_mask.bits.rd_prot = 3;    
	dma_ctrl_mask.bits.rd_burst_size = 3;    
	dma_ctrl_mask.bits.rd_insert_bytes = 3;    
	dma_ctrl_mask.bits.rd_bus = 3;    
	dma_ctrl_mask.bits.td_prot = 0x0f;    
	dma_ctrl_mask.bits.td_burst_size = 3;    
	dma_ctrl_mask.bits.td_bus = 3;    

	gmac_write_reg(tp->dma_base_addr, GMAC_DMA_CTRL_REG, dma_ctrl.bits32, dma_ctrl_mask.bits32);
	
    return;	
}	

/*----------------------------------------------------------------------
*	toe_init_free_queue
*	(1) Initialize the Free Queue Descriptor Base Address & size
*		Register: TOE_GLOBAL_BASE + 0x0004
*	(2) Initialize DMA Read/Write pointer for 
*		SW Free Queue and HW Free Queue
*	(3)	Initialize DMA Descriptors for
*		SW Free Queue and HW Free Queue, 
*----------------------------------------------------------------------*/
void toe_init_free_queue(void)
{
	int 				i, mac;
	TOE_INFO_T			*toe;
	DMA_RWPTR_T			rwptr_reg;
	unsigned int 		rwptr_addr;
	unsigned int		desc_buf,skb_buf;
	GMAC_RXDESC_T		*desc_ptr;
	//struct sk_buff 		*skb;
	unsigned int		buf_ptr, skb_ptr;
	
	toe = (TOE_INFO_T *)&toe_private_data;
	desc_buf = DMA_MALLOC((TOE_SW_FREEQ_DESC_NUM * sizeof(GMAC_RXDESC_T)),
						(dma_addr_t *)&toe->sw_freeq_desc_base_dma) ;
	desc_ptr = (GMAC_RXDESC_T *)desc_buf;
	if (!desc_buf)
	{
		printk("%s::DMA_MALLOC fail !\n",__func__);
		return;
	}
	memset((void *)desc_buf, 0, TOE_SW_FREEQ_DESC_NUM * sizeof(GMAC_RXDESC_T));
	
	// DMA Queue Base & Size
	writel((desc_buf & DMA_Q_BASE_MASK) | TOE_SW_FREEQ_DESC_POWER,
			TOE_GLOBAL_BASE + GLOBAL_SW_FREEQ_BASE_SIZE_REG);
			
	// init descriptor base
	toe->swfq_desc_base = desc_buf;
	
	skb_buf = DMA_MALLOC((TOE_SW_FREEQ_DESC_NUM * RX_BUF_SIZE),
						(dma_addr_t *)&toe->sw_freeq_desc_base_dma) ;
	skb_ptr = skb_buf;
	if (!skb_buf)
	{
		printk("%s::DMA_MALLOC fail !\n",__func__);
		return;
	}
	memset((void *)skb_buf, 0, TOE_SW_FREEQ_DESC_NUM * RX_BUF_SIZE);
	
	
	// SW Free Queue Read/Write Pointer
	rwptr_reg.bits.wptr = TOE_SW_FREEQ_DESC_NUM - 1;
	rwptr_reg.bits.rptr = 0;
	toe->fq_rx_rwptr.bits32 = rwptr_reg.bits32;
	writel(rwptr_reg.bits32, TOE_GLOBAL_BASE + GLOBAL_SWFQ_RWPTR_REG);
	
	// SW Free Queue Descriptors
	for (i=0; i<TOE_SW_FREEQ_DESC_NUM; i++)
	{
		desc_ptr->word0.bits.buffer_size = RX_BUF_SIZE;
		desc_ptr->word1.bits.sw_id = i;	// used to locate skb
		desc_ptr->word2.buf_adr = (unsigned int)(skb_ptr+i*RX_BUF_SIZE);
   		//hal_cache_consistent_sync((unsigned int)desc_ptr, sizeof(GMAC_RXDESC_T), PCI_DMA_TODEVICE);
   		// hal_cache_consistent_sync((unsigned int)skb->data, RX_BUF_SIZE, PCI_DMA_TODEVICE);
   		desc_ptr++;
	}

	// init hardware free queues
	desc_buf = DMA_MALLOC((TOE_HW_FREEQ_DESC_NUM * sizeof(GMAC_RXDESC_T)),
						(dma_addr_t *)&toe->hw_freeq_desc_base_dma) ;
	desc_ptr = (GMAC_RXDESC_T *)desc_buf;
	if (!desc_buf)
	{
		printk("%s::DMA_MALLOC fail !\n",__func__);
		return;
	}
	memset((void *)desc_buf, 0, TOE_HW_FREEQ_DESC_NUM * sizeof(GMAC_RXDESC_T));
	
	// DMA Queue Base & Size
	writel((desc_buf & DMA_Q_BASE_MASK) | TOE_HW_FREEQ_DESC_POWER,
			TOE_GLOBAL_BASE + GLOBAL_HW_FREEQ_BASE_SIZE_REG);
			
	// init descriptor base
	toe->hwfq_desc_base = desc_buf;
	
	// HW Free Queue Read/Write Pointer
	rwptr_reg.bits.wptr = TOE_HW_FREEQ_DESC_NUM - 1;
	rwptr_reg.bits.rptr = 0;
	writel(rwptr_reg.bits32, TOE_GLOBAL_BASE + GLOBAL_HWFQ_RWPTR_REG);
	buf_ptr = DMA_MALLOC(((TOE_HW_FREEQ_DESC_NUM) * RX_BUF_SIZE),
						(dma_addr_t *)&toe->hwfq_buf_base_dma);

	toe->hwfq_buf_base = buf_ptr;
	for (i=0; i<TOE_HW_FREEQ_DESC_NUM; i++)
	{
		desc_ptr->word0.bits.buffer_size = RX_BUF_SIZE;
		desc_ptr->word1.bits.sw_id = i;
		desc_ptr->word2.buf_adr = (unsigned int)(buf_ptr+i*RX_BUF_SIZE);
   		//hal_cache_consistent_sync((unsigned int)desc_ptr, sizeof(GMAC_RXDESC_T), PCI_DMA_TODEVICE);
   		// hal_cache_consistent_sync((unsigned int)buf_ptr, RX_BUF_SIZE, PCI_DMA_TODEVICE);
   		desc_ptr++;
   		//buf_ptr += RX_BUF_SIZE;
	}
}

/*----------------------------------------------------------------------
*	toe_init_swtx_queue
*	(2) Initialize the GMAC 0/1 SW TXQ Queue Descriptor Base Address & size
*		GMAC_SW_TX_QUEUE_BASE_REG(0x0050)
*	(2) Initialize DMA Read/Write pointer for 
*		GMAC 0/1 SW TX Q0-5
*----------------------------------------------------------------------*/
void toe_init_swtx_queue(void)
{
	int 				i, j, mac;
	TOE_INFO_T			*toe;
	DMA_RWPTR_T			rwptr_reg;
	unsigned int 		rwptr_addr;
	unsigned int		desc_buf, txbuf;
	//struct sk_buff 		*skb;
	unsigned int		buf_ptr;
	
	toe = (TOE_INFO_T *)&toe_private_data;
	
	// GMAC-0, SW-TXQ
	// The GMAC-0 and GMAC-0 maybe have different descriptor number
	// so, not use for instruction
	if(!gmac_num)
	{
		desc_buf = DMA_MALLOC((TOE_GMAC0_SWTXQ_DESC_NUM * TOE_SW_TXQ_NUM * sizeof(GMAC_TXDESC_T)),
							(dma_addr_t *)&toe->gmac.swtxq_desc_base_dma) ;
		toe->gmac.swtxq_desc_base = desc_buf;
		if (!desc_buf)
		{
			printk("%s::DMA_MALLOC fail !\n",__func__);
			return;
		}
		memset((void *)desc_buf, 0,	TOE_GMAC0_SWTXQ_DESC_NUM * TOE_SW_TXQ_NUM * sizeof(GMAC_TXDESC_T));
		writel((desc_buf & DMA_Q_BASE_MASK) | TOE_GMAC0_SWTXQ_DESC_POWER,
				TOE_GMAC0_DMA_BASE+ GMAC_SW_TX_QUEUE_BASE_REG);
		

		
		// GMAC0 SW TX Q0-Q5
		rwptr_reg.bits.wptr = 0;
		rwptr_reg.bits.rptr = 0;
		rwptr_addr = TOE_GMAC0_DMA_BASE + GMAC_SW_TX_QUEUE0_PTR_REG;
		

		
		for (i=0; i<TOE_SW_TXQ_NUM; i++)
		{
			toe->gmac.swtxq[i].rwptr_reg = rwptr_addr;
			toe->gmac.swtxq[i].desc_base = desc_buf;
			toe->gmac.swtxq[i].total_desc_num = TOE_GMAC0_SWTXQ_DESC_NUM;
			desc_buf += TOE_GMAC0_SWTXQ_DESC_NUM * sizeof(GMAC_TXDESC_T);
			writel(rwptr_reg.bits32, rwptr_addr);
			
			rwptr_addr+=4;
		}
		
		//
		txbuf = DMA_MALLOC((TOE_GMAC0_SWTXQ_DESC_NUM * TOE_SW_TXQ_NUM * (MAX_ETH_FRAME_SIZE)),
							(dma_addr_t *)&toe->gmac.swtxq_desc_base_dma) ;
		if (!txbuf)
		{
			printk("%s::DMA_MALLOC fail !\n",__func__);
			return;
		}
		memset((void *)txbuf, 0,	TOE_GMAC0_SWTXQ_DESC_NUM * TOE_SW_TXQ_NUM * MAX_ETH_FRAME_SIZE);
		for(j=0;j<TOE_GMAC0_SWTXQ_DESC_NUM;j++)
			txbuf_ptr[j] = txbuf +j*MAX_ETH_FRAME_SIZE;
	}
	else
	{
		desc_buf = DMA_MALLOC((TOE_GMAC1_SWTXQ_DESC_NUM * TOE_SW_TXQ_NUM * sizeof(GMAC_TXDESC_T)),
							(dma_addr_t *)&toe->gmac.swtxq_desc_base_dma) ;
		toe->gmac.swtxq_desc_base = desc_buf;
		if (!desc_buf)
		{
			printk("%s::DMA_MALLOC fail !\n",__func__);
			return;
		}
		memset((void *)desc_buf, 0,	TOE_GMAC1_SWTXQ_DESC_NUM * TOE_SW_TXQ_NUM * sizeof(GMAC_TXDESC_T));
		writel((desc_buf & DMA_Q_BASE_MASK) | TOE_GMAC1_SWTXQ_DESC_POWER,
				TOE_GMAC1_DMA_BASE+ GMAC_SW_TX_QUEUE_BASE_REG);
		

		
		// GMAC0 SW TX Q0-Q5
		rwptr_reg.bits.wptr = 0;
		rwptr_reg.bits.rptr = 0;
		rwptr_addr = TOE_GMAC1_DMA_BASE + GMAC_SW_TX_QUEUE0_PTR_REG;
		

		
		for (i=0; i<TOE_SW_TXQ_NUM; i++)
		{
			toe->gmac.swtxq[i].rwptr_reg = rwptr_addr;
			toe->gmac.swtxq[i].desc_base = desc_buf;
			toe->gmac.swtxq[i].total_desc_num = TOE_GMAC1_SWTXQ_DESC_NUM;
			desc_buf += TOE_GMAC1_SWTXQ_DESC_NUM * sizeof(GMAC_TXDESC_T);
			writel(rwptr_reg.bits32, rwptr_addr);
			
			rwptr_addr+=4;
		}
		
		//
		txbuf = DMA_MALLOC((TOE_GMAC1_SWTXQ_DESC_NUM * TOE_SW_TXQ_NUM * (MAX_ETH_FRAME_SIZE)),
							(dma_addr_t *)&toe->gmac.swtxq_desc_base_dma) ;
		if (!txbuf)
		{
			printk("%s::DMA_MALLOC fail !\n",__func__);
			return;
		}
		memset((void *)txbuf, 0,	TOE_GMAC1_SWTXQ_DESC_NUM * TOE_SW_TXQ_NUM * MAX_ETH_FRAME_SIZE);
		for(j=0;j<TOE_GMAC1_SWTXQ_DESC_NUM;j++)
			txbuf_ptr[j] = txbuf +j*MAX_ETH_FRAME_SIZE;
	}
	
		
}

/*----------------------------------------------------------------------
*	toe_init_default_queue
*	(1) Initialize the default 0/1 Queue Header
*		Register: TOE_DEFAULT_Q0_HDR_BASE (0x60002000)
*				  TOE_DEFAULT_Q1_HDR_BASE (0x60002008)
*	(2)	Initialize Descriptors of Default Queue 0/1
*----------------------------------------------------------------------*/
void toe_init_default_queue(void)
{
	TOE_INFO_T				*toe;
	volatile NONTOE_QHDR_T	*qhdr;
	GMAC_RXDESC_T			*desc_ptr;
	DMA_SKB_SIZE_T			skb_size;
	
	toe = (TOE_INFO_T *)&toe_private_data;
	if(gmac_num)
	{
		desc_ptr = (GMAC_RXDESC_T *)DMA_MALLOC((TOE_DEFAULT_Q1_DESC_NUM * sizeof(GMAC_RXDESC_T)),
												(dma_addr_t *)&toe->gmac.default_desc_base_dma);
		if (!desc_ptr)
		{
			printk("%s::DMA_MALLOC fail !\n",__func__);
			return;
		}
		memset((void *)desc_ptr, 0, TOE_DEFAULT_Q1_DESC_NUM * sizeof(GMAC_RXDESC_T));
		toe->gmac.default_desc_base = (unsigned int)desc_ptr;				
		toe->gmac.default_desc_num = TOE_DEFAULT_Q1_DESC_NUM;
		qhdr = (volatile NONTOE_QHDR_T *)TOE_DEFAULT_Q1_HDR_BASE;
		qhdr->word0.base_size = ((unsigned int)desc_ptr & NONTOE_QHDR0_BASE_MASK) | TOE_DEFAULT_Q1_DESC_POWER;
		qhdr->word1.bits32 = 0;
		toe->gmac.rx_rwptr.bits32 = 0;
		toe->gmac.default_qhdr = (NONTOE_QHDR_T *)qhdr;
	}
	else
	{	
		desc_ptr = (GMAC_RXDESC_T *)DMA_MALLOC((TOE_DEFAULT_Q0_DESC_NUM * sizeof(GMAC_RXDESC_T)),
												(dma_addr_t *)&toe->gmac.default_desc_base_dma);
		if (!desc_ptr)
		{
			printk("%s::DMA_MALLOC fail !\n",__func__);
			return;
		}
		memset((void *)desc_ptr, 0, TOE_DEFAULT_Q0_DESC_NUM * sizeof(GMAC_RXDESC_T));
		toe->gmac.default_desc_base = (unsigned int)desc_ptr;				
		toe->gmac.default_desc_num = TOE_DEFAULT_Q0_DESC_NUM;
		qhdr = (volatile NONTOE_QHDR_T *)TOE_DEFAULT_Q0_HDR_BASE;
		qhdr->word0.base_size = ((unsigned int)desc_ptr & NONTOE_QHDR0_BASE_MASK) | TOE_DEFAULT_Q0_DESC_POWER;
		qhdr->word1.bits32 = 0;
		toe->gmac.rx_rwptr.bits32 = 0;
		toe->gmac.default_qhdr = (NONTOE_QHDR_T *)qhdr;
	}
	
	
	
	skb_size.bits.hw_skb_size = RX_BUF_SIZE;
	skb_size.bits.sw_skb_size = RX_BUF_SIZE;
	writel(skb_size.bits32, TOE_GLOBAL_BASE + GLOBAL_DMA_SKB_SIZE_REG);
}

/*----------------------------------------------------------------------
*	toe_init_interrupt_config
*	Interrupt Select Registers are used to map interrupt to int0 or int1
*	Int0 and int1 are wired to CPU 0/1 GMAC 0/1
* 	Interrupt Device Inteface data are used to pass device info to
*		upper device deiver or store status/statistics
*	ISR handler
*		(1) If status bit ON but masked, the prinf error message (bug issue)
*		(2) If select bits are for me, handle it, else skip to let 
*			the other ISR handles it.
*  Notes:
*		GMACx init routine (for eCOS) or open routine (for Linux)
*       enable the interrupt bits only which are selected for him.
*
*	Default Setting:
*		GMAC0 intr bits ------>	int0 ----> eth0
*		GMAC1 intr bits ------> int1 ----> eth1
*		TOE intr -------------> int0 ----> eth0
*		Classification Intr --> int0 ----> eth1	// eth1: fpga testing, eth0: normal
*		Default Q0 -----------> int0 ----> eth0
*		Default Q1 -----------> int1 ----> eth1
*----------------------------------------------------------------------*/
static void toe_init_interrupt_config(void)
{
	TOE_INFO_T				*toe;
	volatile NONTOE_QHDR_T	*qhdr;
	GMAC_RXDESC_T			*desc_ptr;
	unsigned int			desc_buf_addr;
	int						i, j;
	
	toe = (TOE_INFO_T *)&toe_private_data;
	toe->intr0_select.bits32 = 	GMAC1_TXDERR_INT_BIT	 |
	                           	GMAC1_TXPERR_INT_BIT	 |
								GMAC1_RXPERR_INT_BIT	 |	
	                            GMAC1_SWTQ15_FIN_INT_BIT |
	                            GMAC1_SWTQ14_FIN_INT_BIT |
	                            GMAC1_SWTQ13_FIN_INT_BIT |
	                            GMAC1_SWTQ12_FIN_INT_BIT |
	                            GMAC1_SWTQ11_FIN_INT_BIT |
	                            GMAC1_SWTQ10_FIN_INT_BIT |
	                            GMAC1_SWTQ15_EOF_INT_BIT |
	                            GMAC1_SWTQ14_EOF_INT_BIT |
	                            GMAC1_SWTQ13_EOF_INT_BIT |
	                            GMAC1_SWTQ12_EOF_INT_BIT |
	                            GMAC1_SWTQ11_EOF_INT_BIT |
	                            GMAC1_SWTQ10_EOF_INT_BIT;
	
	toe->intr1_select.bits32 = DEFAULT_Q1_INT_BIT;

	if (!toe->gmac.existed)
	{
		for (i=0; i<TOE_CLASS_QUEUE_NUM; i++)
			toe->intr1_select.bits32 |= CLASS_RX_INT_BIT(i);
		
		toe->intr1_select.bits32 |= TOE_IQ0_INT_BIT	|	
									TOE_IQ1_INT_BIT	|
									TOE_IQ2_INT_BIT	|
									TOE_IQ3_INT_BIT	|
									TOE_IQ0_FULL_INT_BIT	|
									TOE_IQ1_FULL_INT_BIT	|
									TOE_IQ2_FULL_INT_BIT	|
									TOE_IQ3_FULL_INT_BIT;
	}
	
	toe->intr2_select.bits32 = 0;
	toe->intr3_select.bits32 = 0;
	if (!toe->gmac.existed)
	{
		toe->intr2_select.bits32 = 0xffffffff;
		toe->intr3_select.bits32 = 0xffffffff;
	}
	toe->intr4_select.bits32 = GMAC1_RESERVED_INT_BIT		|
	                           GMAC1_MIB_INT_BIT			|
	                           GMAC1_RX_PAUSE_ON_INT_BIT	|
	                           GMAC1_TX_PAUSE_ON_INT_BIT	|
	                           GMAC1_RX_PAUSE_OFF_INT_BIT	|
	                           GMAC1_TX_PAUSE_OFF_INT_BIT	|
	                           GMAC1_RX_OVERRUN_INT_BIT		|
	                           GMAC1_STATUS_CHANGE_INT_BIT;
	
	if (!toe->gmac.existed)
	{
		for (i=0; i<TOE_CLASS_QUEUE_NUM; i++)
			toe->intr4_select.bits32 |= CLASS_RX_FULL_INT_BIT(i);
	}
	
	toe->intr0_device.bits32 = toe->intr0_select.bits32;
	toe->intr1_device.bits32 = toe->intr1_select.bits32;
	toe->intr2_device.bits32 = toe->intr2_select.bits32;
	toe->intr3_device.bits32 = toe->intr3_select.bits32;
	toe->intr4_device.bits32 = toe->intr4_select.bits32;

	// clear all status bits
	writel(0xffffffff, TOE_GLOBAL_BASE + GLOBAL_INTERRUPT_STATUS_0_REG);
	writel(0xffffffff, TOE_GLOBAL_BASE + GLOBAL_INTERRUPT_STATUS_1_REG);
	writel(0xffffffff, TOE_GLOBAL_BASE + GLOBAL_INTERRUPT_STATUS_2_REG);
	writel(0xffffffff, TOE_GLOBAL_BASE + GLOBAL_INTERRUPT_STATUS_3_REG);
	writel(0xffffffff, TOE_GLOBAL_BASE + GLOBAL_INTERRUPT_STATUS_4_REG);
	
	// set select registers
	writel(toe->intr0_device.bits32, TOE_GLOBAL_BASE + GLOBAL_INTERRUPT_SELECT_0_REG);
	writel(toe->intr1_device.bits32, TOE_GLOBAL_BASE + GLOBAL_INTERRUPT_SELECT_1_REG);
	writel(toe->intr2_device.bits32, TOE_GLOBAL_BASE + GLOBAL_INTERRUPT_SELECT_2_REG);
	writel(toe->intr3_device.bits32, TOE_GLOBAL_BASE + GLOBAL_INTERRUPT_SELECT_3_REG);
	writel(toe->intr4_device.bits32, TOE_GLOBAL_BASE + GLOBAL_INTERRUPT_SELECT_4_REG);
	
	// disable all interrupt
	writel(0, TOE_GLOBAL_BASE + GLOBAL_INTERRUPT_ENABLE_0_REG);
	writel(0, TOE_GLOBAL_BASE + GLOBAL_INTERRUPT_ENABLE_1_REG);
	writel(0, TOE_GLOBAL_BASE + GLOBAL_INTERRUPT_ENABLE_2_REG);
	writel(0, TOE_GLOBAL_BASE + GLOBAL_INTERRUPT_ENABLE_3_REG);
	writel(0, TOE_GLOBAL_BASE + GLOBAL_INTERRUPT_ENABLE_4_REG);
}

/*----------------------------------------------------------------------
* mac_stop_txdma
*----------------------------------------------------------------------*/
void mac_stop_txdma(GMAC_INFO_T *tp)
{
	
	GMAC_DMA_CTRL_T			dma_ctrl, dma_ctrl_mask;
	GMAC_TXDMA_FIRST_DESC_T	txdma_busy;

	// wait idle
	txdma_busy.bits32 = gmac_read_reg(tp->dma_base_addr, GMAC_DMA_TX_FIRST_DESC_REG);
	while (txdma_busy.bits.td_busy){};
	
    /* program dma control register */	
	dma_ctrl.bits32 = 0;
	dma_ctrl.bits.rd_enable = 0;    
	dma_ctrl.bits.td_enable = 0;    
	
	dma_ctrl_mask.bits32 = 0;
	dma_ctrl_mask.bits.rd_enable = 1;    
	dma_ctrl_mask.bits.td_enable = 1;    

	gmac_write_reg(tp->dma_base_addr, GMAC_DMA_CTRL_REG, dma_ctrl.bits32, dma_ctrl_mask.bits32);
}

/*----------------------------------------------------------------------
*	toe_gmac_handle_default_rxq
*	(1) Get rx Buffer for default Rx queue
*	(2) notify or call upper-routine to handle it
*	(3) get a new buffer and insert it into SW free queue
*	(4) Note: The SW free queue Read-Write Pointer should be locked when accessing
*----------------------------------------------------------------------*/
// static inline void toe_gmac_handle_default_rxq(GMAC_INFO_T *tp)
static void toe_gmac_handle_default_rxq(GMAC_INFO_T *tp)
{
	TOE_INFO_T			*toe;
    GMAC_RXDESC_T   	*curr_desc, *fq_desc;
	//struct sk_buff 		*skb;
    DMA_RWPTR_T			rwptr, fq_rwptr;
	unsigned int 		pkt_size;
	unsigned int        desc_count,counter;
	unsigned int        good_frame, rx_status, chksum_status;
	struct ether_drv_stats *isPtr = (struct ether_drv_stats *)&tp->ifStatics;
	//register CYG_INTERRUPT_STATE oldints;

	// toe_gmac_disable_interrupt(tp->irq);
	isPtr->rx_deliver++;
   
	rwptr.bits32 = readl(&tp->default_qhdr->word1);
	if (rwptr.bits.rptr != tp->rx_rwptr.bits.rptr)
	{
		mac_stop_txdma(tp);
		printf("Default Queue HW RD ptr (0x%x) != SW RD Ptr (0x%x)\n",
				rwptr.bits32, tp->rx_rwptr.bits.rptr);
		while(1);
	}
	counter =0;
	toe = (TOE_INFO_T *)&toe_private_data;
	while (rwptr.bits.rptr != rwptr.bits.wptr&& counter++<16)
	{	       
    	curr_desc = (GMAC_RXDESC_T *)tp->default_desc_base + rwptr.bits.rptr;
		//hal_cache_consistent_sync(curr_desc, sizeof(GMAC_RXDESC_T), PCI_DMA_FROMDEVICE);
		tp->default_q_cnt++;
    	tp->rx_curr_desc = (unsigned int)curr_desc;
    	rx_status = curr_desc->word0.bits.status;
    	chksum_status = curr_desc->word0.bits.chksum_status;
    	tp->rx_status_cnt[rx_status]++;
		tp->rx_chksum_cnt[chksum_status]++;
    	good_frame = 1;

		if (curr_desc->word0.bits.derr || curr_desc->word0.bits.perr || rx_status
			)
			// || (chksum_status & 0x4))
		{
			good_frame = 0;		
			isPtr->rx_crc_errors++;		// all errors for UI dump
			if (curr_desc->word0.bits.derr)
				printk("%s::derr (GMAC-%d)!!!\n", __func__, tp->port_id);
			if (curr_desc->word0.bits.perr)
				printk("%s::perr (GMAC-%d)!!!\n", __func__, tp->port_id);
			if (rx_status)
			{
				//if (rx_status == 4 || rx_status == 7)
				//	isPtr->rx_crc_errors++;
				//if (rx_status == 6 || rx_status == 7)
				//	isPtr->rx_align_errors++;
				printk("%s::Status=%d (GMAC-%d)!!!\n", __func__, rx_status, tp->port_id);
				mac_status_error = 1;
			}
			if (chksum_status)
				printk("%s::Checksum Status=%d (GMAC-%d)!!!\n", __func__, chksum_status, tp->port_id);

			if (mac_dump_rxpkt)
			{
				printf("Checksum Status=%d\n", chksum_status);
				printf("GMAC %d Rx %d Bytes:\n", tp->port_id, pkt_size);
				dm_byte((u32)curr_desc->word2.buf_adr, (pkt_size > 256) ? 256 : pkt_size);
			}
			//if ((unsigned long)skb>0x10000000 || skb->tag != NET_BUF_TAG)
			//{
			//	//mac_stop_txdma((struct eth_drv_sc *)tp->sc);
			//	dbg_printf(("Illegal NETBUF header!\n"));
			//	while(1);
			//}
			// curr_desc->word2.buf_adr = 0;

		}
		if (good_frame)
		{
			if (curr_desc->word0.bits.drop)
				printk("%s::Drop (GMAC-%d)!!!\n", __func__, tp->port_id);
			//if (chksum_status)
			//	printk("%s::Checksum Status=%d (GMAC-%d)!!!\n", __func__, chksum_status, tp->port_id);
				
	    	/* get frame information from the first descriptor of the frame */
        	pkt_size = curr_desc->word1.bits.byte_count;  /*total byte count in a frame*/
			desc_count = curr_desc->word0.bits.desc_count; /* get descriptor count per frame */
        	
			isPtr->rx_good++;
			//hal_cache_consistent_sync(curr_desc->word2.buf_adr, pkt_size, PCI_DMA_FROMDEVICE);
			
			//skb = (struct sk_buff *)(REG32(curr_desc->word2.buf_adr - SKB_RESERVE_BYTES));
			
			//if ((unsigned long)skb>0x10000000 || skb->tag != NET_BUF_TAG)
			//{
			//	mac_stop_txdma((struct eth_drv_sc *)tp->sc);
			//	dbg_printf(("Illegal NETBUF header!\n"));
			//	while(1);
			//}
			// curr_desc->word2.buf_adr = 0;
			
//			skb_reserve (skb, RX_INSERT_BYTES);	/* 16 byte align the IP fields. */
//			// skb->dev = dev;
//			// skb->ip_summed = CHECKSUM_UNNECESSARY;
//			skb_put(skb, pkt_size);
//			// skb->protocol = eth_type_trans(skb,dev); /* set skb protocol */
//			// netif_rx(skb);  /* socket rx */
//			// dev->last_rx = jiffies;
//			skb->rx_port_id = tp->port_id;
//			skb->rx_qid = tp->port_id + TOE_GMAC0_DEFAULT_QID;
//			skb->sw_id = curr_desc->word1.bits.sw_id;
//			skb->l3_offset = curr_desc->word3.bits.l3_offset;
//			skb->l4_offset = curr_desc->word3.bits.l4_offset;
//			skb->l7_offset = curr_desc->word3.bits.l7_offset;
//			skb->rx_chksum_status = chksum_status;
//			tp->curr_rx_skb = skb;
			if (mac_dump_rxpkt)
			{
				//printf("Checksum Status=%d, L3:0x%x, L4:0x%x, L7:0x%x\n", 
				//		chksum_status, skb->l3_offset, skb->l4_offset, skb->l7_offset);
				printf("GMAC %d Rx %d Bytes:\n", tp->port_id, pkt_size);
				dm_byte(curr_desc->word2.buf_adr, (pkt_size > 128) ? 128 : pkt_size);
			}
			
				enet_input(curr_desc->word2.buf_adr+RX_INSERT_BYTES, pkt_size);
				goto rx_next_frame;
				
			


rx_next_frame:       	    
			isPtr->rx_count++;
        }
        /* release buffer to Remaining Buffer Number Register */
#if 0
        if (tp->flow_control_enable ==1)
        {
			gmac_write_reg(tp->base_addr, GMAC_BNCR, desc_count, 0x0000ffff);
        }
#endif		
		// allocate a buffer and insert to sw free queue
		
		fq_rwptr.bits32 = readl(TOE_GLOBAL_BASE + GLOBAL_SWFQ_RWPTR_REG);
		if (toe_private_data.fq_rx_rwptr.bits.wptr != fq_rwptr.bits.wptr)
		{
			mac_stop_txdma(tp);
			//HAL_RESTORE_INTERRUPTS(oldints);
			printf("Free Queue HW Write ptr (0x%x) != SW Write Ptr(0x%x)\n",
					fq_rwptr.bits.wptr, toe_private_data.fq_rx_rwptr.bits.wptr);
			while(1);
		}

		fq_desc = (GMAC_RXDESC_T *)toe->swfq_desc_base + fq_rwptr.bits.wptr;
   		//hal_cache_consistent_sync((unsigned int)fq_desc, sizeof(GMAC_RXDESC_T), PCI_DMA_TODEVICE);
   		
		// advance one for Rx default Q 0/1
		rwptr.bits.rptr = RWPTR_ADVANCE_ONE(rwptr.bits.rptr, tp->default_desc_num);
		SET_RPTR(&tp->default_qhdr->word1, rwptr.bits.rptr);
     	tp->rx_rwptr.bits32 = rwptr.bits32;
		fq_rwptr.bits.wptr = RWPTR_ADVANCE_ONE(fq_rwptr.bits.wptr, TOE_SW_FREEQ_DESC_NUM);
		SET_WPTR(TOE_GLOBAL_BASE + GLOBAL_SWFQ_RWPTR_REG, fq_rwptr.bits.wptr);
		toe_private_data.fq_rx_rwptr.bits32 = fq_rwptr.bits32;
		//HAL_RESTORE_INTERRUPTS(oldints);
	}
}

/*----------------------------------------------------------------------
*	toe_gmac_deliver
*----------------------------------------------------------------------*/
void toe_gmac_deliver(void)
{
	TOE_INFO_T		*toe;
	GMAC_INFO_T 	*tp;
	int				i;
	
	toe = (TOE_INFO_T *)&toe_private_data;
	tp = (GMAC_INFO_T *)&toe->gmac;

	if ((sys_get_ticks() - gmac_poll_phy_ticks) >= (BOARD_TPS * 3))
	{
		gmac_get_phy_status(tp);
		gmac_poll_phy_ticks = sys_get_ticks();
	}
			
	toe_gmac_handle_default_rxq(tp);
	//for (i=0; i<TOE_CLASS_QUEUE_NUM; i++)
	//	toe_gmac_handle_classq(&toe->classq[i], tp->sc);
    //
	//toe_gmac_handle_toeq(tp);
	// toe_gmac_enable_interrupt(tp->irq);
	/* release buffer to Remaining Buffer Number Register */
    	   
}	

/*----------------------------------------------------------------------
*	toe_gmac_can_send
*		called by eCOS Ethernet I/O
*		check PHY status and TX Q0
*	return 1: if can send, 0: cannot send
*----------------------------------------------------------------------*/
int toe_gmac_can_send(void)
{
	TOE_INFO_T		*toe;
	GMAC_INFO_T 	*tp;
    GMAC_STATUS_T	status;
    DMA_RWPTR_T		rwptr;
    
    
    toe = (TOE_INFO_T *)&toe_private_data;
	tp = (GMAC_INFO_T *)&toe->gmac;
	
    status.bits32 = gmac_read_reg(tp->base_addr, GMAC_STATUS);
    if (!status.bits.link)
    	return 0;
    
    rwptr.bits32 = gmac_read_reg(tp->base_addr, GMAC_SW_TX_QUEUE0_PTR_REG);
    
    // if (wptr + 1) == (rptr), then cannot send
    if (RWPTR_ADVANCE_ONE(rwptr.bits.wptr, tp->swtxq[0].total_desc_num) == rwptr.bits.rptr)
    	return 0;
    else
    	return 1;
}

/*----------------------------------------------------------------------
*	toe_gmac_send
*		key  = 0 if called by mac_send_raw_pkt()
*			!= 0 if called by eCOS Ethernet I/O
*
* TX Queue Selection:
*	For diagnostics (called by mac_send_raw_pkt), the skb->priority = 
*		0: SW TX Q0, 1: SW TX Q1, 2: SW TX Q2, 3: SW TX Q3
*		4: SW TX Q4, others: SW TX Q5
*	For eCOS Ethernet I/O, always use SW TX Q0
*----------------------------------------------------------------------*/
//static void toe_gmac_send(struct eth_drv_sc *sc, struct eth_drv_sg *sg_list, int sg_len,
//                             int total_len, unsigned long key)
void toe_gmac_send(char *bufp, int total_len)                             
{
	TOE_INFO_T		*toe;
	GMAC_INFO_T 	*tp;
    DMA_RWPTR_T				rwptr;
	unsigned char           *tx_buf_adr;
	//struct sk_buff			*skb;
	GMAC_TXDESC_T			*curr_desc;
	unsigned int			tx_qid;
	GMAC_SWTXQ_T			*swtxq;
	int						i, free_desc;
	struct ether_drv_stats *isPtr = (struct ether_drv_stats *)&tp->ifStatics;
	
	toe = (TOE_INFO_T *)&toe_private_data;
	tp = (GMAC_INFO_T *)&toe->gmac;
	
	
	
//#ifdef _BYPASS_SEND
//	eth_drv_tx_done(sc, key, 0);
//	
//	return;
//#endif
	
#if 0
	if (!eth_drv_initialized)
	{
		eth_drv_tx_done(sc,key,-EINVAL);
		return;
	}
#endif

#ifdef _DEBUG_TCP_PERFORMANCE
	net_dump_clock(4);
#endif

	//if (total_len > GMAC_MAX_ETH_FRAME_SIZE || !sg_len || !sg_list->buf)
	if (total_len > GMAC_MAX_ETH_FRAME_SIZE)
	{
		// too long
		isPtr->tx_dropped++;
		//eth_drv_tx_done(sc,key,-EINVAL);
    	return;
    } 
    
   	// Select TX Queue
   	tx_qid = 0;
	swtxq = &tp->swtxq[tx_qid];
  //  cyg_drv_mutex_lock((cyg_drv_mutex_t *)tp->tx_mutex);

   	// if (wptr + 1) == (rptr), then cannot send
    rwptr.bits32 = readl(swtxq->rwptr_reg);
    // if (RWPTR_ADVANCE_ONE(rwptr.bits.wptr, swtxq->total_desc_num) == rwptr.bits.rptr)
	// check finished desc or empty BD
	// cannot check by read ptr of RW PTR register, 
	// because the HW complete to send but the SW may NOT handle it
	if (rwptr.bits.wptr >= rwptr.bits.rptr)
		free_desc = swtxq->total_desc_num - rwptr.bits.wptr - 1 + rwptr.bits.rptr;
	else 
		free_desc = rwptr.bits.rptr - rwptr.bits.wptr - 1;
	if (!free_desc)
	{
		//cyg_drv_mutex_unlock((cyg_drv_mutex_t *)tp->tx_mutex);
		isPtr->tx_dropped++;
		//eth_drv_tx_done(sc,key,-EINVAL);
		printk("Stop here due to no available descriptor!\n");
		while(1);
    	return;
    }
#if 0
	for (i=0; i<1; i++)
	{
		int wptr, rptr;
		
		wptr = rwptr.bits.wptr;
		rptr = rwptr.bits.rptr;
		if (wptr == rptr)
			break;
		wptr = RWPTR_ADVANCE_ONE(wptr, swtxq->total_desc_num);
		if (wptr == rptr)
		{
			//cyg_drv_mutex_unlock((cyg_drv_mutex_t *)tp->tx_mutex);
			isPtr->tx_dropped++;
			//eth_drv_tx_done(sc,key,-EINVAL);
			while(1);
    		return;
		}
	}
#endif

    curr_desc = (GMAC_TXDESC_T *)swtxq->desc_base + rwptr.bits.wptr;
	//hal_cache_consistent_sync(curr_desc, sizeof(GMAC_TXDESC_T), PCI_DMA_FROMDEVICE);

/*    
#if (GMAC_DEBUG==1)
    // if curr_desc->word2.buf_adr !=0 means that the ISR does NOT handle it
    // if (curr_desc->word2.buf_adr)
    if (swtxq->tx_skb[rwptr.bits.wptr])
    {
    	printk("Error! TX descriptor's buffer is not freed!\n");
    	dev_kfree_skb(swtxq->tx_skb[rwptr.bits.wptr]);
    	swtxq->tx_skb[rwptr.bits.wptr] = NULL;
    }
#endif
*/	
	// called by eCOS Ethernet I/O
	//skb = (struct sk_buff *)dev_alloc_skb(total_len);
	//if (!skb)
	//{
	//	isPtr->tx_dropped++;
	//	goto send_end;
	//}
	//tx_buf_adr = skb->data;
	//skb->len = total_len;
	//while (sg_len)
	//{
	//	memcpy(tx_buf_adr,(unsigned char*)sg_list->buf,sg_list->len);
	//	tx_buf_adr += sg_list->len;
	//	++sg_list;
	//	--sg_len;
	//}

	if (mac_dump_txpkt)
	{
		printf("\nGMAC %d TxQid = %d Tx (Size = %d):\n", tp->port_id, tx_qid, total_len);
		dm_byte((u32)bufp, (total_len > 64) ? 64 : total_len);
	}
	
   	swtxq->total_sent++;
	//swtxq->tx_skb[rwptr.bits.wptr] = skb;
		
	/* set TX descriptor */
	/* copy packet to descriptor buffer address */
	curr_desc->word0.bits32 = 0;
	curr_desc->word0.bits.buffer_size = total_len;    /* total frame byte count */
	curr_desc->word1.bits32 = 0;
	curr_desc->word1.bits.udp_chksum = 1;//skb->hw_udp_chksum;
	curr_desc->word1.bits.tcp_chksum = 1;//skb->hw_tcp_chksum;
	curr_desc->word1.bits.ip_chksum = 1;//skb->hw_ip_chksum;
	curr_desc->word1.bits.ipv6_enable = 1;//skb->ipv6_enable;
	curr_desc->word1.bits.mtu_enable = 0;//skb->mtu_enable;
	curr_desc->word1.bits.byte_count = total_len;
	//if(curr_desc->word2.buf_adr==NULL)
	//{
		memcpy((unsigned char *)(txbuf_ptr[rwptr.bits.wptr]), bufp, total_len);
		curr_desc->word2.buf_adr = (unsigned int)txbuf_ptr[rwptr.bits.wptr]; //skb->data;
	//}
	
	curr_desc->word3.bits32 = 0;
	curr_desc->word3.bits.sof_eof = 3;
	curr_desc->word3.bits.eofie = 1;
	curr_desc->word3.bits.mtu_size = 1500;//(skb->mtu_size) ? skb->mtu_size : 1500;
   	swtxq->tx_curr_desc = curr_desc;
 	
	//hal_cache_consistent_sync(skb->data, total_len, PCI_DMA_TODEVICE);
	//hal_cache_consistent_sync(curr_desc, sizeof(GMAC_TXDESC_T), PCI_DMA_TODEVICE);
	
	// Share Pin issue
	//HAL_LOCK_SHARE_PIN();
	SET_WPTR(swtxq->rwptr_reg, RWPTR_ADVANCE_ONE(rwptr.bits.wptr, swtxq->total_desc_num));
	//HAL_UNLOCK_SHARE_PIN();
	
send_end:
	//cyg_drv_mutex_unlock((cyg_drv_mutex_t *)tp->tx_mutex);
	;
	//eth_drv_tx_done(sc, key, 0);
	
}

//static int __init gmac_init_module(void)
int toe_gmac_sl2312_init(void)
{
	int 		i;
	TOE_INFO_T	*toe;
	unsigned long data,val;
	unsigned int 		flag,tmp;
	
	mac_dump_rxpkt = 0;
	mac_dump_txpkt = 0;
	
	if (!gmac_initialized)
	{
		gmac_initialized = 1;
		//fg = REG32(SL2312_GLOBAL_BASE + 0x04);	
		//tmp = REG32(SL2312_GLOBAL_BASE + 0x0);	

		//if((fg&0x40000000)&&((tmp&0x000000FF)==0xc3))
		//{
		//	gmac_num = 1;
		//}
		//else
		//{
		/* 10/100 만 사용 하므로 GigaBit을 쓸 일이 없다.
		 * 따라서 아래와 같이 gmac_num=0으로 고정한다.
		 * 110818 ASTEL by. sis
		 *	if(SPI_get_identifier())
		 *	{
		 *		gmac_num = 1;
		 *		SPI_default();
		 *	}
		 *	else
		 */
				gmac_num = 0;
		//}
			
#ifdef LEPUS_ASIC
		
		/* set GMAC global register */
		val = REG32(SL2312_GLOBAL_BASE+0x10);
    	val = val | 0x005f0000;
    	REG32(SL2312_GLOBAL_BASE+0x10)=val;
    	if(gmac_num)
    		REG32(SL2312_GLOBAL_BASE+0x1c)=0x5787a7f0;
    	else
    		REG32(SL2312_GLOBAL_BASE+0x1c)=0xa7f0a7f0;
    	REG32(SL2312_GLOBAL_BASE+0x20)=0x77777777;
    	REG32(SL2312_GLOBAL_BASE+0x24)=0x77777777;
    	REG32(SL2312_GLOBAL_BASE+0x2c)=0x09200030;
    	//if(gmac_num)
		{
    		val = readl(SL2312_GLOBAL_BASE+0x04);
			if((val&(1<<20))==0){           // GMAC1 enable
 				val = readl(SL2312_GLOBAL_BASE+0x30);
				val = (val & 0xe7ffffff) | 0x08000000;
				flag = readl(SL2312_GLOBAL_BASE + 0x04);	
				tmp = readl(SL2312_GLOBAL_BASE + 0x0);	
    		
				if((flag&0x40000000)&&((tmp&0x000000FF)==0xc3))
					val &= 0xffffffef;
//						val ^= (1 << 4);
				writel(val,SL2312_GLOBAL_BASE+0x30);
			}
		}
#endif	
		
		// init private data
		toe = (TOE_INFO_T *)&toe_private_data;
		memset((void *)toe, 0, sizeof(TOE_INFO_T));
		
		if(gmac_num)
		{
			//printf("gmac_num : %x\n",gmac_num);
			toe->gmac.base_addr = TOE_GMAC1_BASE; //TOE_GMAC0_BASE;
			toe->gmac.dma_base_addr = TOE_GMAC1_DMA_BASE;//TOE_GMAC0_DMA_BASE;
			toe->gmac.auto_nego_cfg = 1;//cfg_get_mac_auto_nego(0);
#ifdef LEPUS_ASIC			
        	toe->gmac.speed_cfg = GMAC_SPEED_1000;
        	toe->gmac.phy_mode = GMAC_PHY_RGMII_1000;//GMAC_PHY_MII;
#else
			toe->gmac.speed_cfg = GMAC_SPEED_100;
			toe->gmac.phy_mode = GMAC_PHY_RGMII_100;//GMAC_PHY_MII;
#endif   
        	toe->gmac.full_duplex_cfg = 1;
        	//toe->gmac.phy_mode = GMAC_PHY_RGMII_1000;//GMAC_PHY_RGMII_100;//GMAC_PHY_MII;
        	toe->gmac.port_id = GMAC_PORT1;//GMAC_PORT0;
        	toe->gmac.phy_id = 1;//0;
        	toe->gmac.phy_addr = 2;//0x11;
        	toe->gmac.irq = SL2312_INTERRUPT_GMAC1;//SL2312_INTERRUPT_GMAC0;
        	memcpy(toe->gmac.mac_addr1, sys_get_mac_addr(1), ETHER_ADDR_LEN);
        	
        	
			// check GMAC 0/1 existed on not
	//		for (i=0; i<gmac_num; i++)
	//		{
				gmac_write_reg(toe->gmac.base_addr, GMAC_STA_ADD2, 0x55aa55aa, 0xffffffff);
				data = gmac_read_reg(toe->gmac.base_addr, GMAC_STA_ADD2);
				if (data == 0x55aa55aa)
					toe->gmac.existed = GMAC_EXISTED_FLAG;
	//		}
		}
		else
		{
			toe->gmac.base_addr = TOE_GMAC0_BASE; //TOE_GMAC0_BASE;
			toe->gmac.dma_base_addr = TOE_GMAC0_DMA_BASE;//TOE_GMAC0_DMA_BASE;
			toe->gmac.auto_nego_cfg = 1;//cfg_get_mac_auto_nego(0);

#ifdef LEPUS_ASIC
  #if __ORIGINAL__
			toe->gmac.speed_cfg = GMAC_SPEED_1000;
			toe->gmac.phy_mode = GMAC_PHY_RGMII_1000;	//GMAC_PHY_MII;
  #else	// ASTEL - by. sis
			/* 10/100 이며 MII 모드로 설정
			 */
			toe->gmac.speed_cfg = GMAC_SPEED_100;		// ASTEL by. sis
			toe->gmac.phy_mode  = GMAC_PHY_MII;			//GMAC_PHY_MII;
  #endif
#else
			toe->gmac.speed_cfg = GMAC_SPEED_100;
			toe->gmac.phy_mode = GMAC_PHY_RGMII_100;	//GMAC_PHY_MII;
#endif   

        	toe->gmac.full_duplex_cfg = 1;
        	toe->gmac.port_id = GMAC_PORT0;//GMAC_PORT0;
        	toe->gmac.phy_id = 0;//0;
        	toe->gmac.phy_addr = 0x1;//1;//0x11;
        	toe->gmac.irq = SL2312_INTERRUPT_GMAC0;//SL2312_INTERRUPT_GMAC0;
        	memcpy(toe->gmac.mac_addr1, sys_get_mac_addr(0), ETHER_ADDR_LEN);
        	
        	
			// check GMAC 0/1 existed on not
	//		for (i=0; i<GMAC_NUM; i++)
	//		{
				gmac_write_reg(toe->gmac.base_addr, GMAC_STA_ADD2, 0x55aa55aa, 0xffffffff);
				data = gmac_read_reg(toe->gmac.base_addr, GMAC_STA_ADD2);
				if (data == 0x55aa55aa)
					toe->gmac.existed = GMAC_EXISTED_FLAG;
	//		}
		}	
		//FLAG_SWITCH = 0;
		toe_gmac_sw_reset();
		toe_init_free_queue();
		toe_init_swtx_queue();
		//toe_init_hwtx_queue();
		toe_init_default_queue();
		//toe_init_toe_queue();
		//toe_init_class_queue();
		//toe_init_interrupt_queue();
		//toe_init_interrupt_config();
		
		
		// Write GLOBAL_QUEUE_THRESHOLD_REG
		// TBD
		
printk("existed=%d\n", toe->gmac.existed);
		if (toe->gmac.existed)
			toe_init_gmac(&toe->gmac);
#if 0
#endif
				
	}
	//printf("toe->gmac.port_id : %x\n",toe->gmac.port_id);
	
    return TRUE;
}	

/*----------------------------------------------------------------------
*	toe_init_gmac
*----------------------------------------------------------------------*/
bool toe_init_gmac(GMAC_INFO_T *tp)
{
	
	TOE_INFO_T				*toe;
	struct ether_drv_stats	*isPtr = (struct ether_drv_stats *)&tp->ifStatics;
	
	if (!gmac_initialized)
		return TRUE;
		
	//tp->ndt = (void *)tab;
	//tp->sc = sc;
	//tp->flow_control_enable = 1;
	tp->pre_phy_status = LINK_DOWN;
	tp->full_duplex_status = tp->full_duplex_cfg;
	tp->speed_status = tp->speed_status;

	//cyg_drv_mutex_init((cyg_drv_mutex_t *)&tx_mutex[tp->port_id]);
	//tp->tx_mutex = (void *)&tx_mutex[tp->port_id];

    /* set PHY register to start autonegition process */
    gmac_set_phy_status(tp);

	//tp->flow_control_enable = 1;

	/* GMAC initialization */
	if ( toe_gmac_init_chip(tp) ) 
	{
		printk ("GMAC %d init fail\n", tp->port_id);
	}	

    /* allocates tx/rx descriptor and buffer memory */
    /* enable tx/rx register */    
    toe_gmac_enable_tx_rx();
    
    /* clear statistic counter */
    toe_gmac_clear_counter(tp);
	
	memset((void *)&tp->ifStatics, 0, sizeof(struct ether_drv_stats));
	isPtr->duplex = 3;
	isPtr->speed = 100000000;
	isPtr->rx_resource = gmac_num ? 
						  TOE_DEFAULT_Q1_DESC_NUM : TOE_DEFAULT_Q0_DESC_NUM;
	isPtr->tx_queue_len = tp->swtxq[0].total_desc_num;
	strcpy(isPtr->description, LEPUS_DRIVER_NAME);
	strcpy(isPtr->snmp_chipset, DRV_NAME);

	/* -----------------------------------------------------------
	Enable GMAC interrupt & disable loopback 
	Notes:
		GMACx init routine (for eCOS) or open routine (for Linux)
		enable the interrupt bits only which are selected for him.
	--------------------------------------------------------------*/
	toe = (TOE_INFO_T *)&toe_private_data;
	
	// Enable Interrupt Bits
	if (tp->port_id == 0)
	{
		tp->intr0_enabled = tp->intr0_selected = ~toe->intr0_select.bits32;
		tp->intr1_enabled = tp->intr1_selected = ~toe->intr1_select.bits32;
		tp->intr2_enabled = tp->intr2_selected = ~toe->intr2_select.bits32;
		tp->intr3_enabled = tp->intr3_selected = ~toe->intr3_select.bits32;
		tp->intr4_enabled = tp->intr4_selected = ~toe->intr4_select.bits32;
		
	}
	else
	{
		tp->intr0_enabled = tp->intr0_selected = toe->intr0_select.bits32;
		tp->intr1_enabled = tp->intr1_selected = toe->intr1_select.bits32;
		tp->intr2_enabled = tp->intr2_selected = toe->intr2_select.bits32; 
		tp->intr3_enabled = tp->intr3_selected = toe->intr3_select.bits32; 
		tp->intr4_enabled = tp->intr4_selected = toe->intr4_select.bits32; 
	}
	
	// enable only selected bits
	gmac_write_reg(TOE_GLOBAL_BASE, GLOBAL_INTERRUPT_ENABLE_0_REG, 
					tp->intr0_enabled, tp->intr0_selected);
	gmac_write_reg(TOE_GLOBAL_BASE, GLOBAL_INTERRUPT_ENABLE_1_REG, 
					tp->intr1_enabled, tp->intr1_selected);
	gmac_write_reg(TOE_GLOBAL_BASE, GLOBAL_INTERRUPT_ENABLE_2_REG, 
					tp->intr2_enabled, tp->intr2_selected);
	gmac_write_reg(TOE_GLOBAL_BASE, GLOBAL_INTERRUPT_ENABLE_3_REG, 
					tp->intr3_enabled, tp->intr3_selected);
	gmac_write_reg(TOE_GLOBAL_BASE, GLOBAL_INTERRUPT_ENABLE_4_REG, 
					tp->intr4_enabled, tp->intr4_selected);

	// Level Trigger, Actiev HIGH
	//hal_interrupt_configure(tp->irq, 1, 1);
	//cyg_drv_interrupt_create(tp->irq,
	//						 0,
	//						 (unsigned int)sc,
	//						 toe_gmac_isr,
	//						 eth_drv_dsr,
	//						 &intr_handle[tp->port_id],
	//						 &intr_object[tp->port_id]);
  	//cyg_drv_interrupt_attach(intr_handle[tp->port_id]);

    /* start DMA process */
    if (tp->existed)
    {
		toe_gmac_hw_start(tp);

		//toe_gmac_enable_interrupt(tp->irq);
	
	//	eth_drv_init(sc, tp->mac_addr1);
	}
	tp->flow_control_enable = 1;
    return TRUE;
}	



#endif // LEPUS_FPGA

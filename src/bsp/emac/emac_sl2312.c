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

#ifndef MIDWAY
#include "emac_sl2312.h"

#define diag_printf			printf
    
unsigned int FLAG_SWITCH = 0;	/* if 1-->switch chip presented. if 0-->switch chip unpresented */
int vlan_enabled = 0;
/*************************************************************
 *         Global Variable
 *************************************************************/
EMAC_INFO_T emac_private_data;
//static unsigned int     tx_free_desc = TX_DESC_NUM;
static unsigned int     flow_control_enable = 0;
//static unsigned int     tx_desc_virtual_base = 0;
//static unsigned int     rx_desc_virtual_base = 0;
//static unsigned int     tx_buf_virtual_base = 0;
//static unsigned int     rx_buf_virtual_base = 0;
unsigned int     		full_duplex = 1;
unsigned int			chip_id=0;

static unsigned char    tx_desc_array[TX_DESC_NUM * sizeof(EMAC_DESCRIPTOR_T)] __attribute__((aligned(32))); 
static unsigned char    rx_desc_array[RX_DESC_NUM * sizeof(EMAC_DESCRIPTOR_T)] __attribute__((aligned(32)));
static unsigned char    tx_buf_array[TX_BUF_TOT_LEN] __attribute__((aligned(32)));
static unsigned char    rx_buf_array[RX_BUF_TOT_LEN] __attribute__((aligned(32)));


/************************************************/
/*                 function declare             */
/************************************************/
void emac_set_mac_addr(unsigned char *mac1, unsigned char *mac2);
void emac_enable_tx_rx(void);
void emac_disable_tx_rx(void);
extern void emac_set_phy_status(void);
extern void emac_get_phy_status(void);
extern char *sys_get_mac_addr(int index);

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

#define emac_read_reg(offset)					REG32(EMAC_BASE_ADDR + offset)
#define emac_write_reg(offset, data, mask)		REG32(EMAC_BASE_ADDR + offset) = 	\
												(emac_read_reg(offset) & (~mask)) | \
												(data & mask)

        			
static int emac_init_chip(void)
{
	EMAC_RBNR_T		rbnr_val,rbnr_mask;
	EMAC_CONFIG2_T	config2_val;
	EMAC_CONFIG0_T	config0,config0_mask;
	EMAC_CONFIG1_T	config1;
	UINT32 data;

	/* chip reset */
	 
    /* set RMII mode for testing */
    if (full_duplex == 1)
    {
        emac_write_reg(EMAC_STATUS,0x00000017,0x0000001f);  /* 100M full duplex */
    }
    else
    {    
        emac_write_reg(EMAC_STATUS,0x00000013,0x0000001f);    /* 100M half duplex */
	}

    emac_set_mac_addr(sys_get_mac_addr(0), sys_get_mac_addr(1));
    
    /* set RX_FLTR register to receive all multicast packet */
    emac_write_reg(EMAC_RX_FLTR,0x00000005,0x0000001f);
//    emac_write_reg(EMAC_RX_FLTR,0x00000001,0x0000001f);

	/* set per packet buffer size */
	config1.bits32 = 0;
    config1.bits.buf_size = 11; /* buffer size = 2048-byte */
    emac_write_reg(EMAC_CONFIG1,config1.bits32,0x0000000f);
	
	/* set flow control threshold */
	config2_val.bits32 = 0;
	config2_val.bits.set_threshold = RX_DESC_NUM/4;
	config2_val.bits.rel_threshold = RX_DESC_NUM*3/4;
	emac_write_reg(EMAC_CONFIG2,config2_val.bits32,0xffffffff);

	/* init remaining buffer number register */
	rbnr_val.bits32 = 0;
	rbnr_val.bits.buf_remain = RX_DESC_NUM;
	rbnr_mask.bits32 = 0;
	rbnr_mask.bits.buf_remain = 0xffff;	 
	emac_write_reg(EMAC_RBNR,rbnr_val.bits32,rbnr_mask.bits32);

    /* disable TX/RX and disable internal loop back */
    config0.bits32 = 0;
    config0_mask.bits32 = 0;
    config0.bits.max_len = 2;
    config0.bits.fc_en = 0; /* enable flow control */
    config0.bits.dis_rx = 1;  /* disable rx */
    config0.bits.dis_tx = 1;  /* disable tx */
    config0.bits.loop_back = 0; /* enable/disable EMAC loopback */
    config0_mask.bits.max_len = 3;
    config0_mask.bits.fc_en = 1;
    config0_mask.bits.dis_rx = 1;
    config0_mask.bits.dis_tx = 1;
    config0_mask.bits.loop_back = 1;
    emac_write_reg(EMAC_CONFIG0,config0.bits32,config0_mask.bits32);
    
	flow_control_enable = 0; /* disable flow control flag */
	return (0);
}

void emac_enable_tx_rx(void)
{
	EMAC_CONFIG0_T	config0,config0_mask;

    /* enable TX/RX */
    config0.bits32 = 0;
    config0_mask.bits32 = 0;
    config0.bits.dis_rx = 0;  /* enable rx */
    config0.bits.dis_tx = 0;  /* enable tx */
    config0_mask.bits.dis_rx = 1;
    config0_mask.bits.dis_tx = 1;
    emac_write_reg(EMAC_CONFIG0,config0.bits32,config0_mask.bits32);    
}

void emac_disable_tx_rx(void)
{
	EMAC_CONFIG0_T	config0,config0_mask;

    /* enable TX/RX */
    config0.bits32 = 0;
    config0_mask.bits32 = 0;
    config0.bits.dis_rx = 1;  /* disable rx */
    config0.bits.dis_tx = 1;  /* disable tx */
    config0_mask.bits.dis_rx = 1;
    config0_mask.bits.dis_tx = 1;
    emac_write_reg(EMAC_CONFIG0,config0.bits32,config0_mask.bits32);    
}
    
#if 0
static void emac_tx_interrupt(void)
{
	EMAC_INFO_T     		*tp = (EMAC_INFO_T *)&emac_private_data;
    EMAC_DESCRIPTOR_T	    *tx_hw_complete_desc;
    unsigned int desc_cnt=0;
    unsigned int i;

	/* get tx H/W completed descriptor virtual address */
	tx_hw_complete_desc = (EMAC_DESCRIPTOR_T *)(emac_read_reg(EMAC_TXDMA_CURR_DESC) & 0xfffffff0);
	/* check tx status and accumulate tx statistics */
    for (;;)
    {
        if (tp->tx_finished_desc == tx_hw_complete_desc)   /* complete tx processing */ 
        {
            break;
        }    
    	if (tp->tx_finished_desc->frame_ctrl.bits_tx.own == CPU)
    	{
    	    if ( (tp->tx_finished_desc->frame_ctrl.bits_tx.derr) ||
    	         (tp->tx_finished_desc->frame_ctrl.bits_tx.perr) )
    	    {
    	        diag_printf("emac_tx_interrupt::Descriptor Processing Error !!!\n");
    	    }
    	              
            if (tp->tx_finished_desc->frame_ctrl.bits_tx.success_tx == 1)
            {
//                tp->stats.tx_bytes += tp->tx_finished_desc->flag_status.bits_tx_flag.frame_count;
//                tp->stats.tx_packets ++;
            }
            else
            {
//                tp->stats.tx_errors++;
            }
            desc_cnt = tp->tx_finished_desc->frame_ctrl.bits_tx.desc_count;
        	for (i=1; i<desc_cnt; i++)  /* multi_descriptor */
        	{
        	    /* release Tx descriptor to CPU */
                tp->tx_finished_desc = (EMAC_DESCRIPTOR_T *)((tp->tx_finished_desc->next_desc.next_descriptor & 0xfffffff0));
                tp->tx_finished_desc->frame_ctrl.bits_tx.own = CPU;
         	}
            tp->tx_finished_desc = (EMAC_DESCRIPTOR_T *)((tp->tx_finished_desc->next_desc.next_descriptor & 0xfffffff0));
     	}
    }
//    REG32(SL2312_GLOBAL_BASE+GLOBAL_MISC_CTRL) &= ~GLOBAL_PFLASH_EN_BIT;//~0x00000001;
    	    
}	
#endif

static void emac_weird_interrupt(void)
{
}

void sl2312_emac_deliver(void)
{
	EMAC_INFO_T 			*tp = (EMAC_INFO_T *)&emac_private_data;
    EMAC_DESCRIPTOR_T   	*rx_desc;
	unsigned int 			pkt_len;
	unsigned int        	own;
	unsigned int        	desc_count;
	EMAC_RXDMA_FIRST_DESC_T	rxdma_busy;

    own = tp->rx_cur_desc->frame_ctrl.bits32 >> 31;
    if (own == CPU) /* check owner bit */
    {
        rx_desc = tp->rx_cur_desc;
        /* check error interrupt */
        if ( (rx_desc->frame_ctrl.bits_rx.derr==1)||(rx_desc->frame_ctrl.bits_rx.perr==1) )
        {
	        diag_printf("emac_rx_interrupt::Rx Descriptor Processing Error !!!\n");
	    }
	    
	    /* get frame information from the first descriptor of the frame */
        pkt_len = rx_desc->flag_status.bits_rx_status.frame_count-4;  /*total byte count in a frame*/
		desc_count = rx_desc->frame_ctrl.bits_rx.desc_count; /* get descriptor count per frame */

		if ((pkt_len > 0) && (rx_desc->frame_ctrl.bits_rx.frame_state == 0x000)) /* good frame */
		{
            // (sc->funs->eth_drv->recv)(sc, pkt_len);
            // queue it
            tp->rx_pkts++;
            enet_input(rx_desc->buf_adr, pkt_len);
		}
	    
	    rx_desc->frame_ctrl.bits_rx.own = DMA; /* release rx descriptor to DMA */
        /* point to next rx descriptor */             
        tp->rx_cur_desc = (EMAC_DESCRIPTOR_T *)(tp->rx_cur_desc->next_desc.next_descriptor & 0xfffffff0);
 		
        /* release buffer to Remaining Buffer Number Register */
        if (flow_control_enable ==1)
        {
            emac_write_reg(EMAC_BNCR, desc_count, 0x0000ffff);
        }
	}
    
    /* if RX DMA process is stoped , restart it */    
	rxdma_busy.bits.rd_first_des_ptr = emac_read_reg(EMAC_RXDMA_FIRST_DESC);
	if (rxdma_busy.bits.rd_busy == 0)
	{
    	EMAC_RXDMA_CTRL_T rxdma_ctrl,rxdma_ctrl_mask;
	    rxdma_ctrl.bits32 = 0;
    	rxdma_ctrl.bits.rd_start = 1;    /* start RX DMA transfer */
	    rxdma_ctrl.bits.rd_continue = 1; /* continue RX DMA operation */
	    rxdma_ctrl_mask.bits32 = 0;
    	rxdma_ctrl_mask.bits.rd_start = 1;   
	    rxdma_ctrl_mask.bits.rd_continue = 1; 
	    emac_write_reg(EMAC_RXDMA_CTRL,rxdma_ctrl.bits32,rxdma_ctrl_mask.bits32);
    }

}

/* The interrupt handler does all of the Rx thread work and cleans up
   after the Tx thread. */
//static void emac_interrupt (int irq, void *dev_instance, struct pt_regs *regs)
void emac_sl2312_poll(void)
{
	EMAC_RXDMA_FIRST_DESC_T	rxdma_busy;
	EMAC_TXDMA_FIRST_DESC_T	txdma_busy;
    EMAC_TXDMA_CTRL_T       txdma_ctrl,txdma_ctrl_mask;
    EMAC_RXDMA_CTRL_T       rxdma_ctrl,rxdma_ctrl_mask;
	EMAC_DMA_STATUS_T	    status;
		
    // for (;;)
    {
        /* read DMA status */
	    status.bits32 = emac_read_reg(EMAC_DMA_STATUS);

	    /* clear DMA status */
        emac_write_reg(EMAC_DMA_STATUS,status.bits32,status.bits32);	
        
        if ((status.bits32 & 0xffffc000)==0)
        {
            // break;
            return;
        }  
          
	    if (status.bits.rx_overrun == 1)
	    {
	    	emac_private_data.rx_overrun++;
            /* if RX DMA process is stoped , restart it */    
	        rxdma_busy.bits32 = emac_read_reg(EMAC_RXDMA_FIRST_DESC) ;
	        if (rxdma_busy.bits.rd_busy == 0)
	        {
	            /* restart Rx DMA process */
    	        rxdma_ctrl.bits32 = 0;
    	        rxdma_ctrl.bits.rd_start = 1;    /* start RX DMA transfer */
	            rxdma_ctrl.bits.rd_continue = 1; /* continue RX DMA operation */
	            rxdma_ctrl_mask.bits32 = 0;
    	        rxdma_ctrl_mask.bits.rd_start = 1;   
	            rxdma_ctrl_mask.bits.rd_continue = 1; 
	            emac_write_reg(EMAC_RXDMA_CTRL,rxdma_ctrl.bits32,rxdma_ctrl_mask.bits32);
            }
	    }
	    if (status.bits.tx_underrun == 1)
	    {
            /* if TX DMA process is stoped , restart it */
            emac_private_data.tx_underrun++;
	        txdma_busy.bits32 = emac_read_reg(EMAC_TXDMA_FIRST_DESC);
	        if (txdma_busy.bits.td_busy == 0)
	        {
		        /* restart Tx DMA process */
		        txdma_ctrl.bits32 = 0;
		        txdma_ctrl.bits.td_start = 1;
		        txdma_ctrl.bits.td_continue = 1;
		        txdma_ctrl_mask.bits32 = 0;
		        txdma_ctrl_mask.bits.td_start = 1;
		        txdma_ctrl_mask.bits.td_continue = 1;
		        emac_write_reg(EMAC_TXDMA_CTRL,txdma_ctrl.bits32,txdma_ctrl_mask.bits32);
	        }
	    }
	    
        /* receive rx interrupt */
    	if ( ((status.bits.rs_eofi==1)||(status.bits.rs_finish==1))||
    	     ((status.bits.ts_eofi==1)||(status.bits.ts_finish==1)) )
    	{
//    		emac_rx_interrupt(dev); 
            sl2312_emac_deliver();	
        }
        
        /* receive tx interrupt */
//    	if ( ((status.bits.ts_eofi==1)||(status.bits.ts_finish==1)) )
//    	{
//    		emac_tx_interrupt(dev);
//    	}
    	
    	/* check uncommon events */	
        if ((status.bits32 & 0x633fc000)!=0)
        {
            emac_weird_interrupt();
        }    
	}
	
	return;
}

static void emac_hw_start(void)
{
	EMAC_INFO_T     		*tp = (EMAC_INFO_T *)&emac_private_data;
	EMAC_TXDMA_CURR_DESC_T	tx_desc;
	EMAC_RXDMA_CURR_DESC_T	rx_desc;
    EMAC_TXDMA_CTRL_T       txdma_ctrl,txdma_ctrl_mask;
    EMAC_RXDMA_CTRL_T       rxdma_ctrl,rxdma_ctrl_mask;
	EMAC_DMA_STATUS_T       dma_status,dma_status_mask;
	
	/* program TxDMA Current Descriptor Address register for first descriptor */
	tx_desc.bits32 = (unsigned int)(tp->tx_desc_dma);
	tx_desc.bits.eofie = 1;
	tx_desc.bits.dec = 0;
	tx_desc.bits.sof_eof = 0x03;
	emac_write_reg(EMAC_TXDMA_CURR_DESC,tx_desc.bits32,0xffffffff);
	emac_write_reg(0xff2c,tx_desc.bits32,0xffffffff);   /* tx next descriptor address */
	
	/* program RxDMA Current Descriptor Address register for first descriptor */
	rx_desc.bits32 = (unsigned int)(tp->rx_desc_dma);
	rx_desc.bits.eofie = 1;
	rx_desc.bits.dec = 0;
	rx_desc.bits.sof_eof = 0x03;
	emac_write_reg(EMAC_RXDMA_CURR_DESC,rx_desc.bits32,0xffffffff);
	emac_write_reg(0xff3c,rx_desc.bits32,0xffffffff);   /* rx next descriptor address */
	    	
	/* enable EMAC interrupt & disable loopback */
	dma_status.bits32 = 0;
	dma_status.bits.loop_back = 0;  /* disable DMA loop-back mode */
	dma_status.bits.m_tx_fail = 1;
	dma_status.bits.m_cnt_full = 1;
	dma_status.bits.m_rx_pause_on = 1;
	dma_status.bits.m_tx_pause_on = 1;
	dma_status.bits.m_rx_pause_off = 1;
	dma_status.bits.m_tx_pause_off = 1;
	dma_status.bits.m_rx_overrun = 1;
	dma_status.bits.m_tx_underrun = 1;
	dma_status_mask.bits32 = 0;
	dma_status_mask.bits.loop_back = 1;
	dma_status_mask.bits.m_tx_fail = 1;
	dma_status_mask.bits.m_cnt_full = 1;
	dma_status_mask.bits.m_rx_pause_on = 1;
	dma_status_mask.bits.m_tx_pause_on = 1;
	dma_status_mask.bits.m_rx_pause_off = 1;
	dma_status_mask.bits.m_tx_pause_off = 1;
	dma_status_mask.bits.m_rx_overrun = 1;
	dma_status_mask.bits.m_tx_underrun = 1;
	emac_write_reg(EMAC_DMA_STATUS,dma_status.bits32,dma_status_mask.bits32);

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
	emac_write_reg(EMAC_TXDMA_CTRL,txdma_ctrl.bits32,txdma_ctrl_mask.bits32);
	
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
	emac_write_reg(EMAC_RXDMA_CTRL,rxdma_ctrl.bits32,rxdma_ctrl_mask.bits32);
    return;	
}	
static void emac_hw_stop(void)
{
    EMAC_TXDMA_CTRL_T       txdma_ctrl,txdma_ctrl_mask;
    EMAC_RXDMA_CTRL_T       rxdma_ctrl,rxdma_ctrl_mask;

    /* program tx dma control register */	
	txdma_ctrl.bits32 = 0;
	txdma_ctrl.bits.td_start = 0;    
	txdma_ctrl.bits.td_continue = 0; 
	txdma_ctrl_mask.bits32 = 0;
	txdma_ctrl_mask.bits.td_start = 1;    
	txdma_ctrl_mask.bits.td_continue = 1; 	
	emac_write_reg(EMAC_TXDMA_CTRL,txdma_ctrl.bits32,txdma_ctrl_mask.bits32);
    /* program rx dma control register */	
	rxdma_ctrl.bits32 = 0;
	rxdma_ctrl.bits.rd_start = 0;    /* stop RX DMA transfer */
	rxdma_ctrl.bits.rd_continue = 0; /* stop continue RX DMA operation */
	rxdma_ctrl_mask.bits32 = 0;
	rxdma_ctrl_mask.bits.rd_start = 1;    
	rxdma_ctrl_mask.bits.rd_continue = 1; 
	emac_write_reg(EMAC_RXDMA_CTRL,rxdma_ctrl.bits32,rxdma_ctrl_mask.bits32);
}

static int emac_init_desc_buf(void)
{
	EMAC_INFO_T  *tp = (EMAC_INFO_T *)&emac_private_data;
	dma_addr_t   tx_first_desc_dma;
	dma_addr_t   rx_first_desc_dma;
	dma_addr_t   tx_first_buf_dma;
	dma_addr_t   rx_first_buf_dma;
	int    i;

	/* allocates TX/RX DMA packet buffer */
	tp->tx_bufs = (unsigned char *)(((int)&tx_buf_array[0]));
	memset(tp->tx_bufs,0x00,TX_BUF_TOT_LEN);
	tp->rx_bufs = (unsigned char *)(((int)&rx_buf_array[0]));
	memset(tp->rx_bufs,0x00,RX_BUF_TOT_LEN);
    tp->tx_bufs_dma = (unsigned int)tp->tx_bufs;
    tp->rx_bufs_dma = (unsigned int)tp->rx_bufs;
//    diag_printf("tx_bufs = %08x\n",(unsigned int)tp->tx_bufs);
//    diag_printf("rx_bufs = %08x\n",(unsigned int)tp->rx_bufs);
	
	/* allocates TX/RX descriptors */
	tp->tx_desc = (EMAC_DESCRIPTOR_T *)(((int)&tx_desc_array[0]));
    memset(tp->tx_desc,0x00,TX_DESC_NUM*sizeof(EMAC_DESCRIPTOR_T));
	tp->rx_desc = (EMAC_DESCRIPTOR_T *)(((int)&rx_desc_array[0]));
    memset(tp->rx_desc,0x00,RX_DESC_NUM*sizeof(EMAC_DESCRIPTOR_T));
//    diag_printf("tx_desc = %08x\n",(unsigned int)tp->tx_desc);
//    diag_printf("rx_desc = %08x\n",(unsigned int)tp->rx_desc);
	
	/* TX descriptors initial */
	tp->tx_cur_desc = tp->tx_desc;  /* virtual address */
	tp->tx_finished_desc = tp->tx_desc; /* virtual address */
	tp->tx_desc_dma = (unsigned int)tp->tx_desc;
	tx_first_desc_dma = tp->tx_desc_dma; /* physical address */
    tx_first_buf_dma = tp->tx_bufs_dma;
	for (i = 1; i < TX_DESC_NUM; i++)
	{
		tp->tx_desc->frame_ctrl.bits_tx.own = CPU; /* set owner to CPU */
		tp->tx_desc->frame_ctrl.bits_tx.buffer_size = TX_BUF_SIZE;  /* set tx buffer size for descriptor */
		tp->tx_desc->buf_adr = tp->tx_bufs_dma; /* set data buffer address */
		tp->tx_bufs_dma = tp->tx_bufs_dma + TX_BUF_SIZE; /* point to next buffer address */
		tp->tx_desc_dma = tp->tx_desc_dma + sizeof(EMAC_DESCRIPTOR_T); /* next tx descriptor DMA address */
		tp->tx_desc->next_desc.next_descriptor = tp->tx_desc_dma | 0x0000000b;
		tp->tx_desc = &tp->tx_desc[1] ; /* next tx descriptor virtual address */
	}
	/* the last descriptor will point back to first descriptor */
	tp->tx_desc->frame_ctrl.bits_tx.own = CPU;
	tp->tx_desc->frame_ctrl.bits_tx.buffer_size = TX_BUF_SIZE;
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
		tp->rx_desc_dma = tp->rx_desc_dma + sizeof(EMAC_DESCRIPTOR_T); /* next rx descriptor DMA address */
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

static int emac_clear_counter (void)
{
//	EMAC_INFO_T     *tp = (EMAC_INFO_T *)&emac_private_data;

    /* clear counter */
    emac_read_reg(EMAC_IN_DISCARDS);
    emac_read_reg(EMAC_IN_ERRORS); 
//    tp->stats.tx_bytes = 0;
//    tp->stats.tx_packets = 0;
//	tp->stats.tx_errors = 0;
//    tp->stats.rx_bytes = 0;
//	tp->stats.rx_packets = 0;
//	tp->stats.rx_errors = 0;
//    tp->stats.rx_dropped = 0;    
	return (0);    
}
   			
//static int emac_open (struct net_device *dev)
static void emac_sl2312_start(void)
{

    /* allocates tx/rx descriptor and buffer memory */
    emac_init_desc_buf();

    /* set PHY register to start autonegition process */
    emac_set_phy_status();

	/* EMAC initialization */
	if ( emac_init_chip() ) 
	{
		diag_printf ("EMAC init fail\n");
	}	

    /* enable tx/rx register */    
    emac_enable_tx_rx();
    
    /* start DMA process */
	emac_hw_start();

    /* clear statistic counter */
    emac_clear_counter();
	
	return ;	
}


//static int emac_close(struct net_device *dev)
static void emac_sl2312_stop(void)
{
    
    /* stop tx/rx packet */
    emac_disable_tx_rx();

    /* stop the chip's Tx and Rx DMA processes */
	emac_hw_stop();          
}

int emac_sl2312_can_send(void)
{
	EMAC_INFO_T	*tp = (EMAC_INFO_T *)&emac_private_data;

    if (tp->tx_cur_desc->frame_ctrl.bits_tx.own == CPU)
    {
        return (1);
    }
    else
    {
    	tp->tx_no_resource++;
        return (0);
    }        
}

//static int emac_start_xmit(struct sk_buff *skb, struct net_device *dev)
void emac_sl2312_send(char *bufp, int total_len)
{
	EMAC_INFO_T     		*tp = (EMAC_INFO_T *)&emac_private_data;
	EMAC_TXDMA_CTRL_T		tx_ctrl,tx_ctrl_mask;
	EMAC_TXDMA_FIRST_DESC_T	txdma_busy;
	unsigned char           *tx_buf_adr;

	// Share Pin issue
#ifndef MIDWAY	
	REG32(SL2312_GLOBAL_BASE+GLOBAL_MISC_CTRL) |= GLOBAL_FLASH_EN_BIT ;
#endif    
    
    if ((tp->tx_cur_desc->frame_ctrl.bits_tx.own == CPU) && (total_len < TX_BUF_SIZE))
	{
        if (FLAG_SWITCH==1 && (chip_id<0xA3) && vlan_enabled)
        {
   		   	tp->tx_cur_desc->flag_status.bits_tx_flag.vlan_enable = 1;      /* enable vlan TIC insertion */
		   	tp->tx_cur_desc->flag_status.bits_tx_flag.vlan_id = 1;		/* 	1:To LAN . 2:To WAN */		
	    }
	    else
	    {
   		   	tp->tx_cur_desc->flag_status.bits_tx_flag.vlan_enable = 0;      /* disable vlan TIC insertion */
        }		        
		tx_buf_adr = (unsigned char *)(tp->tx_cur_desc->buf_adr);
		/* set TX descriptor */
		/* copy packet to descriptor buffer address */
		
		memcpy(tx_buf_adr, bufp, total_len);

		tp->tx_cur_desc->frame_ctrl.bits_tx.buffer_size = total_len;  /* descriptor byte count */
    	tp->tx_cur_desc->flag_status.bits_tx_flag.frame_count = total_len;    /* total frame byte count */
    	tp->tx_cur_desc->next_desc.bits.sof_eof = 0x03;                 /*only one descriptor*/
    	tp->tx_cur_desc->frame_ctrl.bits_tx.own = DMA;	                /* set owner bit */
    	tp->tx_cur_desc = (EMAC_DESCRIPTOR_T *)(tp->tx_cur_desc->next_desc.next_descriptor & 0xfffffff0);
		tp->tx_pkts++;
	} 

 	/* if TX DMA process is stoped , restart it */    
	txdma_busy.bits32 = emac_read_reg(EMAC_TXDMA_FIRST_DESC);
	if (txdma_busy.bits.td_busy == 0)
	{
		/* restart DMA process */
		tx_ctrl.bits32 = 0;
		tx_ctrl.bits.td_start = 1;
		tx_ctrl.bits.td_continue = 1;
		tx_ctrl_mask.bits32 = 0;
		tx_ctrl_mask.bits.td_start = 1;
		tx_ctrl_mask.bits.td_continue = 1;
		emac_write_reg(EMAC_TXDMA_CTRL,tx_ctrl.bits32,tx_ctrl_mask.bits32);
	}	
	
}

//static int __init emac_init_module(void)
int emac_sl2312_init(void)
{
	chip_id = REG32(SL2312_GLOBAL_BASE + GLOBAL_ID) & 0xff;
	
	FLAG_SWITCH = 0 ;
	FLAG_SWITCH = SPI_get_identifier();
	if(FLAG_SWITCH)
	{	
		diag_printf("\nConfig ADM699X...\n");
		SPI_default(vlan_enabled);	//Add by jason for ADM6996 config
	}
	
	emac_sl2312_start();
	
    return TRUE;
}	

/*----------------------------------------------------------------------
* emac_set_mac_addr
*	set mac address
*----------------------------------------------------------------------*/
void emac_set_mac_addr(unsigned char *mac1, unsigned char *mac2)
{
	UINT32 data;
	data = mac1[0] + (mac1[1]<<8) + (mac1[2]<<16) + (mac1[3]<<24);
	emac_write_reg(EMAC_STA_ADD0, data, 0xffffffff);
	data = mac1[4] + (mac1[5]<<8) + (mac2[0]<<16) + (mac2[1]<<24);
	emac_write_reg(EMAC_STA_ADD1, data, 0xffffffff);
	data = mac2[2] + (mac2[3]<<8) + (mac2[4]<<16) + (mac2[5]<<24);
	emac_write_reg(EMAC_STA_ADD2, data, 0xffffffff);
}

/*----------------------------------------------------------------------
* emac_reset_statistics
*----------------------------------------------------------------------*/
void emac_reset_statistics(void)
{
	EMAC_INFO_T *tp = (EMAC_INFO_T *)&emac_private_data;
	
#ifdef EMAC_STATISTICS
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
void emac_show_statistics(char argc, char *argv[])
{
	EMAC_INFO_T *tp = (EMAC_INFO_T *)&emac_private_data;
	EMAC_DESCRIPTOR_T *bd;
	int i, type, total;
#ifdef EMAC_STATISTICS        	    
	printf("EMAC Statistics:\n");
	printf("Rx packets    : %u\n", tp->rx_pkts);
	printf("Tx packets    : %u\n", tp->tx_pkts);
	printf("Rx Overrun    : %u\n", tp->rx_overrun);
	printf("Tx Underrun   : %u\n", tp->tx_underrun);
	printf("Tx no resource: %u\n", tp->tx_no_resource);
#endif

#if 0        	    
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

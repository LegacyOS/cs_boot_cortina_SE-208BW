/****************************************************************************
* Copyright  Storlink Corp 2005.  All rights reserved.                
*--------------------------------------------------------------------------
* Name			: emac_diag.c
* Description	: 
*		Ethernet Diagnostic 
*
* History
*
*	Date		Writer		Description
*	-----------	-----------	-------------------------------------------------
*	05/25/2005	Gary Chen	Implement by following the algorithm of Xi Chen
*
****************************************************************************/
#include <define.h>
#include <board_config.h>
#include <sl2312.h>

#ifndef MIDWAY
#include "emac_sl2312.h"

/*************************************************************
 *         Global Variable
 *************************************************************/
#undef EMAC_DIAG_ENABLE_FLOW_CTRL
// #define DIAG_BY_LA	1

#ifdef BOARD_DCACHE_ON
	typedef struct {
		unsigned char	tx_desc_array[TX_DESC_NUM * sizeof(EMAC_DESCRIPTOR_T)] __attribute__((aligned(32)));
		unsigned char	rx_desc_array[RX_DESC_NUM * sizeof(EMAC_DESCRIPTOR_T)]  __attribute__((aligned(32)));
		unsigned char	buffer[EMAC_BUF_SIZE] __attribute__((aligned(32)));
	} EMAC_NONCACHE_MEM_T;
	typedef struct {
		EMAC_NONCACHE_MEM_T	buf;
		unsigned char 		free[BOARD_DRAM_NONCACHE_SIZE - sizeof(EMAC_NONCACHE_MEM_T)];
	} EMAC_NONCACHE_AREA_T;

	EMAC_NONCACHE_MEM_T 	*emac_noncache_memp = (EMAC_NONCACHE_MEM_T *)BOARD_DRAM_NONCACHE_BASE;
	static unsigned char    *tx_desc_array;
	static unsigned char    *rx_desc_array;
	static unsigned char	*rx_buf_array;
#else
	static unsigned char    tx_desc_array[TX_DESC_NUM * sizeof(EMAC_DESCRIPTOR_T)] __attribute__((aligned(32))); 
	static unsigned char    rx_desc_array[RX_DESC_NUM * sizeof(EMAC_DESCRIPTOR_T)] __attribute__((aligned(32)));
	static unsigned char    rx_buf_array[EMAC_BUF_SIZE] __attribute__((aligned(32)));
#endif

extern EMAC_INFO_T emac_private_data;
extern unsigned int full_duplex;
extern unsigned int flow_control_enable;
int emac_bridge_diag_mode;

static EMAC_DESCRIPTOR_T null_rxbd;

#define tp	emac_private_data


/************************************************/
/*                 function declare             */
/************************************************/
#define emac_read_reg(offset)					REG32(EMAC_BASE_ADDR + offset)
#define emac_write_reg(offset, data, mask)		REG32(EMAC_BASE_ADDR + offset) = 	\
												(emac_read_reg(offset) & (~mask)) | \
												(data & mask)
static void emac_diag_start(void);
static void emac_diag_stop(void);
static void emac_diag_hw_stop(void);
static void emac_diag_hw_start(void);

extern void emac_enable_tx_rx(void);
extern void emac_disable_tx_rx(void);
static int emac_diag_init_chip(void);
static void emac_diag_init_desc_buf(void);
static void emac_diag_isr(void);

static void emac_diag_rx_isr_begin(EMAC_INFO_T *tpp);
static void emac_diag_tx_isr_begin(EMAC_INFO_T *tpp);

extern int emac_clear_counter (void);
extern void emac_set_phy_status(void);
extern void emac_get_phy_status(void);
extern char *sys_get_mac_addr(int index);

/************************************************/
/*                 function body                */
/************************************************/

/*----------------------------------------------------------------------
* emac_set_xi_diag_mode
*----------------------------------------------------------------------*/
void emac_set_xi_diag_mode(int mode)
{
    unsigned long old_ints;
	char *status_msg[2]={"disabled", "enabled"};
	register EMAC_INFO_T *tpp;
		
	tpp = (EMAC_INFO_T *)&emac_private_data;
	
	if (mode)
	{
		
		REG32(SL2312_GLOBAL_BASE + GLOBAL_RESET) = 0x20;	// Reset EMAC
		emac_diag_start();
		emac_bridge_diag_mode = 1;
		
    	emac_write_reg(EMAC_RX_FLTR,0x0000001f,0x0000001f);
#ifdef BOARD_DCACHE_ON 
		if (!hal_get_dcache_status())
		{
    		HAL_DISABLE_INTERRUPTS(old_ints);
			HAL_DCACHE_INVALIDATE_ALL();
    		HAL_DCACHE_ENABLE();
			HAL_RESTORE_INTERRUPTS(old_ints);
		}
#endif    	

		printf("MMU: %s, I-CACHE: %s, D-CACHE: %s\n",
				status_msg[hal_get_mmu_status()],
				status_msg[hal_get_icache_status()],
				status_msg[hal_get_dcache_status()]);
			
		emac_reset_statistics();
#ifndef MIDWAY		
		REG32(SL2312_GLOBAL_BASE+GLOBAL_MISC_CTRL) |= GLOBAL_FLASH_EN_BIT; //0x00000001 ;
#endif
		hal_register_irq_entry((void *)emac_diag_isr);
		hal_interrupt_unmask(SL2312_INTERRUPT_EMAC); // should be last
    }
	else    	
	{
		hal_interrupt_mask(SL2312_INTERRUPT_EMAC);  // should be first
		hal_interrupt_acknowledge(SL2312_INTERRUPT_EMAC);  // should be first
		hal_register_irq_entry(NULL);
		emac_bridge_diag_mode = 0;

#ifdef BOARD_DCACHE_ON
		if (hal_get_dcache_status())
		{
    		HAL_DISABLE_INTERRUPTS(old_ints);
			HAL_ICACHE_INVALIDATE_ALL();
			HAL_DCACHE_INVALIDATE_ALL();
    		HAL_DCACHE_DISABLE();
    	}
#endif    	
    	emac_write_reg(EMAC_RX_FLTR,0x00000005,0x0000001f);
		hal_interrupt_acknowledge(SL2312_INTERRUPT_TIMER1);  // should be first
    	hal_interrupt_unmask(SL2312_INTERRUPT_TIMER1);
		HAL_ENABLE_INTERRUPTS();
    }
}


/*----------------------------------------------------------------------
* emac_diag_start
*----------------------------------------------------------------------*/
static void emac_diag_start(void)
{

    /* allocates tx/rx descriptor and buffer memory */
    emac_diag_init_desc_buf();

    /* set PHY register to start autonegition process */
    emac_set_phy_status();

	/* EMAC initialization */
	if ( emac_diag_init_chip() ) 
	{
		printf ("EMAC init fail\n");
	}	

    /* enable tx/rx register */    
    emac_enable_tx_rx();
    
    /* start DMA process */
	emac_diag_hw_start();

    /* clear statistic counter */
    emac_clear_counter();
	
	return ;
}

/*----------------------------------------------------------------------
* emac_diag_stop
*----------------------------------------------------------------------*/
static void emac_diag_stop(void)
{
    /* stop tx/rx packet */
    emac_disable_tx_rx();

    /* stop the chip's Tx and Rx DMA processes */
	emac_diag_hw_stop();          
}

/*----------------------------------------------------------------------
* emac_diag_init_chip
*----------------------------------------------------------------------*/
static int emac_diag_init_chip(void)
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
#ifdef EMAC_DIAG_ENABLE_FLOW_CTRL
    config0.bits.fc_en = 1; /* enable flow control */
#else
    config0.bits.fc_en = 0; /* disable flow control */
#endif    
    config0.bits.dis_rx = 1;  /* disable rx */
    config0.bits.dis_tx = 1;  /* disable tx */
    config0.bits.loop_back = 0; /* enable/disable EMAC loopback */
    config0_mask.bits.max_len = 3;
    config0_mask.bits.fc_en = 1;
    config0_mask.bits.dis_rx = 1;
    config0_mask.bits.dis_tx = 1;
    config0_mask.bits.loop_back = 1;
    config0.bits.adj_ifg = 7;
    config0_mask.bits.adj_ifg = 0x0f;
    emac_write_reg(EMAC_CONFIG0,config0.bits32,config0_mask.bits32);
    
#ifdef EMAC_DIAG_ENABLE_FLOW_CTRL
	flow_control_enable = 1; /* enable flow control flag */
#else
	flow_control_enable = 0; /* disable flow control flag */
#endif	
	return (0);
}

/*----------------------------------------------------------------------
* emac_diag_init_desc_buf
*----------------------------------------------------------------------*/
static void emac_diag_init_desc_buf(void)
{
	int    i;
	EMAC_DESCRIPTOR_T *desc;
	unsigned char *bufp;

#ifdef BOARD_DCACHE_ON
	tx_desc_array = (unsigned char *)&emac_noncache_memp->tx_desc_array[0];
	rx_desc_array = (unsigned char *)&emac_noncache_memp->rx_desc_array[0];
	rx_buf_array  = (unsigned char *)&emac_noncache_memp->buffer[0];
#endif

    tp.tx_desc = (EMAC_DESCRIPTOR_T *)tx_desc_array;
    tp.rx_desc = (EMAC_DESCRIPTOR_T *)rx_desc_array;
    tp.bufs = rx_buf_array;

    /* TX descriptors initial */
    tp.tx_head = 0;
    tp.tx_tail = 0;
    
	/* TX descriptors initial */
	desc = tp.tx_desc;
	for (i = 0; i < TX_DESC_NUM; i++)
	{
		memset((char *)desc, 0, sizeof(*desc));
		desc->frame_ctrl.bits_tx.own = CPU; /* set owner to CPU */
		desc->frame_ctrl.bits_tx.buffer_size = 0;  /* set tx buffer size for descriptor */
		desc->buf_adr = 0;
		desc->next_desc.next_descriptor = (unsigned long)(desc + 1) | 0x0000000b;
		desc++;
	}
	desc--;
	desc->next_desc.next_descriptor = (unsigned long)tp.tx_desc | 0x0000000b;
	
	/* RX descriptors initial */
    tp.rx_head = 0;
    tp.rx_tail = RX_DESC_NUM-1;
	desc = tp.rx_desc;
	for (i = 0; i < RX_DESC_NUM; i++)
	{
		memset((char *)desc, 0, sizeof(*desc));
		desc->frame_ctrl.bits_rx.own = DMA;  /* set owner bit to DMA */
		desc->frame_ctrl.bits_rx.buffer_size = RX_BUF_SIZE; /* set rx buffer size for descriptor */
		desc->buf_adr = (unsigned int)tp.bufs+i*RX_BUF_SIZE;
		memset(desc->buf_adr, 0, RX_BUF_SIZE);
		desc->next_desc.next_descriptor = (unsigned long)(desc + 1) | 0x0000000b;
		desc++;
	}
	desc--;
	memset((char *)&null_rxbd, 0, sizeof(EMAC_DESCRIPTOR_T));
	// desc->next_desc.next_descriptor =  (unsigned int)&null_rxbd | 0x0000000b;
	desc->next_desc.next_descriptor =  0x0000000b;
}    

/*----------------------------------------------------------------------
* emac_diag_hw_start
*----------------------------------------------------------------------*/
void emac_diag_hw_start(void)
{
	EMAC_TXDMA_CURR_DESC_T	tx_desc;
	EMAC_RXDMA_CURR_DESC_T	rx_desc;
    EMAC_TXDMA_CTRL_T       txdma_ctrl,txdma_ctrl_mask;
    EMAC_RXDMA_CTRL_T       rxdma_ctrl,rxdma_ctrl_mask;
	EMAC_DMA_STATUS_T       dma_status,dma_status_mask;
	
	/* program TxDMA Current Descriptor Address register for first descriptor */
	tx_desc.bits32 = (unsigned int)(tp.tx_desc);
	tx_desc.bits.eofie = 1;
	tx_desc.bits.dec = 0;
	tx_desc.bits.sof_eof = 0x03;
	emac_write_reg(EMAC_TXDMA_CURR_DESC,tx_desc.bits32,0xffffffff);
	emac_write_reg(0xff2c,tx_desc.bits32,0xffffffff);   /* tx next descriptor address */
	
	/* program RxDMA Current Descriptor Address register for first descriptor */
	rx_desc.bits32 = (unsigned int)(tp.rx_desc);
	rx_desc.bits.eofie = 1;
	rx_desc.bits.dec = 0;
	rx_desc.bits.sof_eof = 0x03;
	emac_write_reg(EMAC_RXDMA_CURR_DESC,rx_desc.bits32,0xffffffff);
	emac_write_reg(0xff3c,rx_desc.bits32,0xffffffff);   /* rx next descriptor address */
	    	
	/* enable EMAC interrupt & disable loopback */
	dma_status.bits32 = 0;
	dma_status.bits.loop_back = 0;  /* disable DMA loop-back mode */
	dma_status.bits.m_tx_fail = 1;
	dma_status.bits.m_cnt_full = 0;
#ifdef EMAC_DIAG_ENABLE_FLOW_CTRL
	dma_status.bits.m_rx_pause_on = 1;
	dma_status.bits.m_tx_pause_on = 1;
	dma_status.bits.m_rx_pause_off = 1;
	dma_status.bits.m_tx_pause_off = 1;
#endif	
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

/*----------------------------------------------------------------------
* emac_diag_hw_stop
*----------------------------------------------------------------------*/
void emac_diag_hw_stop(void)
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

/*----------------------------------------------------------------------
* emac_diag_rx_isr_begin
*----------------------------------------------------------------------*/
static void emac_diag_rx_isr_begin(EMAC_INFO_T *tpp)
{
	int                    tx_tmptail;
	int					  total=0;	// gary
  EMAC_TXDMA_FIRST_DESC_T txdma_busy;
  EMAC_TXDMA_CTRL_T       tx_ctrl,tx_ctrl_mask;
#ifdef EMAC_DIAG_ENABLE_FLOW_CTRL	
	unsigned int	desc_count; 
#endif
	BD_FRAME_CTRL_T	reg_desc_frame_ctrl;
	unsigned int		reg_desc_buf_adr;
	BD_FLAG_STATUS_T	reg_desc_flag_status;
	BD_NEXT_DESC_T	reg_desc_next_desc;
  
#ifdef	DIAG_BY_LA
	{
		unsigned long data;
		data = REG32(SL2312_FLASH_SHADOW);
	}
#endif

  tx_tmptail = tpp->tx_tail;
  // copy rx_desc[tpp->rx_head] to reg_desc
  reg_desc_frame_ctrl.bits32  
    = tpp->rx_desc[tpp->rx_head].frame_ctrl.bits32 ;
  while ( reg_desc_frame_ctrl.bits_rx.own == CPU && tpp->rx_head != -1)
  {
  	reg_desc_flag_status.bits32  
    	= tpp->rx_desc[tpp->rx_head].flag_status.bits32 ;
  	reg_desc_buf_adr  
    	= tpp->rx_desc[tpp->rx_head].buf_adr ;
  	reg_desc_next_desc.next_descriptor  
    	= tpp->rx_desc[tpp->rx_head].next_desc.next_descriptor;

  	total++;
#ifdef EMAC_STATISTICS		
  	tpp->rx_pkts++;
#endif
#if 0   
   	if (reg_desc_flag_status.bits32 == 0)
   	{
   		tpp->tx_err++;
   		printf("err: reg_desc_frame_ctrl=0x%x, reg_desc_flag_status=0x%x, tpp->tx_desc[tx_tmptail].frame_ctrl.bits32=0x%x\n", 
   				reg_desc_frame_ctrl.bits32, reg_desc_flag_status.bits32, tpp->tx_desc[tx_tmptail].frame_ctrl.bits32);
   	}
#endif   	
    // reg_desc_frame_ctrl.bits32 &= 0x0000ffff;
    if ( tx_tmptail != tpp->tx_tail )
      reg_desc_frame_ctrl.bits32 = 0x80000000 + reg_desc_flag_status.bits_rx_status.frame_count-4;		// DMA bit
    else
      reg_desc_frame_ctrl.bits32 = reg_desc_flag_status.bits_rx_status.frame_count-4;		// DMA bit
    
	// setup tx descriptor and rd_index
    tpp->tx_desc[tx_tmptail].frame_ctrl.bits32 						// Gary Mask 
      = reg_desc_frame_ctrl.bits32;								// Gary Mask 
    tpp->tx_desc[tx_tmptail].flag_status.bits32  					// Gary Mask 
      = reg_desc_flag_status.bits_rx_status.frame_count-4;
    // tpp->tx_desc[tx_tmptail].frame_ctrl.bits_tx.buffer_size =			// Gary Add
    //tpp->tx_desc[tx_tmptail].flag_status.bits_tx_flag.frame_count		// Gary Add
    //  = reg_desc_flag_status.bits_rx_status.frame_count-4;			// Gary Add
    tpp->tx_desc[tx_tmptail].buf_adr  
      = reg_desc_buf_adr ;

#if 0   
   	if (tpp->tx_desc[tx_tmptail].frame_ctrl.bits32 == 0x7ffffffc)
   	{
   		tpp->tx_err++;
   		printf("err: reg_desc_frame_ctrl=0x%x, reg_desc_flag_status=0x%x, tpp->tx_desc[tx_tmptail].frame_ctrl.bits32=0x%x\n", 
   				reg_desc_frame_ctrl.bits32, reg_desc_flag_status.bits32, tpp->tx_desc[tx_tmptail].frame_ctrl.bits32);
   	}
#endif   	
    tpp->rd_index[tx_tmptail] = tpp->rx_head;

#ifdef EMAC_DIAG_ENABLE_FLOW_CTRL
	desc_count = reg_desc_frame_ctrl.bits_rx.desc_count; /* get descriptor count per frame */
	emac_write_reg(EMAC_BNCR, desc_count, 0x0000ffff);
#endif

	// advance tx_tmptail
    tx_tmptail = (tx_tmptail+1)%TX_DESC_NUM;
	// advance rx_head
	if ((reg_desc_next_desc.next_descriptor & ~0x0f))
    	tpp->rx_head = ((unsigned int)(reg_desc_next_desc.next_descriptor & 0xfffffff0)
                 - (unsigned int)tpp->rx_desc)
                 / sizeof(EMAC_DESCRIPTOR_T);
    else
    {
    	tpp->rx_head = -1;
    	break;
    }
    	

	// read tpp->rx_desc[tpp->rx_head] to reg_desc
    //reg_desc_frame_ctrl.bits32						// Gary mask
    //  = tpp->rx_desc[tpp->rx_head].frame_ctrl.bits32 ;	// Gary mask
    reg_desc_frame_ctrl.bits32							
      = tpp->rx_desc[tpp->rx_head].frame_ctrl.bits32 ;		
  }
  		
#ifdef	DIAG_BY_LA
	{
		unsigned long data;
		data = REG32(SL2312_FLASH_SHADOW);
	}
#endif

  // if ( tx_tmptail != tpp->tx_tail )
  if ( total || (tpp->rx_head == -1))
  {  // packet need to transmitted.
  	// REG8(tpp->tx_desc[tpp->tx_tail].buf_adr) = 0x22; /* Gary testing */
    tpp->tx_desc[tpp->tx_tail].frame_ctrl.bits_tx.own = DMA;
    tpp->tx_tail = tx_tmptail;								// Gary add
    txdma_busy.bits32 = emac_read_reg(EMAC_TXDMA_FIRST_DESC);
    if (txdma_busy.bits.td_busy == 0)
    {
	  emac_write_reg(EMAC_TXDMA_CURR_DESC,(unsigned int)(&tpp->tx_desc[tpp->tx_head]) | 0x0b,0xffffffff);
	  emac_write_reg(0xff2c,(unsigned int)(&tpp->tx_desc[tpp->tx_head]) | 0x0b,0xffffffff);   /* rx next descriptor address */
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
}

/*----------------------------------------------------------------------
* emac_diag_tx_isr_begin
*----------------------------------------------------------------------*/
static void emac_diag_tx_isr_begin(EMAC_INFO_T *tpp)
{
  // register EMAC_DESCRIPTOR_T       reg_desc;
	int                     rx_tmphead, rx_tmptail;
	int                     reg_rxdesc_index;
  EMAC_RXDMA_FIRST_DESC_T rxdma_busy;
  EMAC_RXDMA_CTRL_T       rxdma_ctrl,rxdma_ctrl_mask;
	BD_FRAME_CTRL_T	reg_desc_frame_ctrl;
	unsigned int		reg_desc_buf_adr;
	BD_FLAG_STATUS_T	reg_desc_flag_status;
	BD_NEXT_DESC_T	reg_desc_next_desc;

  rx_tmphead = -1;
  rx_tmptail = -1;
  // copy tx_desc[tp.tx_head] to reg_desc
  reg_desc_frame_ctrl.bits32  
    = tpp->tx_desc[tpp->tx_head].frame_ctrl.bits32 ;
  reg_desc_flag_status.bits32  
    = tpp->tx_desc[tpp->tx_head].flag_status.bits32 ;
  reg_desc_buf_adr  
    = tpp->tx_desc[tpp->tx_head].buf_adr ;
  reg_desc_next_desc.next_descriptor  
    = tpp->tx_desc[tpp->tx_head].next_desc.next_descriptor;
  reg_rxdesc_index = tpp->rd_index[tpp->tx_head];

  // while (reg_desc_frame_ctrl.bits_tx.own == CPU )	// Gary Mask
  // while (reg_desc_frame_ctrl.bits_tx.own == CPU && reg_desc_frame_ctrl.bits_tx.success_tx)	// Gary change
  while (reg_desc_frame_ctrl.bits_tx.own == CPU && tpp->tx_head != tpp->tx_tail)
  {
  	// tpp->tx_desc[tpp->tx_head].frame_ctrl.bits_tx.success_tx = 0;
#ifdef EMAC_STATISTICS		
     tpp->tx_pkts++;
#endif 	
	if ( rx_tmphead == -1 )
	  rx_tmphead = reg_rxdesc_index;
	if ( rx_tmptail != -1 )
      tpp->rx_desc[rx_tmptail].next_desc.next_descriptor
		= (unsigned int)&tpp->rx_desc[reg_rxdesc_index] | 0x0000000b;

	rx_tmptail = reg_rxdesc_index;
    tpp->tx_head = (tpp->tx_head + 1) % TX_DESC_NUM;

	// Init tpp->rx_desc[reg_rx_desc_index];
    reg_desc_buf_adr            = (unsigned int)tpp->bufs + reg_rxdesc_index * RX_BUF_SIZE;
    reg_desc_frame_ctrl.bits32  = 0x80000000 | RX_BUF_SIZE;		// GARYCHEN
    tpp->rx_desc[reg_rxdesc_index].flag_status.bits32  
      = 0x00000000;
    tpp->rx_desc[reg_rxdesc_index].buf_adr
	  = reg_desc_buf_adr;
    tpp->rx_desc[reg_rxdesc_index].frame_ctrl.bits32  
      = reg_desc_frame_ctrl.bits32;

	// copy tpp->tx_desc[tpp->tx_head] to reg_desc_
    // reg_desc_frame_ctrl.bits32  						// Gary mask
    //   = tpp->tx_desc[tpp->tx_head].frame_ctrl.bits32 ;	// Gary mask
    reg_desc_flag_status.bits32  
      = tpp->tx_desc[tpp->tx_head].flag_status.bits32 ;
    reg_desc_buf_adr  
      = tpp->tx_desc[tpp->tx_head].buf_adr ;
    reg_desc_next_desc.next_descriptor  
      = tpp->tx_desc[tpp->tx_head].next_desc.next_descriptor;
    reg_rxdesc_index = tpp->rd_index[tpp->tx_head];
    reg_desc_frame_ctrl.bits32  						// Gary add
       = tpp->tx_desc[tpp->tx_head].frame_ctrl.bits32 ;		// Gary add
  }
	if ( rx_tmptail != -1 )
		tpp->rx_desc[rx_tmptail].next_desc.next_descriptor = 0 | 0x0000000b;

  if ( rx_tmphead != -1 )
  {
	if (tpp->rx_head != -1)
	{
    	tpp->rx_desc[tpp->rx_tail].next_desc.next_descriptor
	  		= (unsigned int)&tpp->rx_desc[rx_tmphead] | 0x0000000b;
		tpp->rx_tail = rx_tmptail;
	}
	else
	{
		tpp->rx_head = rx_tmphead;
		tpp->rx_tail = rx_tmptail;
	}

    rxdma_busy.bits32 = emac_read_reg(EMAC_RXDMA_FIRST_DESC) ;
    if (rxdma_busy.bits.rd_busy == 0)
    {
      /* restart Rx DMA process */
	  emac_write_reg(EMAC_RXDMA_CURR_DESC,(unsigned int)(&tpp->rx_desc[tpp->rx_head]) | 0x0b,0xffffffff);
	  emac_write_reg(0xff3c,(unsigned int)(&tpp->rx_desc[tpp->rx_head]) | 0x0b,0xffffffff);   /* rx next descriptor address */
      rxdma_ctrl.bits32 = 0;
      rxdma_ctrl.bits.rd_start = 1;    /* start RX DMA transfer */
      rxdma_ctrl.bits.rd_continue = 1; /* continue RX DMA operation */
      rxdma_ctrl_mask.bits32 = 0;
      rxdma_ctrl_mask.bits.rd_start = 1;   
      rxdma_ctrl_mask.bits.rd_continue = 1; 
      emac_write_reg(EMAC_RXDMA_CTRL,rxdma_ctrl.bits32,rxdma_ctrl_mask.bits32);
    }
  }
}

/*----------------------------------------------------------------------
* emac_diag_isr
*----------------------------------------------------------------------*/
static void emac_diag_isr(void)
{
	register unsigned long status;
	register EMAC_INFO_T *tpp;
		
	sl2312_eth_disable_interrupt();
		
	tpp = (EMAC_INFO_T *)&emac_private_data;
#ifdef EMAC_STATISTICS		
	tpp->interrupt_cnt++;
#endif	
	status = emac_read_reg(EMAC_DMA_STATUS);

	/* clear DMA status */
	emac_write_reg(EMAC_DMA_STATUS, status, status);	
        
	if (status & 0xc000)	// rx_overrun and tx_underrun bits
	{
		if (status & 0x8000)	// rx_overrun
		{
			EMAC_RXDMA_FIRST_DESC_T	rxdma_busy;
		    /* if RX DMA process is stoped , restart it */    
		    rxdma_busy.bits32 = emac_read_reg(EMAC_RXDMA_FIRST_DESC) ;
		    if (rxdma_busy.bits.rd_busy == 0)
		    {
		    	register EMAC_RXDMA_CTRL_T rxdma_ctrl,rxdma_ctrl_mask;
		    	
		        /* restart Rx DMA process */
		        if (tpp->rx_head != -1)
		        {
	  				emac_write_reg(EMAC_RXDMA_CURR_DESC,(unsigned int)(&tpp->rx_desc[tpp->rx_head]) | 0x0b,0xffffffff);
	  		    	emac_write_reg(0xff3c,(unsigned int)(&tpp->rx_desc[tpp->rx_head]) | 0x0b,0xffffffff);   /* rx next descriptor address */
	  		    }
		        rxdma_ctrl.bits32 = 0;
		        rxdma_ctrl.bits.rd_start = 1;    /* start RX DMA transfer */
		        rxdma_ctrl.bits.rd_continue = 1; /* continue RX DMA operation */
		        rxdma_ctrl_mask.bits32 = 0;
		        rxdma_ctrl_mask.bits.rd_start = 1;   
		        rxdma_ctrl_mask.bits.rd_continue = 1; 
		        emac_write_reg(EMAC_RXDMA_CTRL,rxdma_ctrl.bits32,rxdma_ctrl_mask.bits32);
		    }
#ifdef EMAC_STATISTICS		
		    tpp->rx_overrun++;
#endif		    
		}
		if (status & 0x4000)	// tx_underrun
		{
			EMAC_TXDMA_FIRST_DESC_T	txdma_busy;
#ifdef EMAC_STATISTICS		
		    tpp->tx_underrun++;
#endif
		    /* if TX DMA process is stoped , restart it */    
		    txdma_busy.bits32 = emac_read_reg(EMAC_TXDMA_FIRST_DESC);
		    if (txdma_busy.bits.td_busy == 0)
		    {
		    	register EMAC_TXDMA_CTRL_T txdma_ctrl,txdma_ctrl_mask;
		    	
		        /* restart Tx DMA process */
	  			emac_write_reg(EMAC_TXDMA_CURR_DESC,(unsigned int)(&tpp->tx_desc[tpp->tx_head]) | 0x0b,0xffffffff);
	  			emac_write_reg(0xff2c,(unsigned int)(&tpp->tx_desc[tpp->tx_head]) | 0x0b,0xffffffff);   /* rx next descriptor address */
		        txdma_ctrl.bits32 = 0;
		        txdma_ctrl.bits.td_start = 1;
		        txdma_ctrl.bits.td_continue = 1;
		        txdma_ctrl_mask.bits32 = 0;
		        txdma_ctrl_mask.bits.td_start = 1;
		        txdma_ctrl_mask.bits.td_continue = 1;
		        emac_write_reg(EMAC_TXDMA_CTRL,txdma_ctrl.bits32,txdma_ctrl_mask.bits32);
		    }
		}
	}
	else
	{	    
		/* receive rx interrupt */
    	if (status & 0x04C00000)	// rs_eofi, rs_eodi, and rs_finish
    	{
#ifdef EMAC_STATISTICS		
    		tpp->rx_intr_cnt++;
#endif
			emac_diag_rx_isr_begin(tpp);
		}
	        
		/* receive tx interrupt */
		if (status & 0x98000000)	// ts_eofi, ts_finish, and ts_eodi bits
		{
#ifdef EMAC_STATISTICS		
    		tpp->tx_intr_cnt++;
#endif
			emac_diag_tx_isr_begin(tpp);
		}
	}
    	
	sl2312_eth_enable_interrupt();
	return;
}

#endif // MIDWAY

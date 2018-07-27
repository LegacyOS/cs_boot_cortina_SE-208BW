/****************************************************************************
* Copyright  Storlink Corp 2005.  All rights reserved.                
*--------------------------------------------------------------------------
* Name			: emac_sl2312.h
* Description	: 
*		Define for device driver of Storlink SL2312 Chip
*
* History
*
*	Date		Writer		Description
*	-----------	-----------	-------------------------------------------------
*	04/25/2005	Gary Chen	Create and implement from Jason's Redboot code
*
****************************************************************************/
#ifndef _EMAC_SL2312_H
#define _EMAC_SL2312_H

#define EMAC_STATISTICS		1

typedef unsigned int		dma_addr_t;
#define ETHER_ADDR_LEN		6
#define BIG_ENDIAN  		0

/* define chip information */
#define DRV_NAME			"SL2312"
#define DRV_VERSION			"0.1.1"
#define SL2312_DRIVER_NAME  DRV_NAME " Fast Ethernet driver " DRV_VERSION

#define TX_DESC_NUM			16
#define RX_DESC_NUM			16
#define RX_BUF_NUM			(RX_DESC_NUM * 1)

#define MAX_ETH_FRAME_SIZE	1536

#undef TX_DESC_NUM
#undef RX_DESC_NUM
#define BUF_COUNT					512
#define TX_DESC_NUM           		(BUF_COUNT)
#define RX_DESC_NUM					(BUF_COUNT)
#define EMAC_BUF_SIZE				(RX_DESC_NUM * MAX_ETH_FRAME_SIZE)


/* define TX/RX descriptor parameter */
#define TX_BUF_SIZE			MAX_ETH_FRAME_SIZE
#define TX_BUF_TOT_LEN		(TX_BUF_SIZE * TX_DESC_NUM)
#define RX_BUF_SIZE			MAX_ETH_FRAME_SIZE
#define RX_BUF_TOT_LEN		(RX_BUF_SIZE * RX_BUF_NUM)

/* define EMAC base address */
#define EMAC_BASE_ADDR		(SL2312_EMAC_BASE)

/* define owner bit */
#define CPU					0
#define DMA					1

/* define PHY address */
#define PHY_ADDR    		0x01

#define ADM_EECS			0x01
#define ADM_EECK			0x02
#define ADM_EDIO			0x04

#define LDN_GPIO			0x07
#define LPC_BASE_ADDR		SL2312_LPC_IO_BASE
#define IT8712_GPIO_BASE	0x800	// 0x800-0x804 for GPIO set1-set5
#define BASE_OFF			0x03

/***************************************/
/* the offset address of EMAC register */
/***************************************/
enum EMAC_REGISTER {
	EMAC_STA_ADD0 	= 0x0000,
	EMAC_STA_ADD1	= 0x0004,
	EMAC_STA_ADD2	= 0x0008,
	EMAC_RX_FLTR	= 0x000c,
	EMAC_MCAST_FIL0 = 0x0010,
	EMAC_MCAST_FIL1 = 0x0014,
	EMAC_CONFIG0	= 0x0018,
	EMAC_CONFIG1	= 0x001c,
	EMAC_CONFIG2	= 0x0020,
	EMAC_BNCR		= 0x0024,
	EMAC_RBNR		= 0x0028,
	EMAC_STATUS		= 0x002c,
	EMAC_IN_DISCARDS= 0x0030,
	EMAC_IN_ERRORS  = 0x0034
};		

/*******************************************/
/* the offset address of EMAC DMA register */
/*******************************************/
enum EMAC_DMA_REGISTER {
	EMAC_DMA_DEVICE_ID		= 0xff00,
	EMAC_DMA_STATUS			= 0xff04,
	EMAC_TXDMA_CTRL 	 	= 0xff08,
	EMAC_TXDMA_FIRST_DESC 	= 0xff0c,
	EMAC_TXDMA_CURR_DESC	= 0xff10,
	EMAC_RXDMA_CTRL			= 0xff14,
	EMAC_RXDMA_FIRST_DESC	= 0xff18,
	EMAC_RXDMA_CURR_DESC	= 0xff1c,
};

/*******************************************/
/* the register structure of EMAC          */
/*******************************************/
typedef union
{
	unsigned int bits32;
	struct bit1_0004
	{
#if (BIG_ENDIAN==1) 	    
		unsigned int sta_add2_l16	: 16;	/* station MAC address2 bits 15 to 0 */
		unsigned int sta_add1_h16	: 16;	/* station MAC address1 bits 47 to 32 */
#else
		unsigned int sta_add1_h16	: 16;	/* station MAC address1 bits 47 to 32 */
		unsigned int sta_add2_l16	: 16;	/* station MAC address2 bits 15 to 0 */
#endif		
	} bits;
} EMAC_STA_ADD1_T;		

typedef union
{
	unsigned int bits32;
	struct bit1_000c
	{
#if (BIG_ENDIAN==1) 	    
		unsigned int 				: 27;
		unsigned int error			:  1;	/* enable receive of all error frames */
		unsigned int promiscuous	:  1;   /* enable receive of all frames */
		unsigned int broadcast		:  1;	/* enable receive of broadcast frames */
		unsigned int multicast		:  1;	/* enable receive of multicast frames that pass multicast filter */
		unsigned int unicast		:  1;	/* enable receive of unicast frames that are sent to STA address */
#else
		unsigned int unicast		:  1;	/* enable receive of unicast frames that are sent to STA address */
		unsigned int multicast		:  1;	/* enable receive of multicast frames that pass multicast filter */
		unsigned int broadcast		:  1;	/* enable receive of broadcast frames */
		unsigned int promiscuous	:  1;   /* enable receive of all frames */
		unsigned int error			:  1;	/* enable receive of all error frames */
		unsigned int 				: 27;
#endif
	} bits;
} EMAC_RX_FLTR_T;

typedef union
{
	unsigned int bits32;
	struct bit1_0018
	{
#if (BIG_ENDIAN==1) 	    
		unsigned int 				: 19;
		unsigned int sim_test		:  1;	/* speed up timers in simulation */
		unsigned int dis_col		:  1;	/* disable 16 collisions abort function */
		unsigned int dis_bkoff		:  1;	/* disable back-off function */
		unsigned int max_len		:  2;	/* maximum receive frame length allowed */
		unsigned int adj_ifg		:  4;	/* adjust IFG from 96+/-56 */
		unsigned int fc_en			:  1;	/* flow control enable */
		unsigned int loop_back		:  1;	/* transmit data loopback enable */
		unsigned int dis_rx			:  1;	/* disable receive */
		unsigned int dis_tx			:  1;	/* disable transmit */
#else
		unsigned int dis_tx			:  1;	/* disable transmit */
		unsigned int dis_rx			:  1;	/* disable receive */
		unsigned int loop_back		:  1;	/* transmit data loopback enable */
		unsigned int fc_en			:  1;	/* flow control enable */
		unsigned int adj_ifg		:  4;	/* adjust IFG from 96+/-56 */
		unsigned int max_len		:  2;	/* maximum receive frame length allowed */
		unsigned int dis_bkoff		:  1;	/* disable back-off function */
		unsigned int dis_col		:  1;	/* disable 16 collisions abort function */
		unsigned int sim_test		:  1;	/* speed up timers in simulation */
		unsigned int 				: 19;
#endif
	} bits;
} EMAC_CONFIG0_T;		

typedef union
{
	unsigned int bits32;
	struct bit1_001c
	{
#if (BIG_ENDIAN==1) 	    
		unsigned int 				: 28;
		unsigned int buf_size		:  4; 	/* per packet buffer size */
#else
		unsigned int buf_size		:  4; 	/* per packet buffer size */
		unsigned int 				: 28;
#endif
	} bits;
} EMAC_CONFIG1_T;

typedef union
{
	unsigned int bits32;
	struct bit1_0020
	{
#if (BIG_ENDIAN==1) 	    
		unsigned int rel_threshold	: 16;	/* flow control release threshold */
		unsigned int set_threshold	: 16; 	/* flow control set threshold */
#else
		unsigned int set_threshold	: 16; 	/* flow control set threshold */
		unsigned int rel_threshold	: 16;	/* flow control release threshold */
#endif
	} bits;
} EMAC_CONFIG2_T;

typedef union
{
	unsigned int bits32;
	struct bit1_0024
	{
#if (BIG_ENDIAN==1) 	    
		unsigned int 				: 16;
		unsigned int buf_num		: 16; 	/* return buffer number from software */
#else
		unsigned int buf_num		: 16; 	/* return buffer number from software */
		unsigned int 				: 16;
#endif
	} bits;
} EMAC_BNCR_T;

typedef union
{
	unsigned int bits32;
	struct bit1_0028
	{
#if (BIG_ENDIAN==1) 	    
		unsigned int				: 16;
		unsigned int buf_remain		: 16;	/* remaining buffer number */
#else
		unsigned int buf_remain		: 16;	/* remaining buffer number */
		unsigned int				: 16;
#endif
	} bits;
} EMAC_RBNR_T;

typedef union
{
	unsigned int bits32;
	struct bit1_002c
	{
#if (BIG_ENDIAN==1) 	    
		unsigned int 				: 27;
		unsigned int phy_mode		:  1;	/* PHY interface mode in 10M-bps */
		unsigned int mii_rmii		:  1;   /* PHY interface type */
		unsigned int duplex			:  1;	/* duplex mode */
		unsigned int speed			:  1;	/* link speed */
		unsigned int link			:  1;	/* link status */
#else
		unsigned int link			:  1;	/* link status */
		unsigned int speed			:  1;	/* link speed */
		unsigned int duplex			:  1;	/* duplex mode */
		unsigned int mii_rmii		:  1;   /* PHY interface type */
		unsigned int phy_mode		:  1;	/* PHY interface mode in 10M-bps */
		unsigned int 				: 27;
#endif
	} bits;
} EMAC_STATUS_T;

		
typedef union
{
	unsigned int bits32;
	struct bit1_009
	{
#if (BIG_ENDIAN==1) 	    
		unsigned int 				: 10;
		unsigned int tx_fail		:  1; 	/* Tx fail interrupt */
		unsigned int cnt_full		:  1;	/* MIB counters half full interrupt */
		unsigned int rx_pause_on	:  1;	/* received pause on frame interrupt */
		unsigned int tx_pause_on	:  1;	/* transmit pause on frame interrupt */	 
		unsigned int rx_pause_off   :  1;	/* received pause off frame interrupt */
		unsigned int tx_pause_off	:  1;	/* received pause off frame interrupt */
		unsigned int rx_overrun		:  1;   /* EMAC Rx FIFO overrun interrupt */
		unsigned int tx_underrun	:  1;	/* EMAC Tx FIFO underrun interrupt */
		unsigned int				:  6;
		unsigned int m_tx_fail 		:  1;	/* Tx fail interrupt mask */
		unsigned int m_cnt_full		:  1;	/* MIB counters half full interrupt mask */
		unsigned int m_rx_pause_on	:  1;	/* received pause on frame interrupt mask */
		unsigned int m_tx_pause_on  :  1;	/* transmit pause on frame interrupt mask */
		unsigned int m_rx_pause_off :  1;	/* received pause off frame interrupt mask */
		unsigned int m_tx_pause_off	:  1;	/* received pause off frame interrupt mask */
		unsigned int m_rx_overrun	:  1;   /* EMAC Rx FIFO overrun interrupt mask */
		unsigned int m_tx_underrun	:  1;	/* EMAC Tx FIFO underrun interrupt mask */
#else
		unsigned int m_tx_underrun	:  1;	/* EMAC Tx FIFO underrun interrupt mask */
		unsigned int m_rx_overrun	:  1;   /* EMAC Rx FIFO overrun interrupt mask */
		unsigned int m_tx_pause_off	:  1;	/* received pause off frame interrupt mask */
		unsigned int m_rx_pause_off :  1;	/* received pause off frame interrupt mask */
		unsigned int m_tx_pause_on  :  1;	/* transmit pause on frame interrupt mask */
		unsigned int m_rx_pause_on	:  1;	/* received pause on frame interrupt mask */
		unsigned int m_cnt_full		:  1;	/* MIB counters half full interrupt mask */
		unsigned int m_tx_fail 		:  1;	/* Tx fail interrupt mask */
		unsigned int				:  6;
		unsigned int tx_underrun	:  1;	/* EMAC Tx FIFO underrun interrupt */
		unsigned int rx_overrun		:  1;   /* EMAC Rx FIFO overrun interrupt */
		unsigned int tx_pause_off	:  1;	/* received pause off frame interrupt */
		unsigned int rx_pause_off   :  1;	/* received pause off frame interrupt */
		unsigned int tx_pause_on	:  1;	/* transmit pause on frame interrupt */	 
		unsigned int rx_pause_on	:  1;	/* received pause on frame interrupt */
		unsigned int cnt_full		:  1;	/* MIB counters half full interrupt */
		unsigned int tx_fail		:  1; 	/* Tx fail interrupt */
		unsigned int 				: 10;
#endif
	} bits;
} EMAC_INT_MASK_T;		


/*******************************************/
/* the register structure of EMAC DMA      */
/*******************************************/
typedef union
{
	unsigned int bits32;
	struct bit2_ff00
	{
#if (BIG_ENDIAN==1) 	    
		unsigned int rp_wclk		:  4;	/* DMA_APB write clock period */
		unsigned int rp_rclk		:  4;	/* DMA_APB read clock period */
		unsigned int 				:  8;
		unsigned int device_id		: 12;
		unsigned int revision_id	:  4;
#else
		unsigned int revision_id	:  4;
		unsigned int device_id		: 12;
		unsigned int 				:  8;
		unsigned int rp_rclk		:  4;	/* DMA_APB read clock period */
		unsigned int rp_wclk		:  4;	/* DMA_APB write clock period */
#endif
	} bits;
} EMAC_DMA_DEVICE_ID_T;

typedef union
{
	unsigned int bits32;
	struct bit2_ff04
	{
#if (BIG_ENDIAN==1) 	    
		unsigned int ts_finish		:  1;	/* finished tx interrupt */
		unsigned int ts_derr		:  1;   /* AHB Bus Error while tx */ 
		unsigned int ts_perr		:  1;   /* Tx Descriptor protocol error */
		unsigned int ts_eodi		:  1;	/* TxDMA end of descriptor interrupt */
		unsigned int ts_eofi		:  1;   /* TxDMA end of frame interrupt */
		unsigned int rs_finish		:  1;   /* finished rx interrupt */
		unsigned int rs_derr		:  1;   /* AHB Bus Error while rx */ 
		unsigned int rs_perr		:  1;   /* Rx Descriptor protocol error */
		unsigned int rs_eodi		:  1;	/* RxDMA end of descriptor interrupt */
		unsigned int rs_eofi		:  1;	/* RxDMA end of frame interrupt */
		unsigned int tx_fail		:  1; 	/* Tx fail interrupt */
		unsigned int cnt_full		:  1;	/* MIB counters half full interrupt */
		unsigned int rx_pause_on	:  1;	/* received pause on frame interrupt */
		unsigned int tx_pause_on	:  1;	/* transmit pause on frame interrupt */	 
		unsigned int rx_pause_off   :  1;	/* received pause off frame interrupt */
		unsigned int tx_pause_off	:  1;	/* received pause off frame interrupt */
		unsigned int rx_overrun		:  1;   /* EMAC Rx FIFO overrun interrupt */
		unsigned int tx_underrun	:  1;	/* EMAC Tx FIFO underrun interrupt */
		unsigned int        		:  1;	/* write 1 to this bit will cause DMA HClk domain soft reset */
		unsigned int            	:  1;   /* write 1 to this bit will cause DMA PClk domain soft reset */
		unsigned int 				:  3;
		unsigned int loop_back		:  1;	/* loopback TxDMA to RxDMA */
		unsigned int m_tx_fail 		:  1;	/* Tx fail interrupt mask */
		unsigned int m_cnt_full		:  1;	/* MIB counters half full interrupt mask */
		unsigned int m_rx_pause_on	:  1;	/* received pause on frame interrupt mask */
		unsigned int m_tx_pause_on  :  1;	/* transmit pause on frame interrupt mask */
		unsigned int m_rx_pause_off :  1;	/* received pause off frame interrupt mask */
		unsigned int m_tx_pause_off	:  1;	/* received pause off frame interrupt mask */
		unsigned int m_rx_overrun	:  1;   /* EMAC Rx FIFO overrun interrupt mask */
		unsigned int m_tx_underrun	:  1;	/* EMAC Tx FIFO underrun interrupt mask */
#else
		unsigned int m_tx_underrun	:  1;	/* EMAC Tx FIFO underrun interrupt mask */
		unsigned int m_rx_overrun	:  1;   /* EMAC Rx FIFO overrun interrupt mask */
		unsigned int m_tx_pause_off	:  1;	/* received pause off frame interrupt mask */
		unsigned int m_rx_pause_off :  1;	/* received pause off frame interrupt mask */
		unsigned int m_tx_pause_on  :  1;	/* transmit pause on frame interrupt mask */
		unsigned int m_rx_pause_on	:  1;	/* received pause on frame interrupt mask */
		unsigned int m_cnt_full		:  1;	/* MIB counters half full interrupt mask */
		unsigned int m_tx_fail 		:  1;	/* Tx fail interrupt mask */
		unsigned int loop_back		:  1;	/* loopback TxDMA to RxDMA */
		unsigned int 				:  3;
		unsigned int            	:  1;   /* write 1 to this bit will cause DMA PClk domain soft reset */
		unsigned int        		:  1;	/* write 1 to this bit will cause DMA HClk domain soft reset */
		unsigned int tx_underrun	:  1;	/* EMAC Tx FIFO underrun interrupt */
		unsigned int rx_overrun		:  1;   /* EMAC Rx FIFO overrun interrupt */
		unsigned int tx_pause_off	:  1;	/* received pause off frame interrupt */
		unsigned int rx_pause_off   :  1;	/* received pause off frame interrupt */
		unsigned int tx_pause_on	:  1;	/* transmit pause on frame interrupt */	 
		unsigned int rx_pause_on	:  1;	/* received pause on frame interrupt */
		unsigned int cnt_full		:  1;	/* MIB counters half full interrupt */
		unsigned int tx_fail		:  1; 	/* Tx fail interrupt */
		unsigned int rs_eofi		:  1;	/* RxDMA end of frame interrupt */
		unsigned int rs_eodi		:  1;	/* RxDMA end of descriptor interrupt */
		unsigned int rs_perr		:  1;   /* Rx Descriptor protocol error */
		unsigned int rs_derr		:  1;   /* AHB Bus Error while rx */ 
		unsigned int rs_finish		:  1;   /* finished rx interrupt */
		unsigned int ts_eofi		:  1;   /* TxDMA end of frame interrupt */
		unsigned int ts_eodi		:  1;	/* TxDMA end of descriptor interrupt */
		unsigned int ts_perr		:  1;   /* Tx Descriptor protocol error */
		unsigned int ts_derr		:  1;   /* AHB Bus Error while tx */ 
		unsigned int ts_finish		:  1;	/* finished tx interrupt */
#endif
	} bits;
} EMAC_DMA_STATUS_T;

typedef union
{
	unsigned int bits32;
	struct bit2_ff08
	{
#if (BIG_ENDIAN==1) 	    
		unsigned int td_start		:  1;	/* Start DMA transfer */
		unsigned int td_continue	:  1;   /* Continue DMA operation */
		unsigned int td_chain_mode	:  1;	/* Descriptor Chain Mode;1-Descriptor Chain mode, 0-Direct DMA mode*/
		unsigned int 				:  2;
		unsigned int td_prot		:  4;	/* TxDMA protection control */
		unsigned int td_burst_size  :  2;	/* TxDMA max burst size for every AHB request */
		unsigned int td_bus		    :  1;	/* peripheral bus width;0 - 8 bits;1 - 16 bits */
		unsigned int td_endian		:  1;	/* AHB Endian. 0-little endian; 1-big endian */
		unsigned int td_finish_en   :  1;	/* DMA Finish Event Interrupt Enable;1-enable;0-mask */
		unsigned int td_fail_en 	:  1;	/* DMA Fail Interrupt Enable;1-enable;0-mask */
		unsigned int td_perr_en 	:  1;	/* Protocol Failure Interrupt Enable;1-enable;0-mask */
		unsigned int td_eod_en  	:  1;	/* End of Descriptor interrupt Enable;1-enable;0-mask */
		unsigned int td_eof_en      :  1;   /* End of frame interrupt Enable;1-enable;0-mask */
		unsigned int 				: 14;
#else
		unsigned int 				: 14;
		unsigned int td_eof_en      :  1;   /* End of frame interrupt Enable;1-enable;0-mask */
		unsigned int td_eod_en  	:  1;	/* End of Descriptor interrupt Enable;1-enable;0-mask */
		unsigned int td_perr_en 	:  1;	/* Protocol Failure Interrupt Enable;1-enable;0-mask */
		unsigned int td_fail_en 	:  1;	/* DMA Fail Interrupt Enable;1-enable;0-mask */
		unsigned int td_finish_en   :  1;	/* DMA Finish Event Interrupt Enable;1-enable;0-mask */
		unsigned int td_endian		:  1;	/* AHB Endian. 0-little endian; 1-big endian */
		unsigned int td_bus		    :  1;	/* peripheral bus width;0 - 8 bits;1 - 16 bits */
		unsigned int td_burst_size  :  2;	/* TxDMA max burst size for every AHB request */
		unsigned int td_prot		:  4;	/* TxDMA protection control */
		unsigned int 				:  2;
		unsigned int td_chain_mode	:  1;	/* Descriptor Chain Mode;1-Descriptor Chain mode, 0-Direct DMA mode*/
		unsigned int td_continue	:  1;   /* Continue DMA operation */
		unsigned int td_start		:  1;	/* Start DMA transfer */
#endif
	} bits;
} EMAC_TXDMA_CTRL_T;
				
typedef union 
{
	unsigned int bits32;
	struct bit2_ff0c
	{
#if (BIG_ENDIAN==1) 	    
		unsigned int td_first_des_ptr	: 28;/* first descriptor address */
		unsigned int td_busy			:  1;/* 1-TxDMA busy; 0-TxDMA idle */
		unsigned int 					:  3;
#else
		unsigned int 					:  3;
		unsigned int td_busy			:  1;/* 1-TxDMA busy; 0-TxDMA idle */
		unsigned int td_first_des_ptr	: 28;/* first descriptor address */
#endif
	} bits;
} EMAC_TXDMA_FIRST_DESC_T;					

typedef union
{
	unsigned int bits32;
	struct bit2_ff10
	{
#if (BIG_ENDIAN==1) 	    
		unsigned int ndar			: 28;	/* next descriptor address */
		unsigned int eofie			:  1;	/* end of frame interrupt enable */
		unsigned int dec			:  1;	/* AHB bus address increment(0)/decrement(1) */
		unsigned int sof_eof		:  2;
#else
		unsigned int sof_eof		:  2;
		unsigned int dec			:  1;	/* AHB bus address increment(0)/decrement(1) */
		unsigned int eofie			:  1;	/* end of frame interrupt enable */
		unsigned int ndar			: 28;	/* next descriptor address */
#endif
	} bits;
} EMAC_TXDMA_CURR_DESC_T;
			

typedef union
{
	unsigned int bits32;
	struct bit2_ff14
	{
#if (BIG_ENDIAN==1) 	    
		unsigned int rd_start		:  1;	/* Start DMA transfer */
		unsigned int rd_continue	:  1;   /* Continue DMA operation */
		unsigned int rd_chain_mode	:  1;	/* Descriptor Chain Mode;1-Descriptor Chain mode, 0-Direct DMA mode*/
		unsigned int 				:  2;
		unsigned int rd_prot		:  4;	/* DMA protection control */
		unsigned int rd_burst_size  :  2;	/* DMA max burst size for every AHB request */
		unsigned int rd_bus		    :  1;	/* peripheral bus width;0 - 8 bits;1 - 16 bits */
		unsigned int rd_endian		:  1;	/* AHB Endian. 0-little endian; 1-big endian */
		unsigned int rd_finish_en   :  1;	/* DMA Finish Event Interrupt Enable;1-enable;0-mask */
		unsigned int rd_fail_en  	:  1;	/* DMA Fail Interrupt Enable;1-enable;0-mask */
		unsigned int rd_perr_en 	:  1;	/* Protocol Failure Interrupt Enable;1-enable;0-mask */
		unsigned int rd_eod_en  	:  1;	/* End of Descriptor interrupt Enable;1-enable;0-mask */
		unsigned int rd_eof_en      :  1;   /* End of frame interrupt Enable;1-enable;0-mask */
		unsigned int 				: 14;
#else
		unsigned int 				: 14;
		unsigned int rd_eof_en      :  1;   /* End of frame interrupt Enable;1-enable;0-mask */
		unsigned int rd_eod_en  	:  1;	/* End of Descriptor interrupt Enable;1-enable;0-mask */
		unsigned int rd_perr_en 	:  1;	/* Protocol Failure Interrupt Enable;1-enable;0-mask */
		unsigned int rd_fail_en  	:  1;	/* DMA Fail Interrupt Enable;1-enable;0-mask */
		unsigned int rd_finish_en   :  1;	/* DMA Finish Event Interrupt Enable;1-enable;0-mask */
		unsigned int rd_endian		:  1;	/* AHB Endian. 0-little endian; 1-big endian */
		unsigned int rd_bus		    :  1;	/* peripheral bus width;0 - 8 bits;1 - 16 bits */
		unsigned int rd_burst_size  :  2;	/* DMA max burst size for every AHB request */
		unsigned int rd_prot		:  4;	/* DMA protection control */
		unsigned int 				:  2;
		unsigned int rd_chain_mode	:  1;	/* Descriptor Chain Mode;1-Descriptor Chain mode, 0-Direct DMA mode*/
		unsigned int rd_continue	:  1;   /* Continue DMA operation */
		unsigned int rd_start		:  1;	/* Start DMA transfer */
#endif
	} bits;
} EMAC_RXDMA_CTRL_T;
				
typedef union 
{
	unsigned int bits32;
	struct bit2_ff18
	{
#if (BIG_ENDIAN==1) 	    
		unsigned int rd_first_des_ptr	: 28;/* first descriptor address */
		unsigned int rd_busy			:  1;/* 1-RxDMA busy; 0-RxDMA idle */
		unsigned int 					:  3;
#else
		unsigned int 					:  3;
		unsigned int rd_busy			:  1;/* 1-RxDMA busy; 0-RxDMA idle */
		unsigned int rd_first_des_ptr	: 28;/* first descriptor address */
#endif
	} bits;
} EMAC_RXDMA_FIRST_DESC_T;					

typedef union
{
	unsigned int bits32;
	struct bit2_ff1c
	{
#if (BIG_ENDIAN==1) 	    
		unsigned int ndar			: 28;	/* next descriptor address */
		unsigned int eofie			:  1;	/* end of frame interrupt enable */
		unsigned int dec			:  1;	/* AHB bus address increment(0)/decrement(1) */
		unsigned int sof_eof		:  2;
#else
		unsigned int sof_eof		:  2;
		unsigned int dec			:  1;	/* AHB bus address increment(0)/decrement(1) */
		unsigned int eofie			:  1;	/* end of frame interrupt enable */
		unsigned int ndar			: 28;	/* next descriptor address */
#endif
	} bits;
} EMAC_RXDMA_CURR_DESC_T;


/********************************************/
/*          Descriptor Format               */
/********************************************/
typedef union frame_control_u
	{
		unsigned int bits32;
		struct bd_bits_0000
		{
#if (BIG_ENDIAN==1) 	    
			unsigned int own 		: 1;	/* owner bit. 0-CPU, 1-DMA */
			unsigned int derr		: 1;	/* data error during processing this descriptor */
			unsigned int perr		: 1;	/* protocol error during processing this descriptor */
			unsigned int            : 1;
			unsigned int vlan_tag   : 1;    /* 802.1q vlan tag packet */
			unsigned int mcast_frame: 1;	/* received frame is multicast frame */
			unsigned int bcast_frame: 1;	/* received frame is broadcast frame */
			unsigned int ucast_mac1 : 1;	/* received frame is unicast frame of MAC address 1 */
			unsigned int ucast_mac2 : 1;	/* received frame is unicast frame of MAC address 2 */
			unsigned int frame_state: 3;    /* reference Rx Status1 */
			unsigned int desc_count : 4;	/* number of descriptors used for the current frame */
			unsigned int buffer_size:16;	/* transfer buffer size associated with current description*/
#else
			unsigned int buffer_size:16;	/* transfer buffer size associated with current description*/
			unsigned int desc_count : 4;	/* number of descriptors used for the current frame */
			unsigned int frame_state: 3;    /* reference Rx Status1 */
			unsigned int ucast_mac2 : 1;	/* received frame is unicast frame of MAC address 2 */
			unsigned int ucast_mac1 : 1;	/* received frame is unicast frame of MAC address 1 */
			unsigned int bcast_frame: 1;	/* received frame is broadcast frame */
			unsigned int mcast_frame: 1;	/* received frame is multicast frame */
			unsigned int vlan_tag   : 1;    /* 802.1q vlan tag packet */
			unsigned int            : 1;
			unsigned int perr		: 1;	/* protocol error during processing this descriptor */
			unsigned int derr		: 1;	/* data error during processing this descriptor */
			unsigned int own 		: 1;	/* owner bit. 0-CPU, 1-DMA */
#endif
		} bits_rx;
		
		struct bd_bits_0001
		{
#if (BIG_ENDIAN==1) 	    
			unsigned int own 		: 1;	/* owner bit. 0-CPU, 1-DMA */
			unsigned int derr		: 1;	/* data error during processing this descriptor */
			unsigned int perr		: 1;	/* protocol error during processing this descriptor */
			unsigned int            : 1;
			unsigned int coll_retry : 4;	/* collision retry count */
			unsigned int fifo_error : 1;	/* Tx FIFO Error */
			unsigned int coll_abort : 1;	/* Collision aborted */
			unsigned int late_col   : 1;	/* late collision aborted */
			unsigned int success_tx : 1;    /* successful transmitted */
			unsigned int desc_count : 4;	/* number of descriptors used for the current frame */
			unsigned int buffer_size:16;	/* transfer buffer size associated with current description*/
#else
			unsigned int buffer_size:16;	/* transfer buffer size associated with current description*/
			unsigned int desc_count : 4;	/* number of descriptors used for the current frame */
			unsigned int success_tx : 1;    /* successful transmitted */
			unsigned int late_col   : 1;	/* late collision aborted */
			unsigned int coll_abort : 1;	/* Collision aborted */
			unsigned int fifo_error : 1;	/* Tx FIFO Error */
			unsigned int coll_retry : 4;	/* collision retry count */
			unsigned int            : 1;
			unsigned int perr		: 1;	/* protocol error during processing this descriptor */
			unsigned int derr		: 1;	/* data error during processing this descriptor */
			unsigned int own 		: 1;	/* owner bit. 0-CPU, 1-DMA */
#endif
        } bits_tx;		    	
} BD_FRAME_CTRL_T;

typedef union flag_status_u
	{
		unsigned int bits32;
		struct bd_bits_0004
		{
#if (BIG_ENDIAN==1) 
            unsigned int priority   : 3;    /* user priority extracted from receiving frame*/
            unsigned int cfi        : 1;	/* cfi extracted from receiving frame*/    
			unsigned int vlan_id    :12;	/* VLAN ID extracted from receiving frame */
			unsigned int frame_count:16;	/* received frame byte count,include CRC,not include VLAN TIC */ 
#else
			unsigned int frame_count:16;	/* received frame byte count,include CRC,not include VLAN TIC */ 
			unsigned int vlan_id    :12;	/* VLAN ID extracted from receiving frame */
            unsigned int cfi        : 1;	/* cfi extracted from receiving frame*/    
            unsigned int priority   : 3;    /* user priority extracted from receiving frame*/
#endif
		} bits_rx_status;

		struct bd_bits_0005
		{
#if (BIG_ENDIAN==1) 	    
            unsigned int priority   : 3;    /* user priority to transmit*/
            unsigned int cfi        : 1;	/* cfi to transmit*/    
			unsigned int vlan_id    :12;	/* VLAN ID to transmit */
			unsigned int vlan_enable: 1;	/* VLAN TIC insertion enable */ 
			unsigned int frame_count:15;    /* total tx frame byte count */
#else
			unsigned int frame_count:15;    /* total tx frame byte count */
			unsigned int vlan_enable: 1;	/* VLAN TIC insertion enable */ 
			unsigned int vlan_id    :12;	/* VLAN ID to transmit */
            unsigned int cfi        : 1;	/* cfi to transmit*/    
            unsigned int priority   : 3;    /* user priority to transmit*/
#endif
		} bits_tx_flag;
} BD_FLAG_STATUS_T;

typedef union next_desc_u
	{
		unsigned int next_descriptor;
		struct bd_bits_000c
		{
#if (BIG_ENDIAN==1) 	    
			unsigned int ndar		:28;	/* next descriptor address */
			unsigned int eofie		: 1;	/* end of frame interrupt enable */
			unsigned int    		: 1;	 
			unsigned int sof_eof	: 2;	/* 00-the linking descriptor   01-the last descriptor of a frame*/
			                                /* 10-the first descriptor of a frame    11-only one descriptor for a frame*/
#else
			unsigned int sof_eof	: 2;	/* 00-the linking descriptor   01-the last descriptor of a frame*/
			                                /* 10-the first descriptor of a frame    11-only one descriptor for a frame*/
			unsigned int    		: 1;	 
			unsigned int eofie		: 1;	/* end of frame interrupt enable */
			unsigned int ndar		:28;	/* next descriptor address */
#endif
		} bits;                    			
} BD_NEXT_DESC_T;					        
		
typedef struct descriptor_t
{
	union frame_control_t
	{
		unsigned int bits32;
		struct bits_0000
		{
#if (BIG_ENDIAN==1) 	    
			unsigned int own 		: 1;	/* owner bit. 0-CPU, 1-DMA */
			unsigned int derr		: 1;	/* data error during processing this descriptor */
			unsigned int perr		: 1;	/* protocol error during processing this descriptor */
			unsigned int            : 1;
			unsigned int vlan_tag   : 1;    /* 802.1q vlan tag packet */
			unsigned int mcast_frame: 1;	/* received frame is multicast frame */
			unsigned int bcast_frame: 1;	/* received frame is broadcast frame */
			unsigned int ucast_mac1 : 1;	/* received frame is unicast frame of MAC address 1 */
			unsigned int ucast_mac2 : 1;	/* received frame is unicast frame of MAC address 2 */
			unsigned int frame_state: 3;    /* reference Rx Status1 */
			unsigned int desc_count : 4;	/* number of descriptors used for the current frame */
			unsigned int buffer_size:16;	/* transfer buffer size associated with current description*/
#else
			unsigned int buffer_size:16;	/* transfer buffer size associated with current description*/
			unsigned int desc_count : 4;	/* number of descriptors used for the current frame */
			unsigned int frame_state: 3;    /* reference Rx Status1 */
			unsigned int ucast_mac2 : 1;	/* received frame is unicast frame of MAC address 2 */
			unsigned int ucast_mac1 : 1;	/* received frame is unicast frame of MAC address 1 */
			unsigned int bcast_frame: 1;	/* received frame is broadcast frame */
			unsigned int mcast_frame: 1;	/* received frame is multicast frame */
			unsigned int vlan_tag   : 1;    /* 802.1q vlan tag packet */
			unsigned int            : 1;
			unsigned int perr		: 1;	/* protocol error during processing this descriptor */
			unsigned int derr		: 1;	/* data error during processing this descriptor */
			unsigned int own 		: 1;	/* owner bit. 0-CPU, 1-DMA */
#endif
		} bits_rx;
		
		struct bits_0001
		{
#if (BIG_ENDIAN==1) 	    
			unsigned int own 		: 1;	/* owner bit. 0-CPU, 1-DMA */
			unsigned int derr		: 1;	/* data error during processing this descriptor */
			unsigned int perr		: 1;	/* protocol error during processing this descriptor */
			unsigned int            : 1;
			unsigned int coll_retry : 4;	/* collision retry count */
			unsigned int fifo_error : 1;	/* Tx FIFO Error */
			unsigned int coll_abort : 1;	/* Collision aborted */
			unsigned int late_col   : 1;	/* late collision aborted */
			unsigned int success_tx : 1;    /* successful transmitted */
			unsigned int desc_count : 4;	/* number of descriptors used for the current frame */
			unsigned int buffer_size:16;	/* transfer buffer size associated with current description*/
#else
			unsigned int buffer_size:16;	/* transfer buffer size associated with current description*/
			unsigned int desc_count : 4;	/* number of descriptors used for the current frame */
			unsigned int success_tx : 1;    /* successful transmitted */
			unsigned int late_col   : 1;	/* late collision aborted */
			unsigned int coll_abort : 1;	/* Collision aborted */
			unsigned int fifo_error : 1;	/* Tx FIFO Error */
			unsigned int coll_retry : 4;	/* collision retry count */
			unsigned int            : 1;
			unsigned int perr		: 1;	/* protocol error during processing this descriptor */
			unsigned int derr		: 1;	/* data error during processing this descriptor */
			unsigned int own 		: 1;	/* owner bit. 0-CPU, 1-DMA */
#endif
        } bits_tx;		    	
	} frame_ctrl;
	
	union flag_status_t
	{
		unsigned int bits32;
		struct bits_0004
		{
#if (BIG_ENDIAN==1) 
            unsigned int priority   : 3;    /* user priority extracted from receiving frame*/
            unsigned int cfi        : 1;	/* cfi extracted from receiving frame*/    
			unsigned int vlan_id    :12;	/* VLAN ID extracted from receiving frame */
			unsigned int frame_count:16;	/* received frame byte count,include CRC,not include VLAN TIC */ 
#else
			unsigned int frame_count:16;	/* received frame byte count,include CRC,not include VLAN TIC */ 
			unsigned int vlan_id    :12;	/* VLAN ID extracted from receiving frame */
            unsigned int cfi        : 1;	/* cfi extracted from receiving frame*/    
            unsigned int priority   : 3;    /* user priority extracted from receiving frame*/
#endif
		} bits_rx_status;

		struct bits_0005
		{
#if (BIG_ENDIAN==1) 	    
            unsigned int priority   : 3;    /* user priority to transmit*/
            unsigned int cfi        : 1;	/* cfi to transmit*/    
			unsigned int vlan_id    :12;	/* VLAN ID to transmit */
			unsigned int vlan_enable: 1;	/* VLAN TIC insertion enable */ 
			unsigned int frame_count:15;    /* total tx frame byte count */
#else
			unsigned int frame_count:15;    /* total tx frame byte count */
			unsigned int vlan_enable: 1;	/* VLAN TIC insertion enable */ 
			unsigned int vlan_id    :12;	/* VLAN ID to transmit */
            unsigned int cfi        : 1;	/* cfi to transmit*/    
            unsigned int priority   : 3;    /* user priority to transmit*/
#endif
		} bits_tx_flag;
	} flag_status;

	unsigned int buf_adr;	/* data buffer address */	
	
	union next_desc_t
	{
		unsigned int next_descriptor;
		struct bits_000c
		{
#if (BIG_ENDIAN==1) 	    
			unsigned int ndar		:28;	/* next descriptor address */
			unsigned int eofie		: 1;	/* end of frame interrupt enable */
			unsigned int    		: 1;	 
			unsigned int sof_eof	: 2;	/* 00-the linking descriptor   01-the last descriptor of a frame*/
			                                /* 10-the first descriptor of a frame    11-only one descriptor for a frame*/
#else
			unsigned int sof_eof	: 2;	/* 00-the linking descriptor   01-the last descriptor of a frame*/
			                                /* 10-the first descriptor of a frame    11-only one descriptor for a frame*/
			unsigned int    		: 1;	 
			unsigned int eofie		: 1;	/* end of frame interrupt enable */
			unsigned int ndar		:28;	/* next descriptor address */
#endif
		} bits;                    			
	} next_desc;					        
} EMAC_DESCRIPTOR_T;		            	

typedef struct emac_conf {
	struct net_device *dev;
	int portmap;
	int vid;
} sys_emac_conf;
		 
typedef struct emac_private {
    unsigned char        *bufs;           /* bounce buffer region. */
    EMAC_DESCRIPTOR_T    *tx_desc;        /* point to virtual TX descriptor address*/
    EMAC_DESCRIPTOR_T    *rx_desc;        /* point to virtual RX descriptor address*/
    short 		rx_head;                          /* index of rx head */
    short 		rx_tail;                          /* index of rx tail */
    short 		tx_head;                          /* index of tx head */
    short 		tx_tail;                          /* index of tx tail */
    short 		rd_index[RX_DESC_NUM];            /* index of related rx desc */
	int			tx_err;
	unsigned char       *tx_bufs;	/* Tx bounce buffer region. */
	unsigned char       *rx_bufs;
	//EMAC_DESCRIPTOR_T	*tx_desc;	/* point to virtual TX descriptor address*/
	//EMAC_DESCRIPTOR_T	*rx_desc;	/* point to virtual RX descriptor address*/
	EMAC_DESCRIPTOR_T	*tx_cur_desc;	/* point to current TX descriptor */
	EMAC_DESCRIPTOR_T	*rx_cur_desc;	/* point to current RX descriptor */
	EMAC_DESCRIPTOR_T   *tx_finished_desc;
	EMAC_DESCRIPTOR_T   *rx_finished_desc;
	unsigned long       cur_tx;
	unsigned int        cur_rx;	/* Index into the Rx buffer of next Rx pkt. */
	unsigned int        tx_flag;
	unsigned long       dirty_tx;
	// unsigned char       *tx_buf[TX_DESC_NUM];	/* Tx bounce buffers */
	dma_addr_t          tx_desc_dma; /* physical TX descriptor address */
	dma_addr_t          rx_desc_dma;	/* physical RX descriptor address */
	dma_addr_t          tx_bufs_dma; /* physical TX descriptor address */
	dma_addr_t          rx_bufs_dma; /* physical RX descriptor address */
#ifdef EMAC_STATISTICS
	unsigned long		rx_overrun;
	unsigned long		tx_underrun;
	unsigned long		tx_no_resource;
	unsigned long		rx_pkts;
	unsigned long		tx_pkts;
	unsigned long		interrupt_cnt;
	unsigned long		rx_intr_cnt;
	unsigned long		tx_intr_cnt;
#endif	
} EMAC_INFO_T;;


struct reg_ioctl_data {
    unsigned int    reg_addr;   /* the register address */
    unsigned int    val_in;     /* data write to the register */
    unsigned int    val_out;    /* data read from the register */
};

#define sl2312_eth_disable_interrupt()	REG32(SL2312_INTERRUPT_BASE + SL2312_IRQ_MASK) &= ~(1<<SL2312_INTERRUPT_EMAC)
#define sl2312_eth_enable_interrupt()	REG32(SL2312_INTERRUPT_BASE + SL2312_IRQ_MASK) |=  (1<<SL2312_INTERRUPT_EMAC)

#endif //_EMAC_SL2312_H

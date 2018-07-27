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
#ifndef _GMAC_SL2312_H
#define _GMAC_SL2312_H

typedef unsigned int		dma_addr_t;
#define ETHER_ADDR_LEN		6
#define BIG_ENDIAN  		0

/* define chip information */
#define DRV_NAME			"SL2312"
#define DRV_VERSION			"0.1.1"
#define SL2312_DRIVER_NAME  DRV_NAME " Fast Ethernet driver " DRV_VERSION

#define TX_DESC_NUM			16
#define RX_DESC_NUM			16

/* define TX/RX descriptor parameter */
#define MAX_ETH_FRAME_SIZE	2048
#define TX_BUF_SIZE			MAX_ETH_FRAME_SIZE
#define TX_BUF_TOT_LEN		(TX_BUF_SIZE * TX_DESC_NUM)
#define RX_BUF_SIZE			MAX_ETH_FRAME_SIZE
#define RX_BUF_TOT_LEN		(RX_BUF_SIZE * RX_DESC_NUM)


/* define EMAC base address */
//#define GMAC_BASE_ADDR		(SL2312_GMAC1_BASE)
#define GMAC_BASE_ADDR		(SL2312_GMAC0_BASE)

/* define owner bit */
#define CPU					0
#define DMA					1

/* define PHY address */
/* define PHY address */
#define HPHY_ADDR   0x01
#define GPHY_ADDR   0x02
#define PHY_ADDR    		0x01    //MII PHY
//#define PHY_ADDR    		0x02    //GMII PHY

#define ADM_EECS			0x01
#define ADM_EECK			0x02
#define ADM_EDIO			0x04

#define LDN_GPIO			0x07
#define LPC_BASE_ADDR		SL2312_LPC_IO_BASE
#define IT8712_GPIO_BASE	0x800	// 0x800-0x804 for GPIO set1-set5
#define BASE_OFF			0x03

/***************************************/
/* the offset address of GMAC register */
/***************************************/
enum GMAC_REGISTER {
	GMAC_STA_ADD0 	= 0x0000,
	GMAC_STA_ADD1	= 0x0004,
	GMAC_STA_ADD2	= 0x0008,
	GMAC_RX_FLTR	= 0x000c,
	GMAC_MCAST_FIL0 = 0x0010,
	GMAC_MCAST_FIL1 = 0x0014,
	GMAC_CONFIG0	= 0x0018,
	GMAC_CONFIG1	= 0x001c,
	GMAC_CONFIG2	= 0x0020,
	GMAC_BNCR		= 0x0024,
	GMAC_RBNR		= 0x0028,
	GMAC_STATUS		= 0x002c,
	GMAC_IN_DISCARDS= 0x0030,
	GMAC_IN_ERRORS  = 0x0034,
	GMAC_IN_MCAST   = 0x0038,
	GMAC_IN_BCAST   = 0x003c,
	GMAC_IN_MAC1    = 0x0040,
	GMAC_IN_MAC2    = 0x0044
};

/*******************************************/
/* the offset address of GMAC DMA register */
/*******************************************/
enum GMAC_DMA_REGISTER {
	GMAC_DMA_DEVICE_ID		= 0xff00,
	GMAC_DMA_STATUS			= 0xff04,
	GMAC_TXDMA_CTRL 	 	= 0xff08,
	GMAC_TXDMA_FIRST_DESC 	= 0xff0c,
	GMAC_TXDMA_CURR_DESC	= 0xff10,
	GMAC_RXDMA_CTRL			= 0xff14,
	GMAC_RXDMA_FIRST_DESC	= 0xff18,
	GMAC_RXDMA_CURR_DESC	= 0xff1c,
};

/*******************************************/
/* the register structure of GMAC          */
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
} GMAC_STA_ADD1_T;

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
} GMAC_RX_FLTR_T;

typedef union
{
	unsigned int bits32;
	struct bit1_0018
	{
#if (BIG_ENDIAN==1)
		unsigned int : 10;
		unsigned int inv_rx_clk     : 1;	/* Inverse RX Clock */
		unsigned int rising_latch   : 1;
        unsigned int rx_tag_remove  :  1;   /* Remove Rx VLAN tag */
        unsigned int ipv6_tss_rx_en :  1;   /* IPv6 TSS RX enable */
        unsigned int ipv4_tss_rx_en :  1;   /* IPv4 TSS RX enable */
        unsigned int rgmii_en       :  1;   /* RGMII in-band status enable */
		unsigned int tx_fc_en		:  1;	/* TX flow control enable */
		unsigned int rx_fc_en		:  1;	/* RX flow control enable */
		unsigned int sim_test		:  1;	/* speed up timers in simulation */
		unsigned int dis_col		:  1;	/* disable 16 collisions abort function */
		unsigned int dis_bkoff		:  1;	/* disable back-off function */
		unsigned int max_len		:  3;	/* maximum receive frame length allowed */
		unsigned int adj_ifg		:  4;	/* adjust IFG from 96+/-56 */
        unsigned int                :  1;   /* reserved */
		unsigned int loop_back		:  1;	/* transmit data loopback enable */
		unsigned int dis_rx			:  1;	/* disable receive */
		unsigned int dis_tx			:  1;	/* disable transmit */
#else
		unsigned int dis_tx			:  1;	/* disable transmit */
		unsigned int dis_rx			:  1;	/* disable receive */
		unsigned int loop_back		:  1;	/* transmit data loopback enable */
        unsigned int                :  1;   /* reserved */
		unsigned int adj_ifg		:  4;	/* adjust IFG from 96+/-56 */
		unsigned int max_len		:  3;	/* maximum receive frame length allowed */
		unsigned int dis_bkoff		:  1;	/* disable back-off function */
		unsigned int dis_col		:  1;	/* disable 16 collisions abort function */
		unsigned int sim_test		:  1;	/* speed up timers in simulation */
		unsigned int rx_fc_en		:  1;	/* RX flow control enable */
		unsigned int tx_fc_en		:  1;	/* TX flow control enable */
        unsigned int rgmii_en       :  1;   /* RGMII in-band status enable */
        unsigned int ipv4_tss_rx_en :  1;   /* IPv4 TSS RX enable */
        unsigned int ipv6_tss_rx_en :  1;   /* IPv6 TSS RX enable */
        unsigned int rx_tag_remove  :  1;   /* Remove Rx VLAN tag */
		unsigned int rising_latch   :  1;
		unsigned int inv_rx_clk : 1;	/* Inverse RX Clock */
		unsigned int : 10;
#endif
	} bits;
} GMAC_CONFIG0_T;

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
} GMAC_CONFIG1_T;

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
} GMAC_CONFIG2_T;

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
} GMAC_BNCR_T;

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
} GMAC_RBNR_T;

typedef union
{
	unsigned int bits32;
	struct bit1_002c
	{
#if (BIG_ENDIAN==1)
		unsigned int 				: 25;
		unsigned int mii_rmii		:  2;   /* PHY interface type */
		unsigned int phy_mode		:  1;	/* PHY interface mode in 10M-bps */
		unsigned int duplex			:  1;	/* duplex mode */
		unsigned int speed			:  2;	/* link speed(00->2.5M 01->25M 10->125M) */
		unsigned int link			:  1;	/* link status */
#else
		unsigned int link			:  1;	/* link status */
		unsigned int speed			:  2;	/* link speed(00->2.5M 01->25M 10->125M) */
		unsigned int duplex			:  1;	/* duplex mode */
		unsigned int phy_mode		:  1;	/* PHY interface mode in 10M-bps */
		unsigned int mii_rmii		:  2;   /* PHY interface type */
		unsigned int 				: 25;
#endif
	} bits;
} GMAC_STATUS_T;


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
		unsigned int rx_overrun		:  1;   /* GMAC Rx FIFO overrun interrupt */
		unsigned int tx_underrun	:  1;	/* GMAC Tx FIFO underrun interrupt */
		unsigned int				:  6;
		unsigned int m_tx_fail 		:  1;	/* Tx fail interrupt mask */
		unsigned int m_cnt_full		:  1;	/* MIB counters half full interrupt mask */
		unsigned int m_rx_pause_on	:  1;	/* received pause on frame interrupt mask */
		unsigned int m_tx_pause_on  :  1;	/* transmit pause on frame interrupt mask */
		unsigned int m_rx_pause_off :  1;	/* received pause off frame interrupt mask */
		unsigned int m_tx_pause_off	:  1;	/* received pause off frame interrupt mask */
		unsigned int m_rx_overrun	:  1;   /* GMAC Rx FIFO overrun interrupt mask */
		unsigned int m_tx_underrun	:  1;	/* GMAC Tx FIFO underrun interrupt mask */
#else
		unsigned int m_tx_underrun	:  1;	/* GMAC Tx FIFO underrun interrupt mask */
		unsigned int m_rx_overrun	:  1;   /* GMAC Rx FIFO overrun interrupt mask */
		unsigned int m_tx_pause_off	:  1;	/* received pause off frame interrupt mask */
		unsigned int m_rx_pause_off :  1;	/* received pause off frame interrupt mask */
		unsigned int m_tx_pause_on  :  1;	/* transmit pause on frame interrupt mask */
		unsigned int m_rx_pause_on	:  1;	/* received pause on frame interrupt mask */
		unsigned int m_cnt_full		:  1;	/* MIB counters half full interrupt mask */
		unsigned int m_tx_fail 		:  1;	/* Tx fail interrupt mask */
		unsigned int				:  6;
		unsigned int tx_underrun	:  1;	/* GMAC Tx FIFO underrun interrupt */
		unsigned int rx_overrun		:  1;   /* GMAC Rx FIFO overrun interrupt */
		unsigned int tx_pause_off	:  1;	/* received pause off frame interrupt */
		unsigned int rx_pause_off   :  1;	/* received pause off frame interrupt */
		unsigned int tx_pause_on	:  1;	/* transmit pause on frame interrupt */
		unsigned int rx_pause_on	:  1;	/* received pause on frame interrupt */
		unsigned int cnt_full		:  1;	/* MIB counters half full interrupt */
		unsigned int tx_fail		:  1; 	/* Tx fail interrupt */
		unsigned int 				: 10;
#endif
	} bits;
} GMAC_INT_MASK_T;


/*******************************************/
/* the register structure of GMAC DMA      */
/*******************************************/
typedef union
{
	unsigned int bits32;
	struct bit2_ff00
	{
#if (BIG_ENDIAN==1)
		unsigned int                :  7;   /* reserved */
		unsigned int s_ahb_err		:  1;	/* Slave AHB bus error */
		unsigned int tx_err_code    :  4;   /* TxDMA error code */
		unsigned int rx_err_code  	:  4;   /* RxDMA error code */
		unsigned int device_id		: 12;
		unsigned int revision_id	:  4;
#else
		unsigned int revision_id	:  4;
		unsigned int device_id		: 12;
		unsigned int rx_err_code  	:  4;   /* RxDMA error code */
		unsigned int tx_err_code    :  4;   /* TxDMA error code */
		unsigned int s_ahb_err		:  1;	/* Slave AHB bus error */
		unsigned int                :  7;   /* reserved */
#endif
	} bits;
} GMAC_DMA_DEVICE_ID_T;

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
		unsigned int        		:  1; 	/* Tx fail interrupt */
		unsigned int cnt_full		:  1;	/* MIB counters half full interrupt */
		unsigned int rx_pause_on	:  1;	/* received pause on frame interrupt */
		unsigned int tx_pause_on	:  1;	/* transmit pause on frame interrupt */
		unsigned int rx_pause_off   :  1;	/* received pause off frame interrupt */
		unsigned int tx_pause_off	:  1;	/* received pause off frame interrupt */
		unsigned int rx_overrun		:  1;   /* GMAC Rx FIFO overrun interrupt */
		unsigned int link_change	:  1;	/* GMAC link changed Interrupt for RGMII mode */
		unsigned int        		:  1;
		unsigned int            	:  1;
		unsigned int 				:  3;
		unsigned int loop_back		:  1;	/* loopback TxDMA to RxDMA */
		unsigned int        		:  1;	/* Tx fail interrupt mask */
		unsigned int m_cnt_full		:  1;	/* MIB counters half full interrupt mask */
		unsigned int m_rx_pause_on	:  1;	/* received pause on frame interrupt mask */
		unsigned int m_tx_pause_on  :  1;	/* transmit pause on frame interrupt mask */
		unsigned int m_rx_pause_off :  1;	/* received pause off frame interrupt mask */
		unsigned int m_tx_pause_off	:  1;	/* received pause off frame interrupt mask */
		unsigned int m_rx_overrun	:  1;   /* GMAC Rx FIFO overrun interrupt mask */
		unsigned int m_link_change	:  1;	/* GMAC link changed Interrupt mask for RGMII mode */
#else
		unsigned int m_link_change	:  1;	/* GMAC link changed Interrupt mask for RGMII mode */
		unsigned int m_rx_overrun	:  1;   /* GMAC Rx FIFO overrun interrupt mask */
		unsigned int m_tx_pause_off	:  1;	/* received pause off frame interrupt mask */
		unsigned int m_rx_pause_off :  1;	/* received pause off frame interrupt mask */
		unsigned int m_tx_pause_on  :  1;	/* transmit pause on frame interrupt mask */
		unsigned int m_rx_pause_on	:  1;	/* received pause on frame interrupt mask */
		unsigned int m_cnt_full		:  1;	/* MIB counters half full interrupt mask */
		unsigned int         		:  1;	/* Tx fail interrupt mask */
		unsigned int loop_back		:  1;	/* loopback TxDMA to RxDMA */
		unsigned int 				:  3;
		unsigned int            	:  1;
		unsigned int        		:  1;
		unsigned int link_change	:  1;	/* GMAC link changed Interrupt for RGMII mode */
		unsigned int rx_overrun		:  1;   /* GMAC Rx FIFO overrun interrupt */
		unsigned int tx_pause_off	:  1;	/* received pause off frame interrupt */
		unsigned int rx_pause_off   :  1;	/* received pause off frame interrupt */
		unsigned int tx_pause_on	:  1;	/* transmit pause on frame interrupt */
		unsigned int rx_pause_on	:  1;	/* received pause on frame interrupt */
		unsigned int cnt_full		:  1;	/* MIB counters half full interrupt */
		unsigned int        		:  1; 	/* Tx fail interrupt */
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
} GMAC_DMA_STATUS_T;

typedef union
{
	unsigned int bits32;
	struct bit2_ff08
	{
#if (BIG_ENDIAN==1)
		unsigned int td_start		:  1;	/* Start DMA transfer */
		unsigned int td_continue	:  1;   /* Continue DMA operation */
		unsigned int td_chain_mode	:  1;	/* Descriptor Chain Mode;1-Descriptor Chain mode, 0-Direct DMA mode*/
		unsigned int 				:  1;
		unsigned int td_prot		:  4;	/* TxDMA protection control */
		unsigned int td_burst_size  :  2;	/* TxDMA max burst size for every AHB request */
		unsigned int td_bus		    :  2;	/* peripheral bus width;0x->8 bits,10->16 bits,11->32 bits */
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
		unsigned int td_bus		    :  2;	/* peripheral bus width;0x->8 bits,10->16 bits,11->32 bits */
		unsigned int td_burst_size  :  2;	/* TxDMA max burst size for every AHB request */
		unsigned int td_prot		:  4;	/* TxDMA protection control */
		unsigned int 				:  1;
		unsigned int td_chain_mode	:  1;	/* Descriptor Chain Mode;1-Descriptor Chain mode, 0-Direct DMA mode*/
		unsigned int td_continue	:  1;   /* Continue DMA operation */
		unsigned int td_start		:  1;	/* Start DMA transfer */
#endif
	} bits;
} GMAC_TXDMA_CTRL_T;


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
} GMAC_TXDMA_FIRST_DESC_T;

typedef union
{
	unsigned int bits32;
	struct bit2_ff10
	{
#if (BIG_ENDIAN==1)
		unsigned int ndar			: 28;	/* next descriptor address */
		unsigned int eofie			:  1;	/* end of frame interrupt enable */
		unsigned int    			:  1;
		unsigned int sof_eof		:  2;
#else
		unsigned int sof_eof		:  2;
		unsigned int    			:  1;
		unsigned int eofie			:  1;	/* end of frame interrupt enable */
		unsigned int ndar			: 28;	/* next descriptor address */
#endif
	} bits;
} GMAC_TXDMA_CURR_DESC_T;


typedef union
{
	unsigned int bits32;
	struct bit2_ff14
	{
#if (BIG_ENDIAN==1)
		unsigned int rd_start		:  1;	/* Start DMA transfer */
		unsigned int rd_continue	:  1;   /* Continue DMA operation */
		unsigned int rd_chain_mode	:  1;	/* Descriptor Chain Mode;1-Descriptor Chain mode, 0-Direct DMA mode*/
		unsigned int 				:  1;
		unsigned int rd_prot		:  4;	/* DMA protection control */
		unsigned int rd_burst_size  :  2;	/* DMA max burst size for every AHB request */
		unsigned int rd_bus		    :  2;	/* peripheral bus width;0x->8 bits,10->16 bits,11->32 bits */
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
		unsigned int rd_bus		    :  2;	/* peripheral bus width;0x->8 bits,10->16 bits,11->32 bits */
		unsigned int rd_burst_size  :  2;	/* DMA max burst size for every AHB request */
		unsigned int rd_prot		:  4;	/* DMA protection control */
		unsigned int 				:  1;
		unsigned int rd_chain_mode	:  1;	/* Descriptor Chain Mode;1-Descriptor Chain mode, 0-Direct DMA mode*/
		unsigned int rd_continue	:  1;   /* Continue DMA operation */
		unsigned int rd_start		:  1;	/* Start DMA transfer */
#endif
	} bits;
} GMAC_RXDMA_CTRL_T;


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
} GMAC_RXDMA_FIRST_DESC_T;

typedef union
{
	unsigned int bits32;
	struct bit2_ff1c
	{
#if (BIG_ENDIAN==1)
		unsigned int ndar			: 28;	/* next descriptor address */
		unsigned int eofie			:  1;	/* end of frame interrupt enable */
		unsigned int    			:  1;
		unsigned int sof_eof		:  2;
#else
		unsigned int sof_eof		:  2;
		unsigned int    			:  1;
		unsigned int eofie			:  1;	/* end of frame interrupt enable */
		unsigned int ndar			: 28;	/* next descriptor address */
#endif
	} bits;
} GMAC_RXDMA_CURR_DESC_T;


/********************************************/
/*          Descriptor Format               */
/********************************************/

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
			unsigned int csum_state : 3;	/* checksum error status */
			unsigned int vlan_tag   : 1;    /* 802.1q vlan tag packet */
			unsigned int frame_state: 3;    /* reference Rx Status1 */
			unsigned int desc_count : 6;	/* number of descriptors used for the current frame */
			unsigned int buffer_size:16;	/* transfer buffer size associated with current description*/
#else
			unsigned int buffer_size:16;	/* transfer buffer size associated with current description*/
			unsigned int desc_count : 6;	/* number of descriptors used for the current frame */
			unsigned int frame_state: 3;    /* reference Rx Status1 */
			unsigned int vlan_tag   : 1;    /* 802.1q vlan tag packet */
			unsigned int csum_state : 3;	/* checksum error status */
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
			unsigned int            : 6;
			unsigned int success_tx : 1;    /* successful transmitted */
			unsigned int desc_count : 6;	/* number of descriptors used for the current frame */
			unsigned int buffer_size:16;	/* transfer buffer size associated with current description*/
#else
			unsigned int buffer_size:16;	/* transfer buffer size associated with current description*/
			unsigned int desc_count : 6;	/* number of descriptors used for the current frame */
			unsigned int success_tx : 1;    /* successful transmitted */
			unsigned int            : 6;
			unsigned int perr		: 1;	/* protocol error during processing this descriptor */
			unsigned int derr		: 1;	/* data error during processing this descriptor */
			unsigned int own 		: 1;	/* owner bit. 0-CPU, 1-DMA */
#endif
        } bits_tx_in;

		struct bits_0002
		{
#if (BIG_ENDIAN==1)
			unsigned int own 		: 1;	/* owner bit. 0-CPU, 1-DMA */
			unsigned int derr		: 1;	/* data error during processing this descriptor */
			unsigned int perr		: 1;	/* protocol error during processing this descriptor */
			unsigned int            : 2;
			unsigned int udp_csum_en: 1;    /* TSS UDP checksum enable */
			unsigned int tcp_csum_en: 1;    /* TSS TCP checksum enable */
			unsigned int ipv6_tx_en : 1;    /* TSS IPv6 TX enable */
			unsigned int ip_csum_en : 1;    /* TSS IPv4 IP Header checksum enable */
			unsigned int vlan_enable: 1;    /* VLAN TIC insertion enable */
			unsigned int desc_count : 6;	/* number of descriptors used for the current frame */
			unsigned int buffer_size:16;	/* transfer buffer size associated with current description*/
#else
			unsigned int buffer_size:16;	/* transfer buffer size associated with current description*/
			unsigned int desc_count : 6;	/* number of descriptors used for the current frame */
			unsigned int vlan_enable: 1;    /* VLAN TIC insertion enable */
			unsigned int ip_csum_en : 1;    /* TSS IPv4 IP Header checksum enable */
			unsigned int ipv6_tx_en : 1;    /* TSS IPv6 TX enable */
			unsigned int tcp_csum_en: 1;    /* TSS TCP checksum enable */
			unsigned int udp_csum_en: 1;    /* TSS UDP checksum enable */
			unsigned int            : 2;
			unsigned int perr		: 1;	/* protocol error during processing this descriptor */
			unsigned int derr		: 1;	/* data error during processing this descriptor */
			unsigned int own 		: 1;	/* owner bit. 0-CPU, 1-DMA */
#endif
        } bits_tx_out;

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
			unsigned int frame_count:16;    /* total tx frame byte count */
#else
			unsigned int frame_count:16;    /* total tx frame byte count */
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
} GMAC_DESCRIPTOR_T;

typedef struct gmac_conf {
	struct net_device *dev;
	int portmap;
	int vid;
	int flag;     /* 1: active  0: non-active */
} sys_gmac_conf;
		 
typedef struct gmac_private {
	unsigned char       *tx_bufs;	/* Tx bounce buffer region. */
	unsigned char       *rx_bufs;
	GMAC_DESCRIPTOR_T	*tx_desc;	/* point to virtual TX descriptor address*/
	GMAC_DESCRIPTOR_T	*rx_desc;	/* point to virtual RX descriptor address*/
	GMAC_DESCRIPTOR_T	*tx_cur_desc;	/* point to current TX descriptor */
	GMAC_DESCRIPTOR_T	*rx_cur_desc;	/* point to current RX descriptor */
	GMAC_DESCRIPTOR_T   *tx_finished_desc;
	GMAC_DESCRIPTOR_T   *rx_finished_desc;
	unsigned long       cur_tx;
	unsigned int        cur_rx;	/* Index into the Rx buffer of next Rx pkt. */
	unsigned int        tx_flag;
	unsigned long       dirty_tx;
	unsigned char       *tx_buf[TX_DESC_NUM];	/* Tx bounce buffers */
	dma_addr_t          tx_desc_dma; /* physical TX descriptor address */
	dma_addr_t          rx_desc_dma;	/* physical RX descriptor address */
	dma_addr_t          tx_bufs_dma; /* physical TX descriptor address */
	dma_addr_t          rx_bufs_dma; /* physical RX descriptor address */
//    struct net_device_stats  stats;
//    spinlock_t          lock;
} GMAC_INFO_T;;


struct reg_ioctl_data {
    unsigned int    reg_addr;   /* the register address */
    unsigned int    val_in;     /* data write to the register */
    unsigned int    val_out;    /* data read from the register */
};

#endif //_GMAC_SL2312_H

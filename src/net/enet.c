/****************************************************************************
* Copyright  Storlink Corp 2005.  All rights reserved.                
*--------------------------------------------------------------------------
* Name			: enet.c
* Description	: 
*		Handle Ethernet input and output function by polling
*
* History
*
*	Date		Writer		Description
*	-----------	-----------	-------------------------------------------------
*	04/25/2005	Gary Chen	Create
*
****************************************************************************/
#include <define.h>
#include <board_config.h>
#include <net.h>

static int enet_initialized = 0;
extern unsigned int gmac_num;
#ifdef MIDWAY
#if defined(LEPUS_FPGA) || defined(LEPUS_ASIC)
#define emac_sl2312_init		toe_gmac_sl2312_init
#define sl2312_emac_deliver		toe_gmac_deliver
#define emac_sl2312_can_send	toe_gmac_can_send
#define emac_sl2312_send		toe_gmac_send
#else
#define emac_sl2312_init		gmac_sl2312_init
#define sl2312_emac_deliver		sl2312_gmac_deliver
#define emac_sl2312_can_send	gmac_sl2312_can_send
#define emac_sl2312_send		gmac_sl2312_send
#endif
#endif

/*----------------------------------------------------------------------
* enet_init
*----------------------------------------------------------------------*/
void enet_init(void)
{
	enet_initialized =0;
	if (!enet_initialized)
	{
		enet_initialized = 1;
		emac_sl2312_init();
#ifdef BOARD_SUPPORT_AOE
		aoe_init();
#endif
	}
}

/*----------------------------------------------------------------------
* enet_input
*	called by ethernet driver
*	queue rcv pkt into network buffer
*----------------------------------------------------------------------*/
void enet_input(char *srcep, int size)
{
	NETBUF_HDR_T *netbuf_p;
	
	if (size > NETBUF_SIZE || size < ETH_HDR_SIZE)
		return;
		
	if ((netbuf_p = (NETBUF_HDR_T *)net_alloc_buf()))
	{
		memcpy(netbuf_p->datap, srcep, size);
		netbuf_p->len = size;
		net_put_rcvq(netbuf_p);
	}
	else
	{
		dbg_printf(("No free network buffer!\n"));
	}
}

/*----------------------------------------------------------------------
* dump_data
*----------------------------------------------------------------------*/
void dump_data(unsigned char *datap, int len)
{
	while(len--)
	{
		printf(" %02x", *datap++);
	}
	printf("\n");
}

/*----------------------------------------------------------------------
* enet_poll
*----------------------------------------------------------------------*/
void enet_poll(void)
{
	eth_header_t *eth_hdr;
	NETBUF_HDR_T *netbuf_p;
	unsigned short eth_type;
	
	if (!enet_initialized)
		return;
		
	// emac_sl2312_poll();
	sl2312_emac_deliver();
	
	while (net_peek_rcvq())
	{
		if (!(netbuf_p = (NETBUF_HDR_T *)net_get_rcvq()))
		{
			dbg_printf(("Fatal error! for enet_poll()\n"));
			return;
		}
		eth_hdr = (eth_header_t *)netbuf_p->datap;
		eth_type = ntohs(eth_hdr->type);
		if (eth_type == ETH_TYPE_IP)
		{
			ip_input(netbuf_p);
		}
		else if (eth_type == ETH_TYPE_ARP)
		{
			arp_input(netbuf_p);
		}
#ifdef BOARD_SUPPORT_AOE
		else if (eth_type == ETH_TYPE_AOE)
		{
			aoe_input(netbuf_p);
		}
#endif		
		else
		{
			// dump_data((unsigned char *)eth_hdr, ETH_HDR_SIZE);
			net_free_buf((void *)netbuf_p);
		}
	}
	
}

/*----------------------------------------------------------------------
* enet_send
*----------------------------------------------------------------------*/
void enet_send(NETBUF_HDR_T *netbuf_p, unsigned char *dest, unsigned short ether_type)
{
    eth_header_t *eth_hdr;
    
    if (emac_sl2312_can_send())
    {
		eth_hdr = (eth_header_t *)netbuf_p->datap;
		if(gmac_num)
			memcpy(eth_hdr->sa, sys_get_mac_addr(1), ETH_MAC_SIZE);
		else
			memcpy(eth_hdr->sa, sys_get_mac_addr(0), ETH_MAC_SIZE);
    	
    	memcpy(eth_hdr->da, dest, ETH_MAC_SIZE);
    	eth_hdr->type = htons(ether_type);
    	emac_sl2312_send(eth_hdr, netbuf_p->len + ETH_HDR_SIZE);
    }
	
	net_free_buf((void *)netbuf_p);
}

/*----------------------------------------------------------------------
* enet_cli_poll
*	A CLI command to test enet polling function on console
*----------------------------------------------------------------------*/
#if 0
void enet_cli_poll(char argc, char *argv[])
{
	char key;
	
	do
	{
		enet_poll();
		uart_scanc(&key);
	} while (uart_scanc(&key)==0 || key!=0x03);
}
#endif

/****************************************************************************
* Copyright  Storlink Corp 2005.  All rights reserved.                
*--------------------------------------------------------------------------
* Name			: arp.c
* Description	: 
*		Handle ARP request & response
*
* History
*
*	Date		Writer		Description
*	-----------	-----------	-------------------------------------------------
*	04/26/2005	Gary Chen	Create
*
****************************************************************************/
#include <define.h>
#include <net.h>

void (*arp_listen_handler)(NETBUF_HDR_T *netbuf_p, arp_header_t *arp) = NULL;

static ARP_ENTRY_T *arp_chk_entry(UINT32 ipaddr);
ARP_ENTRY_T *arp_lookup(UINT32 ipaddr);;

static ARP_ENTRY_T arp_cache_tbl[ARP_MAX_CACHE_NUM];
static int arp_cache_num = 0;
extern unsigned int gmac_num;

/*----------------------------------------------------------------------
* arp_input
*	Handle receiving ARP packets
*	(1) ARP Request
*		Send reply frame if request for me
*	(2) ARP Response
*		If no waiting service, discard it
*----------------------------------------------------------------------*/
void arp_input(NETBUF_HDR_T *netbuf_p)
{
    arp_header_t	*arp;
    int				hw_type;
    int				protocol;
    UINT32			my_ip_addr;

	arp =(arp_header_t *)(netbuf_p->datap + ETH_HDR_SIZE);
	
	if (netbuf_p->len < ARP_PKT_SIZE)
	{
		goto arp_input_end;
	}
		
    protocol = ntohs(arp->protocol);
    hw_type = ntohs(arp->hw_type);
    if ((hw_type != ARP_HW_ETHER) || (protocol != ETH_TYPE_IP))
    {
		goto arp_input_end;
    }

    my_ip_addr = sys_get_ip_addr();
    my_ip_addr = htonl(my_ip_addr);
    
	if (arp->target_ip == my_ip_addr)
	{
		switch (ntohs(arp->opcode))
		{
			case ARP_REQUEST:
				arp->opcode = htons(ARP_REPLY);
				arp->target_ip = arp->sender_ip;
				memcpy(arp->target_mac, arp->sender_mac, ETH_MAC_SIZE);
				arp->sender_ip = my_ip_addr;
				if(gmac_num)
					memcpy(arp->sender_mac, sys_get_mac_addr(1), ETH_MAC_SIZE);
				else
					memcpy(arp->sender_mac, sys_get_mac_addr(0), ETH_MAC_SIZE);
				netbuf_p->len = sizeof(arp_header_t);
				enet_send(netbuf_p, (unsigned char *)&arp->target_mac, ETH_TYPE_ARP);
				return;
			case ARP_REPLY:
				if (arp_listen_handler)
				{
					(*arp_listen_handler)(netbuf_p, arp);
					return;
		    	}
		    }
	}
	
arp_input_end:	
	net_free_buf((void *)netbuf_p);
	return;
}

/*----------------------------------------------------------------------
* arp_reply_listener
*----------------------------------------------------------------------*/
static void arp_reply_listener(NETBUF_HDR_T *netbuf_p, arp_header_t *arp)
{
	int i;
	ARP_ENTRY_T *entry, *free_p;
	UINT32	ipaddr;
	
	ipaddr = arp->sender_ip;
	ipaddr = ntohl(ipaddr);
	
	// check same address
	entry = (ARP_ENTRY_T *)&arp_cache_tbl[0];
	free_p = NULL;
	for (i=0; i<ARP_MAX_CACHE_NUM; i++, entry++)
	{
		if (entry->used)
		{
			if (ipaddr == entry->ip_addr)
			{
				memcpy(entry->mac_addr, arp->sender_mac, ETH_MAC_SIZE);
				net_free_buf((void *)netbuf_p);
				return;
			}
		}
		else if (free_p == NULL)
		{
			free_p = entry;
		}
	}
	
	if (free_p)
	{
		free_p->used = 1;
		free_p->ip_addr = ipaddr;
		memcpy(free_p->mac_addr, arp->sender_mac, ETH_MAC_SIZE);
		arp_cache_num++;
	}
	
	net_free_buf((void *)netbuf_p);
}

/*----------------------------------------------------------------------
* arp_request
*----------------------------------------------------------------------*/
ARP_ENTRY_T *arp_request(UINT32 ipaddr)
{
	NETBUF_HDR_T	*netbuf_p;
    arp_header_t	*arp;
    unsigned long	retry_start;
    unsigned char	bcast_addr[ETH_MAC_SIZE];
    UINT32			my_ip_addr;
    int				retry;
    ARP_ENTRY_T		*entry;
    UINT32			ipaddr_n;
	UINT64			delay_time;
	UINT32			delay_ticks;
	
	my_ip_addr = sys_get_ip_addr();
	my_ip_addr = htonl(my_ip_addr);
	ipaddr_n = htonl(ipaddr);

	bcast_addr[0] = 255;
	bcast_addr[1] = 255;
	bcast_addr[2] = 255;
	bcast_addr[3] = 255;
	bcast_addr[4] = 255;
	bcast_addr[5] = 255;
	
    retry = 3;
    while (retry-- > 0)
    {
		if (!(netbuf_p = (NETBUF_HDR_T *)net_alloc_buf()))
			return NULL;

		arp =(arp_header_t *)(netbuf_p->datap + ETH_HDR_SIZE);
    	arp->opcode = htons(ARP_REQUEST);
    	arp->hw_type = htons(ARP_HW_ETHER);
		arp->protocol = htons(0x800);
    	arp->hw_len = ETH_MAC_SIZE;
    	arp->proto_len = sizeof(UINT32);

    	arp->sender_ip = my_ip_addr;
    	if(gmac_num)
    		memcpy(arp->sender_mac, sys_get_mac_addr(1), ETH_MAC_SIZE);
    	else
    		memcpy(arp->sender_mac, sys_get_mac_addr(0), ETH_MAC_SIZE);
    		
    	arp->target_ip = ipaddr_n;
    	memset(arp->target_mac, 0, ETH_MAC_SIZE);

	
		arp_listen_handler = (void *)&arp_reply_listener;
        /* send the packet */
		netbuf_p->len = sizeof(arp_header_t);
        enet_send(netbuf_p, (unsigned char *)&bcast_addr, ETH_TYPE_ARP);
        
		delay_ticks = (1 * BOARD_TPS);
		delay_time = sys_get_ticks() + delay_ticks;
        
		// polling about 1s
		while (sys_get_ticks() < delay_time)
		{
			enet_poll();
			if ((entry = arp_chk_entry(ipaddr)) != NULL)
			{
				arp_listen_handler = NULL;
				return entry;
			}
		}
		arp_listen_handler = NULL;
    }
	return NULL;
}

/*----------------------------------------------------------------------
* arp_chk_entry
*	input: 
*		UINT32 *ipaddr
*----------------------------------------------------------------------*/
static ARP_ENTRY_T *arp_chk_entry(UINT32 ipaddr)
{
	int i;
	ARP_ENTRY_T *entry;
	
	entry = (ARP_ENTRY_T *)&arp_cache_tbl[0];
	for (i=0; i<ARP_MAX_CACHE_NUM; i++, entry++)
	{
		if (entry->used)
		{
			if (entry->ip_addr == ipaddr)
				return entry;
		}
	}
	return NULL;
}

/*----------------------------------------------------------------------
* arp_flush_cache
*	clear ARP cache table
*----------------------------------------------------------------------*/
void arp_flush_cache(void)
{
	int i;
	ARP_ENTRY_T *entry;
	
	entry = (ARP_ENTRY_T *)&arp_cache_tbl[0];
	for (i=0; i<ARP_MAX_CACHE_NUM; i++, entry++)
		entry->used = 0;
	arp_cache_num = 0;
}

/*----------------------------------------------------------------------
* arp_lookup
*	input: 
*		UINT32 *ipaddr
*----------------------------------------------------------------------*/
ARP_ENTRY_T *arp_lookup(UINT32 ipaddr)
{
	int i;
	ARP_ENTRY_T *entry;
	
	if ((entry = arp_chk_entry(ipaddr)) != NULL)
		return entry;
	
	if (arp_cache_num == ARP_MAX_CACHE_NUM)
		arp_flush_cache();
		
	// not found, Issue ARP request
	if (!ip_is_local_net(ipaddr))
		ipaddr = sys_get_ip_gateway();
	
    return arp_request(ipaddr);
}

/*----------------------------------------------------------------------
* arp_show_cache
*	show ARP table
*----------------------------------------------------------------------*/
void arp_show_cache(char argc, char *argv[])
{
	int i, total=0;
	ARP_ENTRY_T *entry;
	char *mac;
	
	entry = (ARP_ENTRY_T *)&arp_cache_tbl[0];
	for (i=0; i<ARP_MAX_CACHE_NUM; i++, entry++)
	{
		if (entry->used)
		{
			total++;
			mac = (char *)entry->mac_addr;
			printf("%d.%d.%d.%d  %02X:%02X:%02X:%02X:%02X:%02X\n",
					IP1(entry->ip_addr), IP2(entry->ip_addr), IP3(entry->ip_addr),
					IP4(entry->ip_addr),
					mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
		}
	}
	if (!total)
		printf("Empty\n");
}


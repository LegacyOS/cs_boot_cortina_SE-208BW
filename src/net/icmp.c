/****************************************************************************
* Copyright  Storlink Corp 2005.  All rights reserved.                
*--------------------------------------------------------------------------
* Name			: icmp.c
* Description	: 
*		Handle ICMP input and output functions
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
#include "chksum.h"

static void (*icmp_listen_handler)(NETBUF_HDR_T *netbuf_p, void *arg) = NULL;
static void  *icmp_listen_arg;

static int icmp_ident = 0x1;
/*----------------------------------------------------------------------
* icmp_input
*	Handle receiving IP packets
*	(1) ICMP
*	(2) UDP
*----------------------------------------------------------------------*/
void icmp_input(NETBUF_HDR_T *netbuf_p)
{
	eth_header_t	*eth_hdr;
    ip_header_t		*iph;
    icmp_header_t 	*icmp;
    unsigned short	cksum;
    ARP_ENTRY_T		arp;
    int 			len;

	eth_hdr = (eth_header_t *)netbuf_p->datap;
	iph =(ip_header_t *)(netbuf_p->datap + ETH_HDR_SIZE);
    icmp = (icmp_header_t *)(netbuf_p->datap + ETH_HDR_SIZE + IP_HDR_SIZE);
    len = netbuf_p->len - ETH_HDR_SIZE - IP_HDR_SIZE;
    
	if (icmp->type == ICMP_TYPE_ECHOREQUEST	&& icmp->code == 0)
	{
		if (ip_csum((u16 *)icmp, len, 0) != 0)
		{
			// printf("ICMP chksum is bad!\n");
			goto icmp_input_end;
		}
		
		icmp->type = ICMP_TYPE_ECHOREPLY;
		icmp->chksum = 0;
        cksum = ip_csum((u16 *)icmp, len, 0);
		icmp->chksum = htons(cksum);
		arp.ip_addr = ntohl(iph->saddr);
    	memcpy(arp.mac_addr, eth_hdr->sa, ETH_MAC_SIZE);
    	netbuf_p->len = len;
        ip_output(netbuf_p, arp.ip_addr, IP_PROTO_ICMP, &arp);
        return;
	} 
	else if (icmp->type == ICMP_TYPE_ECHOREPLY)
	{
		if (icmp_listen_handler)
		{
			(*icmp_listen_handler)(netbuf_p, icmp_listen_arg);
			return;
		}
    }

icmp_input_end:	
	net_free_buf((void *)netbuf_p);
	return;
}

/*----------------------------------------------------------------------
* icmp_register_listener
*----------------------------------------------------------------------*/
static void icmp_register_listener(void *routine, void *arg)
{
	icmp_listen_handler = routine;
	icmp_listen_arg = arg;
}

/*----------------------------------------------------------------------
* icmp_deregister_listener
*----------------------------------------------------------------------*/
static void icmp_deregister_listener(void)
{
	icmp_listen_handler = NULL;
}

/*----------------------------------------------------------------------
* icmp_ping_listener
*	input:
*			*argv is the pkt id to receive
*
*	Output:
*			*argv is NULL is OK
*			-1 is error	
*----------------------------------------------------------------------*/
static void icmp_ping_listener(NETBUF_HDR_T *netbuf_p, void *arg)
{
    icmp_header_t	*icmp;
    int 			id;
    
    id = *(int *)arg;
	
	icmp =(icmp_header_t *)(netbuf_p->datap + ETH_HDR_SIZE + IP_HDR_SIZE);
	if (id != ntohs(icmp->ident))
	{
		*(int *)arg = -1;
		net_free_buf((void *)netbuf_p);
		return;
	}
	
	*(int *)arg = 0;
	net_free_buf((void *)netbuf_p);
	return;
}

/*----------------------------------------------------------------------
* icmp_ping
*----------------------------------------------------------------------*/
void icmp_ping(UINT32 host, UINT32 count, UINT32 size, UINT32 timeout_ms)
{
	ARP_ENTRY_T		*arp;
	NETBUF_HDR_T	*netbuf_p;
    icmp_header_t	*icmp;
    ip_header_t		*ip;
    int 			num;
    unsigned short	cksum;
    unsigned char	*datap;
    int				i, arg;
    int				wait_time;
	char			key;
	UINT64			delay_time, start_time, response_time;
	UINT32			delay_ticks;
	
	arp_flush_cache();
	
	if ((arp = (ARP_ENTRY_T *)arp_lookup(host)) == NULL)
	{
		printf("Host (%d.%d.%d.%d) is not found!\n",
				IP1(host), IP2(host), IP3(host), IP4(host));
		return;
	}
#if 0 // caller must take care all the input arguments
	if (size < ICMP_MIN_DATA_SIZE || size > ICMP_MAX_DATA_SIZE)
	{
		printf("Illegal size (%d)! Must be from %d to %d\n",
				size, ICMP_MIN_DATA_SIZE, ICMP_MAX_DATA_SIZE);
		return;
	}
#endif	
	
    for (num = 0;  !count || num < count;  num++)
    {
   		//i=0;
    	//while(((arp = (ARP_ENTRY_T *)arp_lookup(host)) == NULL)&&(i<5))
    	if ((arp = (ARP_ENTRY_T *)arp_lookup(host)) == NULL)
		{
			printf("Host (%d.%d.%d.%d) is not found!\n",
					IP1(host), IP2(host), IP3(host), IP4(host));
			//		i++;
			
		}
		//if(i==5)
		//	arp_flush_cache();
	
        if (uart_scanc(&key) && (key==0x1b || key==0x3))
        	 break;
        
		if (!(netbuf_p = (NETBUF_HDR_T *)net_alloc_buf()))
		{
			dbg_printf(("No free net buffer!\n"));
			break;
		}
	
		ip =(ip_header_t *)(netbuf_p->datap + ETH_HDR_SIZE);
		icmp =(icmp_header_t *)(netbuf_p->datap + ETH_HDR_SIZE + IP_HDR_SIZE);
	
    	netbuf_p->len = size + ICMP_HDR_SIZE;
		icmp->type = ICMP_TYPE_ECHOREQUEST;
		icmp->code = 0;
		icmp->chksum = 0;
		icmp->ident = htons(icmp_ident);
		icmp->seqnum = htons(num + 1);

        datap = (unsigned char *)(icmp + 1);
        for (i = 0;  i < size;  i++)
            *datap++ = 'a' + i;
        
		cksum = ip_csum((u16 *)icmp, netbuf_p->len, 0);
		icmp->chksum = htons(cksum);
		
		arg = icmp_ident;
		
		icmp_register_listener((void *)&icmp_ping_listener, (void *)&arg);
        
        start_time = sys_get_ticks();
		delay_ticks = (timeout_ms * BOARD_TPS) /1000;
		if (delay_ticks == 0) delay_ticks = 1;
		delay_time = start_time + delay_ticks;
        
        ip_output(netbuf_p, host, IP_PROTO_ICMP, arp);
        while (sys_get_ticks() < delay_time)
        {
			enet_poll();
			if (arg != icmp_ident)
			{
        		response_time = sys_get_ticks();
				break;
			}
        }
        
        if (arg == icmp_ident)
        	printf("Time-out!\n");
        else if (arg != 0)
        	printf("Error! got incorrect data!\n");
        else
        {
        	int time = (response_time - start_time);
        	time = (time * 1000) / BOARD_TPS;
    		printf("%d Reply from %d.%d.%d.%d, bytes=%d ", 
    				num + 1, IP1(host), IP2(host), IP3(host), IP4(host), size);
    				
    		if (time)
    			printf("time=%dms\n", time);
    		else
    			printf("time<10ms\n");
    	}
    	icmp_deregister_listener();
		icmp_ident++;
	}
}


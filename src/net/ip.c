/****************************************************************************
* Copyright  Storlink Corp 2005.  All rights reserved.                
*--------------------------------------------------------------------------
* Name			: ip.c
* Description	: 
*		Handle IP input and output functions
*
* History
*
*	Date		Writer		Description
*	-----------	-----------	-------------------------------------------------
*	04/25/2005	Gary Chen	Create
*
****************************************************************************/
#include <define.h>
#include <net.h>
#include "chksum.h"

static u16 ip_ident;
extern int web_on;

/*----------------------------------------------------------------------
* ip_input
*	Handle receiving IP packets
*	(1) ICMP
*	(2) UDP
*----------------------------------------------------------------------*/
void ip_input(NETBUF_HDR_T *netbuf_p)
{
    ip_header_t *iph;
    int         hdr_bytes;
    UINT32 		my_ip_addr;

	iph =(ip_header_t *)(netbuf_p->datap + ETH_HDR_SIZE);

	if (iph->ihl < 5 || iph->version != 4)
		goto ip_input_end; 
		
	if (netbuf_p->len < IP_PKT_SIZE)
		goto ip_input_end;

    my_ip_addr = sys_get_ip_addr();
    my_ip_addr = htonl(my_ip_addr);
    
	// only accept broadcast & my ip
	if (iph->daddr != 0xffffffff && iph->daddr != my_ip_addr)
		goto ip_input_end;

	// Not handle fragment packet
	if (iph->frag_off & htons(IP_MF))
	{
		// printf("Discard segement frame\n");
		goto ip_input_end;
	}
	
	if (ip_csum((u16 *)iph, iph->ihl << 2, 0) != 0)
		goto ip_input_end;

#if 0		
	if (ip_csum_asm((u8 *)iph, iph->ihl) != 0)
	{
		// printf ("ip_csum is Error\n");
		goto ip_input_end;
	}
#endif

    switch (iph->protocol)
    {
		case IP_PROTO_ICMP:
			icmp_input(netbuf_p);
			return;
#ifdef BOARD_SUPPORT_WEB		
		case IP_PROTO_TCP:
			if(web_on == 1)
				tcp_input(netbuf_p);
			return;
#endif
		case IP_PROTO_UDP:
			udp_input(netbuf_p);
			return;
		default:
			net_free_buf((void *)netbuf_p);
			return;
    }

ip_input_end:	
	net_free_buf((void *)netbuf_p);
	return;
}

/*----------------------------------------------------------------------
* ip_output
*	Handle receiving IP packets
*	(1) ICMP
*	(2) UDP
*----------------------------------------------------------------------*/
void ip_output(NETBUF_HDR_T *netbuf_p, UINT32 host, int protocol, ARP_ENTRY_T *arp)
{
    ip_header_t 	*iph;
    int				hdr_bytes;
    unsigned short	cksum;
	UINT32			my_ip_addr;
   
	iph =(ip_header_t *)(netbuf_p->datap + ETH_HDR_SIZE);

    netbuf_p->len += IP_HDR_SIZE;

    iph->version = 4;
    iph->ihl = IP_HDR_SIZE >> 2;
    iph->tos = 0;
    iph->tot_len = htons(netbuf_p->len);
    iph->id = htons(ip_ident);
    ip_ident++;
    iph->frag_off = 0;
    iph->ttl = 64;
    iph->protocol = protocol;
    iph->chksum = 0;
    my_ip_addr = sys_get_ip_addr();
    my_ip_addr = htonl(my_ip_addr);
    iph->saddr = my_ip_addr;
    iph->daddr = htonl(host);
    // iph->daddr = htonl(arp->ip_addr);
    cksum = ip_csum((u16 *)iph, IP_HDR_SIZE, 0);
    iph->chksum = htons(cksum);

    enet_send(netbuf_p, (unsigned char *)arp->mac_addr, ETH_TYPE_IP);    
}

/*----------------------------------------------------------------------
* 	ip_verify_addr
*----------------------------------------------------------------------*/
int ip_verify_addr(UINT32 ipaddr)
{
    UINT32  net_id;
    UINT32  host_id;
    
    if ((ipaddr & 0x000000ff) == 0)
    	return -1;
    	
    if ((ipaddr & 0x80000000UL) == 0)         /* class A and loopback */
    {
        net_id = ipaddr & 0x7f000000UL;
        host_id = ipaddr & 0x00ffffffUL;
        
        if (net_id == 0x7f000000UL || net_id == 0 ||
            host_id == 0x00ffffffUL || host_id == 0)
        {
            return -1;
        }
    }
    else if ((ipaddr & 0xc0000000UL) == 0x80000000UL)   /* class B */
    {
        net_id = ipaddr & 0x3fff0000UL;
        host_id = ipaddr & 0x0000ffffUL;
        
        if (net_id == 0 ||
            host_id == 0x0000ffffUL || host_id == 0)
        {
            return -1;
        }
    }
    else if ((ipaddr & 0xe0000000UL) == 0xc0000000UL)   /* class C */
    {
        net_id = ipaddr & 0x1fffff00UL;
        host_id = ipaddr & 0x000000ffUL;
        
        if (net_id == 0 ||
            host_id == 0x000000ffUL || host_id == 0)
        {
            return -1;
        }
    }
    else  
    {
        return -1;   
    }
    
    return 0;
}

/*----------------------------------------------------------------------
* ip_verify_netmask
*----------------------------------------------------------------------*/
int ip_verify_netmask(UINT32 netmask)
{
    UINT32 i;
    UINT32 j=0xffffffffUL;
    
    if (netmask == j) 
    	return -1;
    	
    for (i = 0; i <= 32; i++)
    {
        if (netmask == j)
        {
            return 0;
        }
        j <<= 1;
    }   
    return -1;
}

/*----------------------------------------------------------------------
* ip_is_local_net
*----------------------------------------------------------------------*/
int ip_is_local_net(UINT32 addr)
{
	UINT32 my_ip_addr = sys_get_ip_addr();
	UINT32 my_ip_netmask = sys_get_ip_netmask();
	
	return !((my_ip_addr ^ addr) & my_ip_netmask);
}

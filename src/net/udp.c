/****************************************************************************
* Copyright  Storlink Corp 2005.  All rights reserved.                
*--------------------------------------------------------------------------
* Name			: udp.c
* Description	: 
*		Handle UDP input and output functions
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

UDP_SOCK_T udp_socks[UDP_SOCKET_MAX_NUM];

static int udp_pseudo_sum(ip_header_t *ip);

#ifdef RECOVER_FROM_BOOTP
//bootp
extern bool use_bootp;
extern bootp_header_t *bp_info;
extern UINT32 __local_ip_addr;
extern unsigned char my_mac_addr[0][6];	
extern ip_route_t     r;
#endif
/*----------------------------------------------------------------------
* udp_init
*	Initialize UDP data, i.e. socket-like interface
*----------------------------------------------------------------------*/
int udp_init(void)
{
	int i;
	UDP_SOCK_T *sock;
	
	sock = (UDP_SOCK_T *)&udp_socks[0];
	for (i=0; i<UDP_SOCKET_MAX_NUM; i++, sock++)
	{
		memset(sock, 0, sizeof(UDP_SOCK_T));
	}
}

/*----------------------------------------------------------------------
* udp_input
*	Receive a UDP frame
*----------------------------------------------------------------------*/
int udp_input(NETBUF_HDR_T *netbuf_p)
{
	// eth_header_t	*eth_hdr;
    ip_header_t		*iph;
	udp_header_t 	*udp;
	int				len;
	int				i;
	UDP_SOCK_T		*sock;
    ARP_ENTRY_T		arp;
    char			*data_p;
	
	// eth_hdr = (eth_header_t *)netbuf_p->datap;
	iph =(ip_header_t *)(netbuf_p->datap + ETH_HDR_SIZE);
	udp = (udp_header_t *)(netbuf_p->datap + ETH_HDR_SIZE + IP_HDR_SIZE);
    data_p = (char *)(udp+1);
    
	len = htons(udp->length) - UDP_HDR_SIZE;
	
    if (udp->chksum == 0xffff)
		udp->chksum = 0;

    iph->tot_len = udp->length;
    
    if (ip_csum((u16 *)udp, ntohs(udp->length), udp_pseudo_sum(iph)) != 0)
		goto udp_input_end;
		
	sock = (UDP_SOCK_T *)&udp_socks[0];
	for (i=0; i<UDP_SOCKET_MAX_NUM; i++, sock++)
	{
#ifdef RECOVER_FROM_BOOTP
		///bootp
		if (use_bootp)
		{
			bootp_header_t *b;
			if ((htons(IPPORT_BOOTPC) == udp->dport) && (htons(IPPORT_BOOTPS) == udp->sport))
			{
					
 	
    			b = (bootp_header_t *)(((char *)udp) + sizeof(udp_header_t));
    			if (bp_info) {
    			    memcpy(bp_info, b, len);
    			}
   			
    			if (b->bp_op == BOOTREPLY && !memcmp(b->bp_chaddr, (char *)&my_mac_addr[0][0], 6))
				memcpy(__local_ip_addr, &b->bp_yiaddr, 4);
  			sock->data_size+= len;
			r.ip_addr[0] = IP4(iph->saddr);
			r.ip_addr[1] = IP3(iph->saddr);
			r.ip_addr[2] = IP2(iph->saddr);
			r.ip_addr[3] = IP1(iph->saddr);
    			memcpy(r.enet_addr, (*(((eth_header_t *)netbuf_p->datap))).sa, sizeof(enet_addr_t));
  					net_free_buf((void *)netbuf_p);
  				return;
			}
		}
		///bootp
#endif		
		if (sock->listen_port == udp->dport && sock->from_ip == iph->saddr
			&& ntohl(iph->daddr) == sys_get_ip_addr())
		{
			// copy data
			sock->from_port = ntohs(udp->sport);
			if (sock->buf_start_p)
			{
				if ((sock->data_size + len) > sock->buf_size)
					sock->error_code = UDP_ERROR_OVERFLOW;
				else
				{
					while (len)
					{
						*sock->buf_in_p++ = *data_p++;
						if (sock->buf_in_p >= sock->buf_end_p)
							sock->buf_in_p = sock->buf_start_p;
						len--;
						sock->data_size++;
					}
				}
			}
			else
			{
				sock->error_code = UDP_ERROR_NO_BUF;
			}
			net_free_buf((void *)netbuf_p);
			return;
		}
	}

udp_input_end:	
	net_free_buf((void *)netbuf_p);
	return;
}

/*----------------------------------------------------------------------
* udp_input
*	Receive a UDP frame
*----------------------------------------------------------------------*/
static int udp_pseudo_sum(ip_header_t *ip)
{
    int    sum;
    u16   *p;

    union {
		volatile unsigned char c[2];
		volatile unsigned short s;
    } su;
    
    p = (u16 *)&ip->saddr;
    sum  = *p++;
    sum += *p++;
    sum += *p++;
    sum += *p++;
    
    su.c[0] = 0;
    su.c[1] = ip->protocol;
    sum += su.s;

    sum += ip->tot_len;
    
    return sum;
}

/*----------------------------------------------------------------------
* udp_socket
*	i/p: Server: Server IP Address, host-oriented
*	Return -1 if no enough socket
*----------------------------------------------------------------------*/
int udp_socket(UINT32 server)
{
	int i;
	UDP_SOCK_T *sock;
	
	sock = (UDP_SOCK_T *)&udp_socks[0];
	for (i=0; i<UDP_SOCKET_MAX_NUM; i++, sock++)
	{
		if (!sock->used)
		{
			sock->used = 1;
#ifdef RECOVER_FROM_BOOTP
			if(use_bootp)
				sock->listen_port = IPPORT_BOOTPS;
			else
#endif
				sock->listen_port = 0;
			sock->buf_start_p = (char *)malloc(UDP_SOCK_BUF_SIZE);
			if (!sock->buf_start_p)
			{
				dbg_printf(("No Free memory\n"));
				return -1;
			}
			sock->from_ip = htonl(server);
			sock->buf_in_p = sock->buf_start_p;
			sock->buf_out_p = sock->buf_start_p;
			sock->buf_end_p  = sock->buf_start_p + UDP_SOCK_BUF_SIZE;
			sock->buf_size = UDP_SOCK_BUF_SIZE;
			sock->data_size = 0;
			sock->error_code = 0;
			return i;
		}
	}
	return -1;		
}

/*----------------------------------------------------------------------
* udp_close
*----------------------------------------------------------------------*/
int udp_close(int s)
{
	UDP_SOCK_T *sock;
	
	if (s < 0 || s >= UDP_SOCKET_MAX_NUM)
		return -1;

	sock = (UDP_SOCK_T *)&udp_socks[s];
	if (s >=0 && s < UDP_SOCKET_MAX_NUM)
	{		
		sock->used = 0;
		sock->listen_port = 0;
		if (sock->buf_start_p)
			free(sock->buf_start_p);
	}
	
	return 0;
}

/*----------------------------------------------------------------------
* udp_output
*	Return -1 if no enough socket
*----------------------------------------------------------------------*/
static int udp_output(char *datap, int len, UINT32 host, UINT16 sport, UINT16 dport, ARP_ENTRY_T *arp)
{
	NETBUF_HDR_T	*netbuf_p;
    udp_header_t	*udp;
    ip_header_t		*iph;
    UINT32			my_ip_addr;
    unsigned short	cksum;
	
	if (!(netbuf_p = (NETBUF_HDR_T *)net_alloc_buf()))
	{
		dbg_printf(("No free net buffer!\n"));
		return -1;
	}
	
	if (len > UDP_DATA_MAX_SIZE)
	{
		printf("UDP data size is too long!\n");
		net_free_buf((void *)netbuf_p);
		return -1;
	}
		
	netbuf_p->len = len + UDP_HDR_SIZE;
	iph =(ip_header_t *)(netbuf_p->datap + ETH_HDR_SIZE);
	udp =(udp_header_t *)(netbuf_p->datap + ETH_HDR_SIZE + IP_HDR_SIZE);
		
	udp->sport = htons(sport);
	udp->dport = htons(dport);
	udp->length = htons(netbuf_p->len);
	udp->chksum = 0;
	
	memcpy((char *)(udp + 1), datap, len);
	
#ifdef RECOVER_FROM_BOOTP
	if(use_bootp)
	{
		UINT32 server;
	
		//int ipaddr[4] = r.ip_addr;
		server = IPIV(r.ip_addr[0], r.ip_addr[1], r.ip_addr[2], r.ip_addr[3]);
		__local_ip_addr = 0;
		iph->saddr = htonl(__local_ip_addr);
		iph->daddr = htonl(server);
	}
	else
	{
#endif
		my_ip_addr = sys_get_ip_addr();
	
		iph->saddr = htonl(my_ip_addr);
	
		iph->daddr = htonl(arp->ip_addr);
#ifdef RECOVER_FROM_BOOTP
	}
#endif

    iph->protocol = IP_PROTO_UDP;
    iph->tot_len = udp->length;
	
    cksum = ip_csum((u16 *)udp, netbuf_p->len, udp_pseudo_sum(iph));
    udp->chksum = htons(cksum);

    ip_output(netbuf_p, host, IP_PROTO_UDP, arp);
    
    return 0;
}


/*----------------------------------------------------------------------
* udp_sendto
*	Return -1 if no enough socket
*----------------------------------------------------------------------*/
int udp_sendto(int s, char *datap, int len, UINT32 host, UINT16 sport, UINT16 dport)
{
	ARP_ENTRY_T *arp;

#ifdef RECOVER_FROM_BOOTP
	ARP_ENTRY_T b_arp;
	if(!use_bootp)	
	{
			if ((arp = (ARP_ENTRY_T *)arp_lookup(host)) == NULL)
			{
				printf("\nHost (%d.%d.%d.%d) is not found!\n", IP1(host), IP2(host), IP3(host), IP4(host));
				return -1;
			}
	}
	else{
		b_arp.ip_addr = 0xFFFFFFFF;
		memset(&b_arp.mac_addr,0xFF,6);
		b_arp.used = 1;
		arp = &b_arp;
	}
	
#else
	if ((arp = (ARP_ENTRY_T *)arp_lookup(host)) == NULL)
	{
		printf("\nHost (%d.%d.%d.%d) is not found!\n",
				IP1(host), IP2(host), IP3(host), IP4(host));
		return -1;
	}
#endif
	
	udp_output(datap, len, host, sport, dport, arp);
}

/*----------------------------------------------------------------------
* udp_recvfrom
*	Return -1 if no enough socket
*	return total size
*----------------------------------------------------------------------*/
int udp_recvfrom(int s, char *datap, UINT16 listen_port, UINT16 *server_port, int max_size, int timeout_ms)
{
	UDP_SOCK_T	*sock;
	UINT64		delay_time;
	UINT32		delay_ticks;
	int 		total;
	
	if (s < 0 || s >= UDP_SOCKET_MAX_NUM)
		return -1;
	
	delay_ticks = (timeout_ms * BOARD_TPS) /1000;
	if (delay_ticks == 0) delay_ticks = 1;
	delay_time = sys_get_ticks() + delay_ticks;
	
	sock = (UDP_SOCK_T *)&udp_socks[s];
	sock->listen_port = htons(listen_port);
	while (sock->data_size == 0)
	{
		enet_poll();
		if (sock->error_code < 0)
			return sock->error_code;
		if (sock->data_size)
			break;
		if (sys_get_ticks() > delay_time)
			break;
	}
	
	// copy data
#if 0	
	printf("Sock: buf_start_p=0x%x buf_end_p=0x%x, buf_out_p=0x%x, data_size=%d, max_size=%d\n",
			sock->buf_start_p, sock->buf_end_p, sock->buf_out_p, sock->data_size, max_size);
#endif

#ifdef RECOVER_FROM_BOOTP		
	if (use_bootp)
			total = max_size;//0;
	else
	{
#endif
		*server_port = sock->from_port;
		total =0;
		while(sock->data_size && max_size)
		{
			*datap++ = *sock->buf_out_p++;
			if (sock->buf_out_p >= sock->buf_end_p)
				sock->buf_out_p = sock->buf_start_p;
			total++;
			max_size--;
			sock->data_size--;
		}
#ifdef RECOVER_FROM_BOOTP
	}
#endif
	
#if 0	
	printf("Sock: buf_start_p=0x%x buf_end_p=0x%x, buf_out_p=0x%x, data_size=%d, max_size=%d\n\n",
			sock->buf_start_p, sock->buf_end_p, sock->buf_out_p, sock->data_size, max_size);
#endif
			
	return total;
}
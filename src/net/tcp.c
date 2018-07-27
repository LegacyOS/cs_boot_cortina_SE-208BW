/****************************************************************************
* Copyright  Storlink Corp 2005.  All rights reserved.                
*--------------------------------------------------------------------------
* Name			: tcp.c
* Description	: 
*		Handle TCP input and output functions
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

#ifdef BOARD_SUPPORT_WEB

extern TCP_SOCK_T tcp_socks[TCP_SOCKET_MAX_NUM];
static int tcp_pseudo_sum(ip_header_t *ip);
extern http_TCB http_t;
/* sequence number comparison macros */
#define SEQ_LT(a,b) ((int)((a)-(b)) < 0)
#define SEQ_LE(a,b) ((int)((a)-(b)) <= 0)
#define SEQ_GT(a,b) ((int)((a)-(b)) > 0)
#define SEQ_GE(a,b) ((int)((a)-(b)) >= 0)

#define OZDAY 100*60*60*24

/*
 * OZSPLIT.. is used as a bit of a fudge...
 * ..to handle midnight problems...
 */

#define OZSPLIT1 100*60*5


/* Extract the mss out of the TCP header */
void tcp_parse_options(TCP_SOCK_T *sock, tcp_header_t *tcph)
{
    u16  opt_temp;
    u8  *options;
    u16  numoptions;

 
    if ((numoptions = ( (tcph->hdr_len >> 2 ) - sizeof(tcp_header_t)))) {
        options = ((u8 *) tcph ) + sizeof(tcp_header_t);
        while (numoptions--) {
            switch (*options++) {
            case 0:             /* End of options */
                numoptions = 0;
            case 1:             /* NOP */
                break;
            case 2:             /* mss */
                if (*options == 4) {
                    opt_temp = htons(*(u16 *)(options+1) );
                    if (opt_temp < sock->mss)
                        sock->mss = opt_temp;
                }
            default:
                numoptions -= (*options - 1);
                options += (*options - 1);
            }
        }
    }
}

/*
 *	Set the timeout, entry in seconds, exit in
 *	oz units
 */
UINT64 set_ttimeout(int secs)
{
	return (sys_get_ticks()+((UINT64)(secs*BOARD_TPS)));
}

UINT64 set_timeout(int tensms)
{
	return (sys_get_ticks()+tensms);
}

/*
 * Check the timer supplied to see if it has timed out
 * Return TRUE for timeout
 */
#if 0
int chk_timeout(UINT64 time)
{
	UINT64 now;

	now=sys_get_ticks();

/*
 * 	Bit of fudge time
 */
	//if (time>OZDAY && now<OZSPLIT1) now+=OZDAY;


/*
 *	This will screw up big time over midnight...
 */
	if	(now > time ) return(1);
	return 0;
}
#endif
/*
 * Set various TCP timeout times 
 */

void SetTIMEOUTtime(TCP_SOCK_T *s)
{	
	s->timeout=set_ttimeout(tcp_TIMEOUT);
}

void SetLONGtimeout(TCP_SOCK_T *s)
{
	s->timeout=set_ttimeout(tcp_LONGTIMEOUT);
}

/*----------------------------------------------------------------------
* tcp_init
*	Initialize TCP data, i.e. socket-like interface
*----------------------------------------------------------------------*/
int tcp_init(void)
{
	int i;
	TCP_SOCK_T *sock;
	
	sock = (TCP_SOCK_T *)&tcp_socks[0];
	for (i=0; i<TCP_SOCKET_MAX_NUM; i++, sock++)
	{
		memset(sock, 0, sizeof(TCP_SOCK_T));
		sock->local_port = TCP_PORT_HTTP;
		sock->mss = 1024;//1460;
		sock->post_data = 0;
	}
}

/*
 * Extract data from incoming tcp segment.
 * Returns true if packet is queued on rxlist, false otherwise.
 */
int handle_data(TCP_SOCK_T		*sock, NETBUF_HDR_T *netbuf_p)
{
	ip_header_t *iph;
	tcp_header_t *tcph;
    unsigned int  diff, seq;
    int           data_len,hdr_bytes;
    char          *data_ptr;

	iph =(ip_header_t *)(netbuf_p->datap + ETH_HDR_SIZE);
	hdr_bytes = iph->ihl << 2;
	tcph = (tcp_header_t *)(((char *)iph) + hdr_bytes);
    data_len = ntohs(iph->tot_len) - (tcph->hdr_len << 2);
    seq = ntohl(tcph->seq);
    if (SEQ_LE(seq, sock->remote_seq)) {
	/*
	 * Figure difference between which byte we're expecting and which byte
	 * is sent first. Adjust data length and data pointer accordingly.
	 */
	diff = sock->remote_seq - seq;
	data_len -= diff;
	if (data_len > 0) {
	    /* queue the new data */
	    sock->remote_seq += data_len;
	}
    }
}

void handle_ack(TCP_SOCK_T		*sock, NETBUF_HDR_T *netbuf_p)
{
	ip_header_t *iph;
	tcp_header_t *tcph;
    unsigned short        ack;
    int          advance,hdr_bytes;
    char         *dp;

	iph =(ip_header_t *)(netbuf_p->datap + ETH_HDR_SIZE);

	hdr_bytes = iph->ihl << 2;
	tcph = (tcp_header_t *)(((char *)iph) + hdr_bytes);
	
    /* process ack value in packet */
    ack = ntohl(tcph->ack);
    if (SEQ_GT(ack, sock->local_seq)) {
	//__timer_cancel(&s->timer);
	advance = ack - sock->local_seq;
	if (advance > sock->data_bytes)
	    advance = sock->data_bytes;

	if (advance > 0) {
	    sock->local_seq+= advance;
	    sock->data_bytes -= advance;
	    if (sock->data_bytes) {
		/* other end ack'd only part of the pkt */
		dp = (char *)(tcph + 1);
		memcpy(dp, dp + advance, sock->data_bytes);
	    }
	}
    }
}

/*----------------------------------------------------------------------
* tcp_input
*	Receive a TCP frame
*----------------------------------------------------------------------*/
int tcp_input(NETBUF_HDR_T *netbuf_p)
{
	// eth_header_t	*eth_hdr;
    ip_header_t		*iph;
	tcp_header_t 	*tcph;
	int				len;
	int				i,process_data=0;
	TCP_SOCK_T		*sock;
    ARP_ENTRY_T		arp;
   unsigned char			*data_p;
	int         hdr_bytes;
	unsigned short         seq, ack;
	unsigned short           src, dest;

	iph =(ip_header_t *)(netbuf_p->datap + ETH_HDR_SIZE);

	hdr_bytes = iph->ihl << 2;
    len = ntohs(iph->tot_len) - hdr_bytes;
	iph->tot_len = htons(len);
	
	tcph = (tcp_header_t *)(((char *)iph) + hdr_bytes);
	data_p = (unsigned char *)(tcph+1);
	
    if (tcph->chksum == 0xffff)
		tcph->chksum = 0;

    if (ip_csum((u16 *)tcph, len, tcp_pseudo_sum(iph)) != 0)
		goto tcp_input_end;

	sock = (TCP_SOCK_T *)&tcp_socks[0];
	for (i=0; i<TCP_SOCKET_MAX_NUM; i++, sock++)
	{
		if(sock->del_timeout==1)
				tcp_close(sock);
	}
	sock = (TCP_SOCK_T *)&tcp_socks[0];
	for (i=0; i<TCP_SOCKET_MAX_NUM; i++, sock++)
	{
		if ((sock->remote_port== htons(tcph->sport)) && (sock->local_port == htons(tcph->dport)) && (ntohl(iph->daddr) == sys_get_ip_addr()) && (ntohl(iph->saddr) == sock->from_ip))
			 goto tcp_process;	
	}
	
	sock = (TCP_SOCK_T *)&tcp_socks[0];
	for (i=0; i<TCP_SOCKET_MAX_NUM; i++, sock++)
	{	
		if((sock->used == 0) && (htons(tcph->sport)!=0) && (htons(tcph->dport)==TCP_PORT_HTTP) && (ntohl(iph->daddr) == sys_get_ip_addr()))
		{
				//create tcp socket
			tcp_socket(i, iph->saddr, htons(tcph->dport), htons(tcph->sport));
			goto tcp_process;
		}
				
	}
	goto  tcp_input_end;
tcp_process:	

	sock = (TCP_SOCK_T *)&tcp_socks[0];
	for (i=0; i<TCP_SOCKET_MAX_NUM; i++, sock++)
	{				
		if ((sock->remote_port== htons(tcph->sport)) && (sock->local_port == htons(tcph->dport)) && (ntohl(iph->daddr) == sys_get_ip_addr()) && (ntohl(iph->saddr) == sock->from_ip))
		{
			// copy data
			//sock->from_port = ntohs(tcph->sport);
			//if (sock->buf_start_p)
			//{
			//if ((sock->data_size + len) > sock->buf_size)
			//	sock->error_code = TCP_ERROR_OVERFLOW;
			//else
			//{
			//	while (len)
			//	{
			//		*sock->buf_in_p++ = *data_p++;
			//		if (sock->buf_in_p >= sock->buf_end_p)
			//			sock->buf_in_p = sock->buf_start_p;
			//		len--;
			//		sock->data_size++;
			//	}
			//}
				if (sock->state != SYN_RECVD && tcph->flags & TCP_FLAG_RST) {
						sock->state = CLOSED;
						goto  tcp_input_end;
	   				 }
                    sock->from_ip = ntohl(iph->saddr);
					sock->remote_port = htons(tcph->sport);
					
					switch (sock->state)
					{
						case LISTEN:
							if (tcph->flags & TCP_CNTRL_SYN)
							{
								//sock->timeout = 0;
								//sock->data_bytes = 0;
								//sock->post_data = 0;
								sock->local_seq = TCP_START_SEQ;
								sock->remote_seq = ntohl(tcph->seq)+1;
								sock->flag = TCP_CNTRL_ACK | TCP_CNTRL_SYN;
								sock->old_seq = 0;
								sock->new_seq = 0;
								//sock->post_data = 0;
								
								if (tcp_sendto(sock, 0, 0, sock->from_ip, sock->local_port, sock->remote_port) < 0)
    							{
										tcp_close(sock);
    							    goto tcp_input_end;
    							}	
								// received SYN, send SYN and ACK, enter SYN_RECVD
	   							sock->state = SYN_RECVD;
								SetTIMEOUTtime(sock);
							}
							else
							{
								sock->post_data = 0;
								seq = ntohl(tcph->ack);
    							ack = ntohl(tcph->seq);
    							src = ntohs(tcph->dport);
    							dest = ntohs(tcph->sport);
								if(tcph->flags & TCP_FLAG_RST)
								{
									tcp_close(sock);
									goto tcp_input_end;
								}
								
								if (tcph->flags & TCP_FLAG_ACK) {
									seq = ntohl(tcph->ack);
									ack = 0;
								}
								else {
									if(tcph->flags & TCP_CNTRL_SYN)
										ack = ntohl(tcph->seq++);
									else
										ack = ntohl(tcph->seq);
									seq = 0;
								}				
    							/* tcp header */
    							sock->local_seq = seq;// htonl(seq);
								sock->remote_seq = ack;//htonl(ack);
    							sock->local_port = src;//htons(src);
    							sock->remote_port = dest;//htons(dest);
    							sock->flag = TCP_FLAG_RST | TCP_FLAG_ACK;
								if (tcp_sendto(sock, 0, 0, sock->from_ip, sock->local_port, sock->remote_port) < 0)
    							{
									tcp_close(sock);
    							    goto tcp_input_end;
    							}	
    							tcp_close(sock);
								//send_reset(pkt, r);
							}
							break;
						case SYN_SENT:

							if (tcph->flags & TCP_CNTRL_SYN)
							{
								sock->flag = TCP_CNTRL_ACK;
								SetTIMEOUTtime(sock);
								if ((tcph->flags & TCP_CNTRL_ACK) && (tcph->ack == (sock->local_seq + 1))) 
								{
   									// received SYN and ACK, enter ESTABLISHED, send ACK
									sock->state = ESTABLISHED;
									++sock->local_seq;
									sock->remote_seq = ntohl(tcph->seq+1);
									tcp_parse_options(sock,tcph);
									//tcp_processdata(sock, tp, len);
								
									if (tcp_sendto(sock, 0, 0, sock->from_ip, sock->local_port, sock->remote_port) < 0)
    								{
										tcp_close(sock);
    							    	goto tcp_input_end;
    								}	
								}
								else
								{
									sock->remote_seq++;
									sock->state = SYN_RECVD;
								}
							}
							//else if (tcph->flags & TCP_CNTRL_SYN)
							//{
							//	printf("pak(%d)------> SYN_SENT  : TCP_CNTRL_SYN\n",pak);
	   					//		// received SYN, enter SYN_RECVD, send ACK
							//	sock->remote_seq = ntohl(tcph->seq);
							//	sock->state = SYN_RECVD;
   						//		sock->flag = TCP_CNTRL_ACK;
							//	SetTIMEOUTtime(sock);
							//	if (tcp_sendto(sock, 0, 0, sock->from_ip, sock->local_port, sock->remote_port) < 0)
    					//		{
							//		tcp_close(sock);
    					//		    return TCP_ERROR_SENDTO;
    					//		}	
							//	
							//}
							break;
						case SYN_RECVD:
							if (tcph->flags & TCP_CNTRL_RST)
							{
								//sock->state = LISTEN;
								//sock->post_data = 0;
								tcp_close(sock);
							}						
							else if (tcph->flags & TCP_CNTRL_SYN)
							{
								sock->flag = TCP_FLAG_RST | TCP_FLAG_ACK;
								if (tcp_sendto(sock, 0, 0, sock->from_ip, sock->local_port, sock->remote_port) < 0)
    							{
									tcp_close(sock);
    							    goto tcp_input_end;
    							}	
								SetTIMEOUTtime(sock);
							}
							else if ((tcph->flags & TCP_CNTRL_ACK)  && (ntohl(tcph->ack) == (sock->local_seq+ 1)))
   	 						{
   	 							//printf("pak(%d)------> SYN_RECVD  : TCP_CNTRL_ACK\n",pak);
   	 							//printf("ntohl(tcph->ack):(%x)  local(%x:%x)  rem(%x)",tcph->ack,sock->local_seq,ntohl(sock->local_seq),sock->remote_seq);
   	  		 					// received ACK, enter ESTABLISHED
   	  		 					sock->flag = TCP_FLAG_ACK;
								sock->local_seq++;
											//sock->remote_seq = ntohl(tcph->ack);
											sock->remote_seq = ntohl(tcph->seq);
											sock->remote_seq += ( ntohs(iph->tot_len) - (tcph->hdr_len << 2));
								sock->state = ESTABLISHED;
								SetTIMEOUTtime(sock);
								//if (tcp_sendto(sock, 0, 0, sock->from_ip, sock->local_port, sock->remote_port) < 0)
    							//{
								//	tcp_close(sock);
    							//   return TCP_ERROR_SENDTO;
    							//}	
								//sock->timeout = sys_get_ticks() + (10 * BOARD_TPS);
   				   			}
							break;
						case ESTABLISHED:
						case CLOSE_WAIT:
						//printf("pak(%d)------> ESTABLISHED  : -->\n",pak);
							sock->new_seq = ntohl(tcph->seq);
							//printf("pak(%d)------> ESTABLISHED  : -->%x %x %x %x\n",pak,ntohl(tcph->seq),ntohs(tcph->seq),htonl(tcph->seq),htons(tcph->seq));
							//if (tcph->flags & TCP_CNTRL_SYN)
							//{
							//	//printf("pak(%d)------> ESTABLISHED  : TCP_CNTRL_SYN\n",pak);
							//	if(sys_get_ticks()>sock->timeout)
							//	{
							//		//printf("pak(%d)------> ESTABLISHED  : else\n",pak);
							//		//sock->state = LISTEN;
							//		//process_data = 0;
							//		//sock->post_data = 0;
							//		tcp_close(sock);
							//	}
							//}
							//else 
							if ((tcph->flags & TCP_FLAG_ACK) == 0)
								 return;		/* must ack sommat */
							//tcp_processdata(sock, tcph, len);
							//ack = sock->remote_seq;  /* save original ack */
							
							//printf("pak(%d)------>1 ip seq: %x ack: %x sock seq: %x ack: %x\n",pak,ntohl(tcph->seq),ntohl(tcph->ack),sock->local_seq,sock->remote_seq);
							handle_data(sock, netbuf_p);
							
							if (tcph->flags & TCP_FLAG_ACK)
							{	
							//int test=0;
									//process_data = 1;
							    handle_ack(sock, netbuf_p);
									
								//printf("pak(%d)------> ESTABLISHED  : TCP_FLAG_ACK  len(%d)\n",pak,ntohs(iph->tot_len));
								
								data_p = (unsigned char *)(tcph+1);
    						len = ntohs(iph->tot_len);
								hdr_bytes = tcph->hdr_len << 2;
    						len -= hdr_bytes; 
								
								if (tcph->flags & TCP_CNTRL_FIN)
								{
									//printf("pak(%d)------>4 ip seq: %x ack: %x sock seq: %x ack: %x\n",pak,ntohl(tcph->seq),ntohl(tcph->ack),sock->local_seq,sock->remote_seq);
									//process_data = 0;
									//sock->post_data = 0;
              	
									sock->state = LAST_ACK;
									sock->remote_seq++;
									sock->flag = TCP_CNTRL_ACK | TCP_CNTRL_FIN;
									if (tcp_sendto(sock, 0, 0, sock->from_ip, sock->local_port, sock->remote_port) < 0)
    								{
										tcp_close(sock);
    								   goto tcp_input_end;
    								}	
    								//tcp_close(sock);
									goto tcp_input_end;
								}
								else if (tcph->flags & TCP_CNTRL_RST)
								{
									//printf("pak(%d)------> ESTABLISHED  : TCP_CNTRL_RST\n",pak);
									//sock->state = LISTEN;
									//sock->post_data = 0;
									tcp_close(sock);
									goto tcp_input_end;
								}
								
								if ((ntohs(iph->tot_len)>(tcph->hdr_len << 2)) && ((tcph->flags & TCP_CNTRL_FIN) == 0))//&& (sock->post_data == 0))//TCP_HDR_SIZE))//if ((process_data == 1)&& (len>0))
								{

									switch (htons(tcph->dport))
									{
										case TCP_PORT_HTTP:
										//printf("pak(%d)------> process_data  : TCP_PORT_HTTP\n",pak);
											sock->flag = TCP_CNTRL_ACK;
											
											rx_http_packet((unsigned char *)data_p, len, sock);
									}
									
								}
								
							}
								
   							break;
						
						case FIN_WAIT_1:	

										if ((tcph->flags & (TCP_FLAG_FIN | TCP_FLAG_ACK)) == (TCP_FLAG_FIN | TCP_FLAG_ACK)) {
										    handle_ack(sock, netbuf_p);
											if (tcph->seq == sock->local_seq) {
												sock->remote_seq++;	/* ack their FIN */
													if (tcph->ack >= (sock->local_seq + 1)) {
													    /* Not simul close (they ACK'd our FIN) */
													    sock->local_seq++;
													    sock->state = TIME_WAIT;
													} else {
													    /* simul close */
													    sock->state = CLOSING;
													}
													sock->flag = TCP_FLAG_ACK;
														if (tcp_sendto(sock, 0, 0, sock->from_ip, sock->local_port, sock->remote_port) < 0)
    													{
															tcp_close(sock);
			    									    	goto tcp_input_end;
    													}
													if (sock->state == TIME_WAIT)
													    SetLONGtimeout(sock);
													else
													    SetTIMEOUTtime(sock);
											 }
										}
										else if (tcph->flags  & TCP_FLAG_ACK) {
											    /* Other side is legitamately acking us */
											    if ((tcph->ack == (sock->local_seq + 1)) && (tcph->seq == sock->remote_seq) && (sock->data_bytes == 0)) {
												sock->local_seq++;
												sock->state = FIN_WAIT_2;
												SetTIMEOUTtime(sock);
											    }
										}
										if (tcph->flags & TCP_FLAG_FIN) {
										    //__timer_cancel(&s->timer);
										    sock->remote_seq++;
										    sock->state = LAST_ACK;//CLOSING;
											sock->flag = TCP_FLAG_FIN | TCP_FLAG_ACK;
												if (tcp_sendto(sock, 0, 0, sock->from_ip, sock->local_port, sock->remote_port) < 0)
    											{
													tcp_close(sock);
			    							    	goto tcp_input_end;
    											}	
										}
							break;
						case FIN_WAIT_2:
						//printf("pak(%d)------> FIN_WAIT_2  : -->\n",pak);
						handle_data(sock, netbuf_p);
						/* They may be still transmitting data, must read it */
												//tcp_processdata(sock, tcph, len);
											
									if ((tcph->flags & (TCP_FLAG_FIN | TCP_FLAG_ACK)) == (TCP_FLAG_FIN | TCP_FLAG_ACK)) {
									    if ((tcph->ack == sock->local_seq) && (tcph->seq == sock->remote_seq)) {
										sock->flag = TCP_FLAG_ACK;
										sock->remote_seq++;
										if (tcp_sendto(sock, 0, 0, sock->from_ip, sock->local_port, sock->remote_port) < 0)
    									{
											tcp_close(sock);
			    				    		goto tcp_input_end;
    									}
										sock->state = TIME_WAIT;
										SetLONGtimeout(sock);
									    }
									}
							break;
						case CLOSING:
							//printf("pak(%d)------> CLOSING  : -->\n",pak);
							if ((tcph->flags & (TCP_FLAG_ACK | TCP_FLAG_FIN)) == TCP_FLAG_ACK) {
										    handle_ack(sock, netbuf_p);
									if ((tcph->ack == (sock->local_seq /* + 1*/)) && (tcph->seq == sock->remote_seq)) 
									{
										sock->state = TIME_WAIT;
										SetTIMEOUTtime(sock);
									}	
							}
							break;
						case LAST_ACK:
						//printf("pak(%d)------> LAST_ACK  : -->\n",pak);
							if (tcph->flags & TCP_FLAG_ACK) {
								handle_ack(sock, netbuf_p);
								if (ntohl(tcph->ack) == (sock->local_seq+ 1)) {
									sock->state = CLOSED;
									}
							}
								
							if (tcph->flags & TCP_FLAG_FIN)
							{
								/* They lost our two packets, back up */
								sock->flag = TCP_FLAG_ACK | TCP_FLAG_FIN;
								if (tcp_sendto(sock, 0, 0, sock->from_ip, sock->local_port, sock->remote_port) < 0)
    							{
									tcp_close(sock);
			    			   		goto tcp_input_end;
    							}
								SetTIMEOUTtime(sock);
							}

							else 
							{
								if (tcph->ack == (sock->local_seq + 1) && (tcph->seq == sock->remote_seq)) 
								{
									sock->state = CLOSED;	/*no 2msl need */
									//tcp_signal_task(sock,handler_CLOSED);  /* Close it up immediately */
								}
									
							}
						case CLOSED:
						//printf("pak(%d)------> CLOSED  : -->\n",pak);
							sock->state = LISTEN;
							process_data = 0;
							sock->post_data = 0;
							seq = ntohl(tcph->ack);
    							ack = ntohl(tcph->seq);
    							src = ntohs(tcph->dport);
    							dest = ntohs(tcph->sport);
								if(tcph->flags & TCP_FLAG_RST)
								{
									tcp_close(sock);
									goto tcp_input_end;
								}
								
								if (tcph->flags & TCP_FLAG_ACK) {
									seq = ntohl(tcph->ack);
									ack = 0;
								}
								else {
									if(tcph->flags & TCP_CNTRL_SYN)
										ack = ntohl(tcph->seq++);
									else
										ack = ntohl(tcph->seq);
									seq = 0;
								}
    							/* tcp header */

    							sock->local_seq = seq;// htonl(seq);
								sock->remote_seq = ack;//htonl(ack);
    							sock->local_port = src;//htons(src);
    							sock->remote_port = dest;//htons(dest);
    							sock->flag = TCP_FLAG_RST | TCP_FLAG_ACK;
								if (tcp_sendto(sock, 0, 0, sock->from_ip, sock->local_port, sock->remote_port) < 0)
    							{
									tcp_close(sock);
    							    goto tcp_input_end;
    							}	
								tcp_close(sock);
							break;
						case TIME_WAIT:
						//printf("pak(%d)------> TIME_WAIT  : -->\n",pak);
								if (tcph->flags & (TCP_FLAG_ACK | TCP_FLAG_FIN) == (TCP_FLAG_ACK | TCP_FLAG_FIN)) 
								{
									/* he needs an ack */
									sock->flag = TCP_FLAG_ACK;
									if (tcp_sendto(sock, 0, 0, sock->from_ip, sock->local_port, sock->remote_port) < 0)
    								{
										tcp_close(sock);
			    					   	goto tcp_input_end;
    								}
									sock->state = CLOSED;         /* Might this be bad so soon after an ACK? */
									//tcp_signal_task(s,handler_CLOSED);  /* Close it up immediately */
								}
												
					
							break;
					}
			
			//} ///if (sock->buf_start_p)
			//else
			//{
			//	sock->error_code = TCP_ERROR_NO_BUF;
			//}
			
			
			net_free_buf((void *)netbuf_p);
			//printf("normal pak(%d)<------\n\n",pak);
			return;
		}
	}

tcp_input_end:	
	net_free_buf((void *)netbuf_p);
	//printf("tcp_input_end pak(%d)<------\n\n",pak);
	return;
}

/*----------------------------------------------------------------------
* tcp_input
*	Receive a TCP frame
*----------------------------------------------------------------------*/
static int tcp_pseudo_sum(ip_header_t *ip)
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
//    sum += su.s;
    sum += *(u16 *)su.c;	

    sum += ip->tot_len;
    
    return sum;
}

/*----------------------------------------------------------------------
* tcp_socket
*	i/p: Server: Server IP Address, host-oriented
*	Return -1 if no enough socket
*----------------------------------------------------------------------*/
int tcp_socket(int sock_num,UINT32 server, u16 local_port, u16 remote_port)
{
	int i;
	TCP_SOCK_T *sock;
	
	sock = (TCP_SOCK_T *)&tcp_socks[sock_num];
	//printf("tcp_socket <------remote_port(%d)  i:%d \n",remote_port,sock_num);	
	//for (i=0; i<TCP_SOCKET_MAX_NUM; i++, sock++)
	{
		if (!sock->used)
		{
			sock->used = 1;
				sock->local_port = local_port;
				sock->remote_port = remote_port;
			//sock->buf_start_p = (char *)malloc(TCP_SOCK_BUF_SIZE);
			//if (!sock->buf_start_p)
			//{
			//	dbg_printf(("No Free memory\n"));
			//	return -1;
			//}
			sock->from_ip = htonl(server);
			//sock->buf_in_p = sock->buf_start_p;
			//sock->buf_out_p = sock->buf_start_p;
			//sock->buf_end_p  = sock->buf_start_p + TCP_SOCK_BUF_SIZE;
			sock->buf_size = TCP_SOCK_BUF_SIZE;
			sock->data_size = 0;
			sock->error_code = 0;
			//sock->local_port = TCP_PORT_HTTP;
			//sock->remote_port = 0;
			sock->post_data = 0;
			sock->state = LISTEN;
			sock->flag = 0;
			return i;
		}
	}
	return -1;		
}

/*----------------------------------------------------------------------
* tcp_close
*----------------------------------------------------------------------*/
int tcp_close(TCP_SOCK_T *sock)
{
	TCP_SOCK_T *s;
	int i;
	
	//if (sock < 0 || sock >= TCP_SOCKET_MAX_NUM)
	//	return -1;

	s = (TCP_SOCK_T *)&tcp_socks[0];
	for (i=0; i<TCP_SOCKET_MAX_NUM; i++, sock++)
	{		
		if ((sock->used == 1) && (s->local_port == sock->local_port) && (s->remote_port == sock->remote_port) && (s->from_ip== sock->from_ip))
		{
			//printf("tcp_close <------sock->remote_port(%d)  i:%d \n",sock->remote_port,i);	
				//if (sock->buf_start_p)
				//	free(sock->buf_start_p);
				memset(sock, 0, sizeof(TCP_SOCK_T));
				sock->used = 0;
				sock->local_port = TCP_PORT_HTTP;
				sock->mss = 1024;//1460;
				sock->post_data = 0;		
		}
		
	}
	
	return 0;
}

/*----------------------------------------------------------------------
* tcp_output
*	Return -1 if no enough socket
*----------------------------------------------------------------------*/
//static int tcp_output(TCP_SOCK_T *sock, char *datap, int len, UINT32 host, UINT16 sport, UINT16 dport, ARP_ENTRY_T *arp)
int tcp_output(TCP_SOCK_T *sock, char *datap, int len, UINT32 host, UINT16 sport, UINT16 dport, ARP_ENTRY_T *arp)
{
	NETBUF_HDR_T	*netbuf_p;
    tcp_header_t	*tcph;
    ip_header_t		*iph;
    UINT32			my_ip_addr;
    unsigned short	cksum;
	unsigned int         tcp_magic;
    int           tcp_magic_size = sizeof(tcp_magic);
	
	if (!(netbuf_p = (NETBUF_HDR_T *)net_alloc_buf()))
	{
		dbg_printf(("No free net buffer!\n"));
		return -1;
	}
	
	if (len > TCP_DATA_MAX_SIZE)
	{
		printf("TCP data size is too long!(%d)\n",len);
		net_free_buf((void *)netbuf_p);
		return -1;
	}
	
		//printf("tcp_output len : %d\n",len);
	netbuf_p->len = len + TCP_HDR_SIZE;
	iph =(ip_header_t *)(netbuf_p->datap + ETH_HDR_SIZE);
	tcph =(tcp_header_t *)(netbuf_p->datap + ETH_HDR_SIZE + IP_HDR_SIZE);

	
//	tcph->length = htons(netbuf_p->len);
	//tcph->seq = htonl(sock->local_seq++);
	tcph->seq = htonl(sock->local_seq);
	tcph->ack = htonl(sock->remote_seq);
	//printf("--> in(%x)  ack(%x)\n",ntohl(tcph->seq),ntohl(tcph->ack));
	if ((sock->flag& TCP_CNTRL_SYN))
	{
		netbuf_p->len += 4;//8
		tcph->hdr_len = 6;//7
		tcp_magic = htonl(0x02040400);//htonl(0x02040000 | 0x5b4);//0x5b4 == 1460 0x200 : 512
		memcpy((unsigned char *)(tcph+1), &tcp_magic, 4);
		sock->data_bytes = 0;
		//tcp_magic = htonl(0x01010402);
		//memcpy(((unsigned char *)(tcph+1)+tcp_magic_size), &tcp_magic, 4);
		//iph->tot_len = len+sizeof(tcp_header_t)+8;
		if(len >0)
		{
			memcpy(((char *)(tcph + 1)+4), datap, len);	
		}
	}
	else
	{
		tcph->hdr_len = 5;
		iph->tot_len = len+sizeof(tcp_header_t)+sock->data_bytes;
		if(len >0)
		{
			memcpy((char *)(tcph + 1), datap, len);	
		}
	}

	if (sock->data_bytes)
	    tcph->flags |= TCP_CNTRL_PSH;
	
	tcph->chksum = 0;

	//if(!sock->resend)
	{
		tcph->sport = htons(sport);
		tcph->dport = htons(dport);
		tcph->flags = sock->flag;

		tcph->window = htons(0x0400);//0x5b4 == 1460 0x578 = 1400 0x200 512
		tcph->urgent = 0;
		my_ip_addr = sys_get_ip_addr();
		iph->saddr = htonl(my_ip_addr);
		iph->daddr = htonl(arp->ip_addr);
	    iph->protocol = IP_PROTO_TCP;
	}
	
    cksum = ip_csum((u16 *)tcph, netbuf_p->len, tcp_pseudo_sum(iph));
    tcph->chksum = htons(cksum);

    ip_output(netbuf_p, host, IP_PROTO_TCP, arp);

    return 0;
}


/*----------------------------------------------------------------------
* tcp_sendto
*	Return -1 if no enough socket
*----------------------------------------------------------------------*/
int tcp_sendto(TCP_SOCK_T *sock, char *datap, int len, UINT32 host, UINT16 sport, UINT16 dport)
{
	ARP_ENTRY_T *arp;


	if ((arp = (ARP_ENTRY_T *)arp_lookup(host)) == NULL)
	{
		printf("\nHost (%d.%d.%d.%d) is not found!\n",
				IP1(host), IP2(host), IP3(host), IP4(host));
		return -1;
	}
			tcp_output(sock, datap, len, host, sport, dport, arp);

}

/*----------------------------------------------------------------------
* tcp_recvfrom
*	Return -1 if no enough socket
*	return total size
*----------------------------------------------------------------------*/
int tcp_recvfrom(int s, char *datap, UINT16 listen_port, UINT16 *server_port, int max_size, int timeout_ms)
{
	TCP_SOCK_T	*sock;
	UINT64		delay_time;
	UINT32		delay_ticks;
	int 		total;
	
	if (s < 0 || s >= TCP_SOCKET_MAX_NUM)
		return -1;
	
	delay_ticks = (timeout_ms * BOARD_TPS) /1000;
	if (delay_ticks == 0) delay_ticks = 1;
	delay_time = sys_get_ticks() + delay_ticks;
	
	sock = (TCP_SOCK_T *)&tcp_socks[s];
	sock->local_port = htons(listen_port);
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


		*server_port = sock->remote_port;
		total =0;
#if 0		
		while(sock->data_size && max_size)
		{
			*datap++ = *sock->buf_out_p++;
			if (sock->buf_out_p >= sock->buf_end_p)
				sock->buf_out_p = sock->buf_start_p;
			total++;
			max_size--;
			sock->data_size--;
		}
#endif
#if 0	
	printf("Sock: buf_start_p=0x%x buf_end_p=0x%x, buf_out_p=0x%x, data_size=%d, max_size=%d\n\n",
			sock->buf_start_p, sock->buf_end_p, sock->buf_out_p, sock->data_size, max_size);
#endif
			
	return total;
}


#endif //#ifdef BOARD_SUPPORT_WEB

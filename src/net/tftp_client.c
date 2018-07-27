/****************************************************************************
* Copyright  Storlink Corp 2005.  All rights reserved.                
*--------------------------------------------------------------------------
* Name			: tftp_client.c
* Description	: 
*		Handle TFTP Client function
*
* History
*
*	Date		Writer		Description
*	-----------	-----------	-------------------------------------------------
*	04/27/2005	Gary Chen	Create
*
****************************************************************************/
#include <define.h>
#include <net.h>
#include "chksum.h"

static UINT16 tftpc_get_sport(void);

#define TFTP_BUF_SIZE			512
#define TFTP_PKT_SIZE			(TFTP_HDR_SIZE + TFTP_BUF_SIZE)
#define TFTP_TIMEOUT_NUM		5
#define _DUMP_WAITING_CHAR		1

static UINT64 wait_time;
#ifdef _DUMP_WAITING_CHAR
static void show_waiting_char(void)
{
	UINT64 current_time;
	
	current_time =  sys_get_ticks();
	if ((current_time - wait_time) > (BOARD_TPS / 4))
	{
		wait_time = current_time;
		printf(".");
	}
}
#else
	#define show_waiting_char()
#endif

#ifdef RECOVER_FROM_BOOTP
//bootp	
bootp_header_t *bp_info;
extern bool use_bootp;
UINT32 __local_ip_addr;
bool have_net;//, use_bootp;
bootp_header_t my_bootp_info;
ip_route_t     r;
extern unsigned char my_mac_addr[0][6];	
#endif
/*----------------------------------------------------------------------
* tftpc_get
*	Get a file from server (with IP address)
*	Handle binary only
*----------------------------------------------------------------------*/
int tftpc_get(char *filename, UINT32 server, char *buf, int len, int *total_size)
{
	tftp_header_t	*tftp;
	int 			s;
	int				data_len, file_len;
	char			*cp, *fp;
	UINT16			sport, server_port;
	int 			recv_len;
	int				total_timeouts;
	int				last_good_block;
	int				actual_len, total;
	
	*total_size = 0;
	
	if ((s = udp_socket(server)) < 0)
	{
		return TFTP_NET_ERR;
	}
	
	if (!(tftp = (tftp_header_t *)malloc(TFTP_PKT_SIZE)))
	{
		udp_close(s);
		return TFTP_NO_BUF;
	}
	
    // build TFTP header
    tftp->th_opcode = htons(TFTP_RRQ);	// read request
    data_len = 2;
    
    // add filename, including null character
    cp = (char *)&tftp->th_stuff;
    fp = filename;
    file_len = strlen(filename);
    memcpy(cp, filename, file_len+1);
    cp += file_len + 1;
    data_len += file_len + 1;
    
	// add mode, binary only
	strcpy(cp, "OCTET");
	cp += 5;
	*cp++ = '\0';
	data_len += 6;
	
	sport = tftpc_get_sport();
	
    if (udp_sendto(s, (char *)tftp, data_len, server, sport, TFTP_PORT) < 0)
    {
    	free(tftp);
		udp_close(s);
        return TFTP_NET_ERR;
    }
    
    // wait response
    // Read data
    fp = buf;
    total_timeouts = 0;
    last_good_block = 0;
    total = 0;
    while (1)
    {
    	int rc;
    	rc = udp_recvfrom(s, (char *)tftp, sport, &server_port, TFTP_PKT_SIZE, 3000);
    	if (rc < 0)	// error
    	{
    		free(tftp);
			udp_close(s);
			return TFTP_NET_ERR;
		}
		else if (rc == 0) // timeout
		{
            if ((++total_timeouts > TFTP_TIMEOUT_NUM) || (last_good_block == 0))
    		{
    			free(tftp);
				udp_close(s);
				return TFTP_TIMEOUT;
			}
			
            // Try resending last ACK
            tftp->th_opcode = htons(TFTP_ACK);
            tftp->th_block = htons(last_good_block);
    		if (udp_sendto(s, (char *)tftp, 4, server, sport, server_port) < 0)
    		{
    			free(tftp);
				udp_close(s);
				return TFTP_NET_ERR;
            }
        } 
        else // got data
        {
        	data_len = rc;
            if (ntohs(tftp->th_opcode) == TFTP_DATA)
            {
                actual_len = 0;
                if (ntohs(tftp->th_block) == (last_good_block + 1))
                {
                    // Consume this data
                    cp = tftp->th_data;
                    data_len -= 4;  /* Sizeof TFTP header */
                    actual_len = data_len;
                    total += actual_len;
					// printf("1: len = %d, data_len=%d ", len, data_len);
					//if (data_len != 512)
					//	printf("data_len=%d\n", data_len);
						
                    while (data_len-- > 0)
                    {
                        if (len > 0)
                        {
                            *fp++ = *cp++;
                        }
                        len --;
                    	if (len <= 0 && data_len > 0)
                    	{
							// printf("TFTP_TOO_LARGE\n");
							free(tftp);
							udp_close(s);
							return TFTP_TOO_LARGE;
						}
                    }
 					// printf("\t2: len = %d\n", len);
                    last_good_block++;
                }
                else
                {
                	// maybe re-transmit data block (same seq)
                	// set actual_len to avoid returning
                	actual_len = TFTP_BUF_SIZE;
                }
                	
                // Send out the ACK
                tftp->th_opcode = htons(TFTP_ACK);
                tftp->th_block = htons(last_good_block);
    			if (udp_sendto(s, (char *)tftp, 4, server, sport, server_port) < 0)
    			{
    				free(tftp);
					udp_close(s);
					return TFTP_NET_ERR;
				}
                if (len==0 || ((actual_len >= 0) && (actual_len < TFTP_BUF_SIZE)))
                {
    				free(tftp);
					udp_close(s);
					*total_size = total;
                    return 0;
                }
            } 
            else if (ntohs(tftp->th_opcode) == TFTP_ERROR)
            {
            	int err;
            	err = ntohs(tftp->th_code);
    			free(tftp);
				udp_close(s);
                return err;
            } else {
                // What kind of packet is this?
    			free(tftp);
				udp_close(s);
                return TFTP_UNKNOWN;
            }
        }
        show_waiting_char();
    }
    
	free(tftp);
	udp_close(s);
	
	return 0;
}


/*----------------------------------------------------------------------
* tftpc_put
*	PUT a file to server (with IP address)
*	Handle binary only
*----------------------------------------------------------------------*/
int tftpc_put(char *filename, UINT32 server, char *buf, int len, int *total_size)
{
	tftp_header_t	*tftp;
	int 			s;
	int				data_len, file_len;
	char			*cp, *fp, *sfp;
	UINT16			sport, server_port;
	int 			recv_len;
	int				total_timeouts;
	int				last_good_block;
	int				actual_len, total;
	int 			rc;
	
	*total_size = 0;

	if ((s = udp_socket(server)) < 0)
	{
		return TFTP_NET_ERR;
	}
	
	if (!(tftp = (tftp_header_t *)malloc(TFTP_PKT_SIZE)))
	{
		udp_close(s);
		return TFTP_NO_BUF;
	}
	
    total_timeouts = 0;
    while (1)
    {
        // Init TFTP Request Header Code - Write File
        tftp->th_opcode = htons(TFTP_WRQ);
        data_len=2;
    	
    	// add filename, including null character
    	cp = (char *)&tftp->th_stuff;
    	fp = filename;
    	file_len = strlen(filename);
    	memcpy(cp, filename, file_len+1);
    	cp += file_len + 1;
    	data_len += file_len + 1;
    
		// add mode, binary only
		strcpy(cp, "OCTET");
		cp += 5;
		*cp++ = '\0';
		data_len += 6;
	
	
		// send request
		sport = tftpc_get_sport();
    	if (udp_sendto(s, (char *)tftp, data_len, server, sport, TFTP_PORT) < 0)
    	{
    		free(tftp);
			udp_close(s);
			return TFTP_NET_ERR;
    	}

        // Wait for ACK
    	rc = udp_recvfrom(s, (char *)tftp, sport, &server_port, TFTP_PKT_SIZE, 3000);
    	if (rc < 0)	// error
    	{
    		free(tftp);
			udp_close(s);
			return TFTP_NET_ERR;
		}
		else if (rc == 0) // timeout
		{
			if (++total_timeouts > TFTP_TIMEOUT_NUM)
			{
    			free(tftp);
				udp_close(s);
				return TFTP_TIMEOUT;
			}
		}
		else
		{
			total_timeouts = 0;
			if (ntohs(tftp->th_opcode) == TFTP_ACK)
				break;
			else if (ntohs(tftp->th_opcode) == TFTP_ERROR)
			{
				int err = ntohs(tftp->th_code);
    			free(tftp);
				udp_close(s);
				return err;
            }
            else
            {
    			free(tftp);
				udp_close(s);
				return TFTP_UNKNOWN;
            }
        }
        show_waiting_char();
    }

    // Send data
    sfp = buf;
    last_good_block = 1;
    total = 0;
    while (total <= len)
    {
        // Build packet of data to send
        data_len = min(TFTP_BUF_SIZE, len - total);
        tftp->th_opcode = htons(TFTP_DATA);
        tftp->th_block = htons(last_good_block);
        cp = tftp->th_data;
        fp = sfp;
        actual_len = data_len + 4;
        while (data_len)
        {
        	*cp++ = *fp++;
        	data_len--;
        }
        
    	if (udp_sendto(s, (char *)tftp, actual_len, server, sport, server_port) < 0)
    	{
    		free(tftp);
			udp_close(s);
			return TFTP_NET_ERR;
		}
		
		if (actual_len == 4) // no data
			goto put_end;
		
        // Wait for ACK
    	rc = udp_recvfrom(s, (char *)tftp, sport, &server_port, TFTP_PKT_SIZE, 3000);
    	if (rc < 0)	// error
    	{
    		free(tftp);
			udp_close(s);
			return TFTP_NET_ERR;
		}
		else if (rc == 0) // timeout
		{
			if (++total_timeouts > TFTP_TIMEOUT_NUM)
			{
    			free(tftp);
				udp_close(s);
				return TFTP_TIMEOUT;
			}
		}
		else
		{
			total_timeouts = 0;
            if (ntohs(tftp->th_opcode) == TFTP_ACK)
            {
                if (ntohs(tftp->th_block) == last_good_block)
                {
                    sfp = fp;
                    total += (actual_len - 4);
                    last_good_block++;
                }
            }
            else if (ntohs(tftp->th_opcode) == TFTP_ERROR)
            {
				int err = ntohs(tftp->th_code);
    			free(tftp);
				udp_close(s);
				return err;
			}
            else
            {
    			free(tftp);
				udp_close(s);
				return TFTP_UNKNOWN;
            }
        }
        show_waiting_char();
    }

put_end:    

    free(tftp);
    udp_close(s);
    
    *total_size = total;
    
    return 0;
}



/*----------------------------------------------------------------------
* tftpc_get_sport
*----------------------------------------------------------------------*/
static UINT16 tftpc_get_sport(void)
{
	static int tftp_sport = 7700;
	
	tftp_sport++;
	if (tftp_sport >= 7800)
		tftp_sport = 7700;
		
	return tftp_sport;
}

/*----------------------------------------------------------------------
* tftpc_get_sport
*----------------------------------------------------------------------*/
char *tftp_err_msg(int code)
{
	switch (code)
	{
		case TFTP_NO_BUF:		return ("no free buffer");
		case TFTP_NET_ERR:		return ("network error");
		case TFTP_TIMEOUT:		return ("timeout");
		case TFTP_TOO_LARGE:	return ("File is too large");
		case TFTP_EUNDEF:		return ("not defined");
		case TFTP_ENOTFOUND:	return ("file not found"); 
		case TFTP_EACCESS:		return ("access violation");
		case TFTP_ENOSPACE:		return ("disk full or allocation exceeded");
		case TFTP_EBADOP:		return ("illegal TFTP operation");
		case TFTP_EBADID:		return ("unknown transfer ID");
		case TFTP_EEXISTS:		return ("file already exists");
		case TFTP_ENOUSER:		return ("no such user");
		default:				return ("Unknown!");
	}
}

#ifdef RECOVER_FROM_BOOTP
/*----------------------------------------------------------------------
* bootp_get
*	Get a IP address from server
*----------------------------------------------------------------------*/
int bootp_get_ip(bootp_header_t *info)
{
	UINT64	delay_time;
	UINT32	delay_ticks;
	int 	rc,s;
	bootp_header_t b;
	UINT16			server_port;
	//ip_route_t     r;
	int            retry;
	unsigned long  start;
	UINT32 server;
	
	int ipaddr[4] = BOARD_BOOTS_IP_ADDR;
	
	server = IPIV(ipaddr[0], ipaddr[1], ipaddr[2], ipaddr[3]);
	
	if ((s = udp_socket(server)) < 0)
	{
		return TFTP_NET_ERR;
	}
	
	// default value

	bp_info = info;
	memset(&b, 0, sizeof(b));
	
	b.bp_op = BOOTREQUEST;
	b.bp_htype = HTYPE_ETHERNET;
	b.bp_hlen = 6;
	b.bp_xid = (SHOULD_BE_RANDOM+sys_get_ticks());//SHOULD_BE_RANDOM;
	
	__local_ip_addr = 0;

	memcpy(b.bp_chaddr, (char *)& my_mac_addr[0][0], 6);
	
	/* fill out route for a broadcast */
	memset(r.ip_addr,255,4);
	memset(r.enet_addr,255,6);
		
	if (udp_sendto(s, (char *)&b, sizeof(b), server, IPPORT_BOOTPC, IPPORT_BOOTPS) < 0)
	{
		udp_close(s);
		return TFTP_NET_ERR;
	}

	rc = udp_recvfrom(s, (char *)&b, IPPORT_BOOTPC, &server_port, TFTP_PKT_SIZE, 1000);
	if (rc < 0)	// error
	{	
		udp_close(s);
		return TFTP_NET_ERR;
	}
	
	udp_close(s);
	
	return 0;
}


int tftp_load_image(void)
{
	UINT32 server,	location;
	char bootp_filename[128];
	int size;
	int 			rc;
	
	int ipaddr[4] = BOARD_BOOTS_IP_ADDR;

	server = IPIV(IP4(my_bootp_info.bp_siaddr.s_addr),IP3(my_bootp_info.bp_siaddr.s_addr),IP2(my_bootp_info.bp_siaddr.s_addr),IP1(my_bootp_info.bp_siaddr.s_addr));
	size = 0;
	location = (char *)sys_get_initrd_addr();
	printf("Load %s ...", (char *)sys_get_initrd_name());
	if ((rc = tftpc_get((char *)sys_get_initrd_name(), server, location, BOARD_BOOT2_MALLOC_BASE - location, &size)) != 0)
	{
		printf("\n\nFailed for TFTP! (%d) %s\n", rc, tftp_err_msg(rc));
    	return 1;
	}
	
	location = (char *)sys_get_kernel_addr();
	printf("\nLoad %s ...", (char *)sys_get_kernel_name());
	size = 0;
	if ((rc = tftpc_get((char *)sys_get_kernel_name(), server, location, BOARD_BOOT2_MALLOC_BASE - location, &size)) != 0)
	{
		printf("\n\nFailed for TFTP! (%d) %s\n", rc, tftp_err_msg(rc));
    	return 1;
	}
	
	return 0;
	
}
#endif


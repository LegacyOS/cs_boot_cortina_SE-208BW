/****************************************************************************
* Copyright  Storlink Corp 2005.  All rights reserved.                
*--------------------------------------------------------------------------
* Name			: net.h
* Description	: 
*		Define Network
*
* History
*
*	Date		Writer		Description
*	-----------	-----------	-------------------------------------------------
*	04/25/2005	Gary Chen	Create
*
****************************************************************************/
#ifndef _NET_H
#define _NET_H

#include <board_config.h>

#if (BOARD_ENDIAN == BOARD_LITTLE_ENDIAN)
	#define ntohl(x)	(unsigned long)((((unsigned long)x & 0x000000FF) << 24) |	\
            							(((unsigned long)x & 0x0000FF00) <<  8) |	\
            							(((unsigned long)x & 0x00FF0000) >>  8) |	\
            							(((unsigned long)x & 0xFF000000) >> 24))
	#define ntohs(x)	(unsigned short)((((unsigned short)x & 0x00FF) << 8) |	\
										 (((unsigned short)x & 0xFF00) >> 8))
	#define htonl(x)	ntohl(x)
	#define htons(x)	ntohs(x)
#else
	#define ntohl(x)	(x)
	#define ntohs(x)	(x)
	#define htonl(x)	(x)
	#define htons(x)	(x)
#endif

/*----------------------------------------------------------------------
* Packet Type
*----------------------------------------------------------------------*/
#define ETH_TYPE_IP				0x800
#define ETH_TYPE_ARP			0x806
#define ETH_TYPE_RARP			0x8053
#define ETH_TYPE_AOE			0x88a2
#define AF_INET      			1
#define INADDR_ANY   			0
#define IP_PROTO_ICMP  			1
#define IP_PROTO_TCP   			6
#define IP_PROTO_UDP  			17
#define ARP_HW_ETHER			1
#define ARP_HW_EXP_ETHER		2
#define	ARP_REQUEST				1
#define	ARP_REPLY				2
#define	RARP_REQUEST			3
#define	RARP_REPLY				4
#define ICMP_TYPE_ECHOREPLY		0
#define ICMP_TYPE_ECHOREQUEST	8

/*----------------------------------------------------------------------
* Ethernet header.
*----------------------------------------------------------------------*/
#define ETH_MAC_SIZE	6
typedef struct {
    unsigned char	da[ETH_MAC_SIZE];
    unsigned char	sa[ETH_MAC_SIZE];
    unsigned short	type;
} __attribute__ ((aligned(1), packed)) eth_header_t;

#define ETH_HDR_SIZE		sizeof(eth_header_t)
#define ETH_MAX_PKT_SIZE	1514

/*----------------------------------------------------------------------
* ARP header.
*----------------------------------------------------------------------*/
typedef struct {
    unsigned short		hw_type;
    unsigned short		protocol;
    unsigned char		hw_len;
    unsigned char		proto_len;
    unsigned short		opcode;
    unsigned char		sender_mac[ETH_MAC_SIZE];
    UINT32				sender_ip;
    unsigned char		target_mac[ETH_MAC_SIZE];
    UINT32				target_ip;
} __attribute__ ((aligned(1), packed)) arp_header_t;


#define ARP_PKT_SIZE  (sizeof(arp_header_t) + ETH_HDR_SIZE)

/*----------------------------------------------------------------------
* ARP Cache Entry
*----------------------------------------------------------------------*/
typedef struct {
    UINT32			ip_addr;
    unsigned char	mac_addr[ETH_MAC_SIZE];
    int				used;
} ARP_ENTRY_T;

#define ARP_MAX_CACHE_NUM	4

/*----------------------------------------------------------------------
* IP header.
*----------------------------------------------------------------------*/
typedef struct {
#if (BOARD_ENDIAN == BOARD_LITTLE_ENDIAN)
    unsigned char       ihl:4,
						version:4;
#else
    unsigned char		version:4,
						ihl:4;
#endif
    unsigned char		tos;
    unsigned short		tot_len;
    unsigned short		id;
    unsigned short		frag_off;
    unsigned char		ttl;
    unsigned char		protocol;
    unsigned short		chksum;
    unsigned long		saddr;
    unsigned long		daddr;
} __attribute__ ((aligned(1), packed)) ip_header_t;

#define IP_HDR_SIZE	(sizeof(ip_header_t))
#define IP_PKT_SIZE (sizeof(ip_header_t) + ETH_HDR_SIZE)

/* IP flags. */
#define IP_CE		0x8000		/* Flag: "Congestion"		*/
#define IP_DF		0x4000		/* Flag: "Don't Fragment"	*/
#define IP_MF		0x2000		/* Flag: "More Fragments"	*/
#define IP_OFFSET	0x1FFF		/* "Fragment Offset" part	*/

/*----------------------------------------------------------------------
* UDP header & socket
*----------------------------------------------------------------------*/
typedef struct {
    unsigned short		sport;
    unsigned short		dport;
    unsigned short		length;
    unsigned short		chksum;
} __attribute__ ((aligned(1), packed)) udp_header_t;

#define UDP_HDR_SIZE		(sizeof(udp_header_t))
#define UDP_DATA_MAX_SIZE	(ETH_MAX_PKT_SIZE - ETH_HDR_SIZE -	\
							IP_HDR_SIZE - UDP_HDR_SIZE)

typedef struct udp_sock_t {
	u16					used;
    u16					listen_port;
    u16					from_port;
    u16					pad;		// for alignment 4
    UINT32				from_ip;
    int					error_code;
    int					buf_size;
    int					data_size;
    char				*buf_start_p;
    char				*buf_end_p;
    char				*buf_in_p;
    char				*buf_out_p;
} UDP_SOCK_T;

#define UDP_SOCKET_MAX_NUM		1
#define UDP_SOCK_BUF_SIZE		(8 * 1024)

#define UDP_ERROR_NO_BUF		-1
#define UDP_ERROR_OVERFLOW		-2

/*----------------------------------------------------------------------
* TCP header.
*----------------------------------------------------------------------*/
typedef struct {
    unsigned short		sport;
    unsigned short		dport;
    unsigned int		seq;
    unsigned int		ack;
#if (BOARD_ENDIAN == BOARD_LITTLE_ENDIAN)
    unsigned char		reserved:4,
						hdr_len:4;
#else
    unsigned char		hdr_len:4,
						reserved:4;
#endif
    unsigned char		flags;
    unsigned short		window;
    unsigned short		chksum;
    unsigned short		urgent;
} __attribute__ ((aligned(1), packed)) tcp_header_t;


#define TCP_HDR_SIZE	(sizeof(tcp_header_t))


#define TCP_DATA_MAX_SIZE	(ETH_MAX_PKT_SIZE - ETH_HDR_SIZE - IP_HDR_SIZE - TCP_HDR_SIZE)

typedef struct tcp_sock_t {
	u16					used;
    u16					local_port;//listen_port;
    u16					remote_port;//from_port;
    u16					pad;		// for alignment 4
    UINT32				from_ip;
    int					error_code;
    int					buf_size;
    int					data_size;
    char				*buf_start_p;
    char				*buf_end_p;
    char				*buf_in_p;
    char				*buf_out_p;
	unsigned char state;		// Our current state
	unsigned int local_seq;		// Local sequence number
	unsigned int remote_seq;	// Remote sequence number
	unsigned char flag;		// Our current state
	u16               ack_pending; // true if outgoing ack is deferred while
	u16               data_bytes;   // number of data bytes in pkt //
	UINT64 timeout;
	u16 	mss;
	u16 	post_data;
	u16 	del_timeout;
	unsigned int	old_seq;
	unsigned int	new_seq;
	unsigned char boundary[100];
} TCP_SOCK_T;


#define TCP_SOCKET_MAX_NUM		5
#define TCP_SOCK_BUF_SIZE		(2 * 1024)

#define TCP_ERROR_NO_BUF		-1
#define TCP_ERROR_OVERFLOW		-2
#define TCP_ERROR_SENDTO		-3

#define TCP_FLAG_FIN	1
#define TCP_FLAG_SYN	2
#define TCP_FLAG_RST	4
#define TCP_FLAG_PSH	8
#define TCP_FLAG_ACK	16
#define TCP_FLAG_URG	32

#define TCP_MAX_CONN 1

#define tcp_TIMEOUT          5   /* Various TCP timers */
#define tcp_LONGTIMEOUT     20
#define tcp_RETRANSMITTIME   5
#define RETRAN_STRAT_TIME  100

enum tcp_state
{
	LISTEN = 1,
	SYN_SENT,
	SYN_RECVD,
	ESTABLISHED,
	FIN_WAIT_1,
	FIN_WAIT_2,
	CLOSE_WAIT,
	CLOSING,
	LAST_ACK,
	TIME_WAIT,
	CLOSED
};

enum tcp_cntrl_flags
{
	TCP_CNTRL_FIN = 0x01,
	TCP_CNTRL_SYN = 0x02,
	TCP_CNTRL_RST = 0x04,
	TCP_CNTRL_PSH = 0x08,
	TCP_CNTRL_ACK = 0x10,
	TCP_CNTRL_URG = 0x20
};

struct tcp_TCB
{
	unsigned int local_port;		// Local port number
	unsigned int remote_port;	// Remote port number
	unsigned char remote_addr[4];	// Remote IP address
	unsigned char state;		// Our current state
	unsigned char local_seq;		// Local sequence number
	unsigned char remote_seq;	// Remote sequence number
};

#define TCP_PORT_HTTP 80

#define TCP_START_SEQ 50

typedef struct 
{
	unsigned int tol_len;		// total length
	unsigned int curr_len;		// total length
	unsigned char *post_buf;	// post file buffer
	//unsigned char *progp;		// current point
} __attribute__ ((aligned(1), packed)) http_TCB;

/*----------------------------------------------------------------------
* ICMP header.
*----------------------------------------------------------------------*/
typedef struct {
    unsigned char		type;
    unsigned char		code;
    unsigned short		chksum;
    unsigned short		ident;
    unsigned short		seqnum;
} __attribute__ ((aligned(1), packed)) icmp_header_t;

#define ICMP_HDR_SIZE		(sizeof(icmp_header_t))
#define ICMP_MAX_DATA_SIZE	(1514 - ETH_HDR_SIZE - IP_HDR_SIZE - ICMP_HDR_SIZE)
#define ICMP_MIN_DATA_SIZE	(32)

/*----------------------------------------------------------------------
* TFTP header.
*----------------------------------------------------------------------*/
#define TFTP_PORT			69
#define	TFTP_SEGSIZE		512
#define	TFTP_RRQ			01	// read
#define	TFTP_WRQ			02	// write
#define	TFTP_DATA			03	// data packet
#define	TFTP_ACK			04	// ack
#define	TFTP_ERROR			05	// error

typedef struct	tftphdr {
	short				th_opcode;		/* packet type */
	union {
		unsigned short	tu_block;	/* block # */
		short			tu_code;	/* error code */
		char			tu_stuff[1];	/* request packet stuff */
	} __attribute__ ((packed)) th_u;
	char				th_data[0];		/* data or error string */
} __attribute__ ((aligned(1), packed)) tftp_header_t;

#define	th_block	th_u.tu_block
#define	th_code		th_u.tu_code
#define	th_stuff	th_u.tu_stuff
#define	th_msg		th_data

#define TFTP_HDR_SIZE		(sizeof(tftp_header_t))

// TFTP error code
// Operation Error
#define TFTP_NO_BUF			-1		// no free buffer
#define TFTP_NET_ERR      	-2		// network error
#define TFTP_TIMEOUT     	-3		// timeout
#define TFTP_TOO_LARGE		-4
#define TFTP_UNKNOWN		-5

// Protocol Error
#define	TFTP_EUNDEF			0		// not defined 
#define	TFTP_ENOTFOUND		1		// file not found 
#define	TFTP_EACCESS		2		// access violation
#define	TFTP_ENOSPACE		3		// disk full or allocation exceeded
#define	TFTP_EBADOP			4		// illegal TFTP operation
#define	TFTP_EBADID			5		// unknown transfer ID
#define	TFTP_EEXISTS		6		// file already exists
#define	TFTP_ENOUSER		7		// no such user

/*----------------------------------------------------------------------
* Net buffer
*----------------------------------------------------------------------*/
#define NET_BUF_TAG						(('n' << 24) | ('b' << 16) | ('u' << 8) | ('f'))

// Note the size of NETBUF_HDR_T must be (32 * n)
typedef struct net_buf_t {
	unsigned long		tag;
	struct net_buf_t	*next_buf;
 	unsigned int 		len;	// data length
 	unsigned char		*datap;	// points to data
} NETBUF_HDR_T;

#define NETBUF_HDR_SIZE		((sizeof(NETBUF_HDR_T) + 31) & (~31))
#define NETBUF_SIZE			(NETBUF_HDR_SIZE + 2048)
#define NET_BUF_NUM			100

typedef struct {
		struct net_buf_t	*first;
		struct net_buf_t	*tail;
		int					total;
} NETBUF_QUEUE_T;
//// bootp start////

#define SHOULD_BE_RANDOM  0x12345555
/*
 * Bootstrap Protocol (BOOTP).  RFC951 and RFC1395.
 *
 * This file specifies the "implementation-independent" BOOTP protocol
 * information which is common to both client and server.
 *
 */

#define BP_CHADDR_LEN	 16
#define BP_SNAME_LEN	 64
#define BP_FILE_LEN	128
#define BP_VEND_LEN	 64
#define BP_MINPKTSZ	300	/* to check sizeof(struct bootp) */

// IPv4 support
typedef struct in_addr {
    unsigned long  s_addr;  // IPv4 address
} in_addr_t;

typedef struct bootp {
    unsigned char    bp_op;			/* packet opcode type */
    unsigned char    bp_htype;			/* hardware addr type */
    unsigned char    bp_hlen;			/* hardware addr length */
    unsigned char    bp_hops;			/* gateway hops */
    unsigned int     bp_xid;			/* transaction ID */
    unsigned short   bp_secs;			/* seconds since boot began */
    unsigned short   bp_flags;			/* RFC1532 broadcast, etc. */
    struct in_addr   bp_ciaddr;			/* client IP address */
    struct in_addr   bp_yiaddr;			/* 'your' IP address */
    struct in_addr   bp_siaddr;			/* server IP address */
    struct in_addr   bp_giaddr;			/* gateway IP address */
    unsigned char    bp_chaddr[BP_CHADDR_LEN];	/* client hardware address */
    char	     bp_sname[BP_SNAME_LEN];	/* server host name */
    char	     bp_file[BP_FILE_LEN];	/* boot file name */
    unsigned char    bp_vend[BP_VEND_LEN];	/* vendor-specific area */
    /* note that bp_vend can be longer, extending to end of packet. */
} bootp_header_t;


/*
 * UDP port numbers, server and client.
 */
#define	IPPORT_BOOTPS		67
#define	IPPORT_BOOTPC		68
#define UDPPORT_FREECOME_MSG	5004

#define BOOTREPLY		2
#define BOOTREQUEST		1

typedef unsigned char enet_addr_t[6];
typedef unsigned char ip_addr_t[4];

/*
 * A IP<->ethernet address mapping.
 */
typedef struct {
    ip_addr_t    ip_addr;
    enet_addr_t  enet_addr;
} ip_route_t;

/*
 * Hardware types from Assigned Numbers RFC.
 */
#define HTYPE_ETHERNET		  1
#define HTYPE_EXP_ETHERNET	  2
#define HTYPE_AX25		  3
#define HTYPE_PRONET		  4
#define HTYPE_CHAOS		  5
#define HTYPE_IEEE802		  6
#define HTYPE_ARCNET		  7


/*
 * Vendor magic cookie (v_magic) for CMU
 */
#define VM_CMU		"CMU"

/*
 * Vendor magic cookie (v_magic) for RFC1048
 */
#define VM_RFC1048	{ 99, 130, 83, 99 }


/*
 * Tag values used to specify what information is being supplied in
 * the vendor (options) data area of the packet.
 */
/* RFC 1048 */
/* End of cookie */
#define TAG_END			((unsigned char) 255)
/* padding for alignment */
#define TAG_PAD			((unsigned char)   0)
/* Subnet mask */
#define TAG_SUBNET_MASK		((unsigned char)   1)
/* Time offset from UTC for this system */
#define TAG_TIME_OFFSET		((unsigned char)   2)
/* List of routers on this subnet */
#define TAG_GATEWAY		((unsigned char)   3)
/* List of rfc868 time servers available to client */
#define TAG_TIME_SERVER		((unsigned char)   4)
/* List of IEN 116 name servers */
#define TAG_NAME_SERVER		((unsigned char)   5)
/* List of DNS name servers */
#define TAG_DOMAIN_SERVER	((unsigned char)   6)
/* List of MIT-LCS UDL log servers */
#define TAG_LOG_SERVER		((unsigned char)   7)
/* List of rfc865 cookie servers */
#define TAG_COOKIE_SERVER	((unsigned char)   8)
/* List of rfc1179 printer servers (in order to try) */
#define TAG_LPR_SERVER		((unsigned char)   9)
/* List of Imagen Impress servers (in prefered order) */
#define TAG_IMPRESS_SERVER	((unsigned char)  10)
/* List of rfc887 Resourse Location servers */
#define TAG_RLP_SERVER		((unsigned char)  11)
/* Hostname of client */
#define TAG_HOST_NAME		((unsigned char)  12)
/* boot file size */
#define TAG_BOOT_SIZE		((unsigned char)  13)
/* RFC 1395 */
/* path to dump to in case of crash */
#define TAG_DUMP_FILE		((unsigned char)  14)
/* domain name for use with the DNS */
#define TAG_DOMAIN_NAME		((unsigned char)  15)
/* IP address of the swap server for this machine */
#define TAG_SWAP_SERVER		((unsigned char)  16)
/* The path name to the root filesystem for this machine */
#define TAG_ROOT_PATH		((unsigned char)  17)
/* RFC 1497 */
/* filename to tftp with more options in it */
#define TAG_EXTEN_FILE		((unsigned char)  18)
/* RFC 1533 */
/* The following are in rfc1533 and may be used by BOOTP/DHCP */
/* IP forwarding enable/disable */
#define TAG_IP_FORWARD          ((unsigned char)  19)
/* Non-Local source routing enable/disable */
#define TAG_IP_NLSR             ((unsigned char)  20)
/* List of pairs of addresses/masks to allow non-local source routing to */
#define TAG_IP_POLICY_FILTER    ((unsigned char)  21)
/* Maximum size of datagrams client should be prepared to reassemble */
#define TAG_IP_MAX_DRS          ((unsigned char)  22)
/* Default IP TTL */
#define TAG_IP_TTL              ((unsigned char)  23)
/* Timeout in seconds to age path MTU values found with rfc1191 */
#define TAG_IP_MTU_AGE          ((unsigned char)  24)
/* Table of MTU sizes to use when doing rfc1191 MTU discovery */
#define TAG_IP_MTU_PLAT         ((unsigned char)  25)
/* MTU to use on this interface */
#define TAG_IP_MTU              ((unsigned char)  26)
/* All subnets are local option */
#define TAG_IP_SNARL            ((unsigned char)  27)
/* broadcast address */
#define TAG_IP_BROADCAST        ((unsigned char)  28)
/* perform subnet mask discovery using ICMP */
#define TAG_IP_SMASKDISC        ((unsigned char)  29)
/* act as a subnet mask server using ICMP */
#define TAG_IP_SMASKSUPP        ((unsigned char)  30)
/* perform rfc1256 router discovery */
#define TAG_IP_ROUTERDISC       ((unsigned char)  31)
/* address to send router solicitation requests */
#define TAG_IP_ROUTER_SOL_ADDR  ((unsigned char)  32)
/* list of static routes to addresses (addr, router) pairs */
#define TAG_IP_STATIC_ROUTES    ((unsigned char)  33)
/* use trailers (rfc893) when using ARP */
#define TAG_IP_TRAILER_ENC      ((unsigned char)  34)
/* timeout in seconds for ARP cache entries */
#define TAG_ARP_TIMEOUT         ((unsigned char)  35)
/* use either Ethernet version 2 (rfc894) or IEEE 802.3 (rfc1042) */
#define TAG_ETHER_IEEE          ((unsigned char)  36)
/* default TCP TTL when sending TCP segments */
#define TAG_IP_TCP_TTL          ((unsigned char)  37)
/* time for client to wait before sending a keepalive on a TCP connection */
#define TAG_IP_TCP_KA_INT       ((unsigned char)  38)
/* don't send keepalive with an octet of garbage for compatability */
#define TAG_IP_TCP_KA_GARBAGE   ((unsigned char)  39)
/* NIS domainname */
#define TAG_NIS_DOMAIN		((unsigned char)  40)
/* list of NIS servers */
#define TAG_NIS_SERVER		((unsigned char)  41)
/* list of NTP servers */
#define TAG_NTP_SERVER		((unsigned char)  42)
/* and stuff vendors may want to add */
#define TAG_VEND_SPECIFIC       ((unsigned char)  43)
/* NetBios over TCP/IP name server */
#define TAG_NBNS_SERVER         ((unsigned char)  44)
/* NetBios over TCP/IP NBDD servers (rfc1001/1002) */
#define TAG_NBDD_SERVER         ((unsigned char)  45)
/* NetBios over TCP/IP node type option for use with above */
#define TAG_NBOTCP_OTPION       ((unsigned char)  46)
/* NetBios over TCP/IP scopt option for use with above */
#define TAG_NB_SCOPE            ((unsigned char)  47)
/* list of X Window system font servers */
#define TAG_XFONT_SERVER        ((unsigned char)  48)
/* list of systems running X Display Manager (xdm) available to this client */
#define TAG_XDISPLAY_SERVER     ((unsigned char)  49)

/* While the following are only allowed for DHCP */
/* DHCP requested IP address */
#define TAG_DHCP_REQ_IP         ((unsigned char)  50)
/* DHCP time for lease of IP address */
#define TAG_DHCP_LEASE_TIME     ((unsigned char)  51)
/* DHCP options overload */
#define TAG_DHCP_OPTOVER        ((unsigned char)  52)
/* DHCP message type */
#define TAG_DHCP_MESS_TYPE      ((unsigned char)  53)
/* DHCP server identification */
#define TAG_DHCP_SERVER_ID      ((unsigned char)  54)
/* DHCP ordered list of requested parameters */
#define TAG_DHCP_PARM_REQ_LIST  ((unsigned char)  55)
/* DHCP reply message */
#define TAG_DHCP_TEXT_MESSAGE   ((unsigned char)  56)
/* DHCP maximum packet size willing to accept */
#define TAG_DHCP_MAX_MSGSZ      ((unsigned char)  57)
/* DHCP time 'til client needs to renew */
#define TAG_DHCP_RENEWAL_TIME   ((unsigned char)  58)
/* DHCP  time 'til client needs to rebind */
#define TAG_DHCP_REBIND_TIME    ((unsigned char)  59)
/* DHCP class identifier */
#define TAG_DHCP_CLASSID        ((unsigned char)  60)
/* DHCP client unique identifier */
#define TAG_DHCP_CLIENTID       ((unsigned char)  61)

/* XXX - Add new tags here */


/*
 * "vendor" data permitted for CMU bootp clients.
 */

struct cmu_vend {
	char		v_magic[4];	/* magic number */
        unsigned int    v_flags;        /* flags/opcodes, etc. */
	struct in_addr 	v_smask;	/* Subnet mask */
	struct in_addr 	v_dgate;	/* Default gateway */
	struct in_addr	v_dns1, v_dns2; /* Domain name servers */
	struct in_addr	v_ins1, v_ins2; /* IEN-116 name servers */
	struct in_addr	v_ts1, v_ts2;	/* Time servers */
        int             v_unused[6];	/* currently unused */
};


/* v_flags values */
#define VF_SMASK	1	/* Subnet mask field contains valid data */

#define IPPORT_BOOTPS           67
#define IPPORT_BOOTPC           68



//// bootp end////
/*----------------------------------------------------------------------
* Routines
*----------------------------------------------------------------------*/
void net_init(void);

void net_init_buf(void);
NETBUF_HDR_T *net_alloc_buf(void);
void net_free_buf(NETBUF_HDR_T *hdr);
void net_put_rcvq(NETBUF_HDR_T *hdr);
NETBUF_HDR_T *net_get_rcvq(void);
int net_peek_rcvq(void);

void enet_init(void);
void enet_input(char *srcep, int size);
void enet_poll(void);
void enet_send(NETBUF_HDR_T *netbuf_p, unsigned char *dest, unsigned short ether_type);

void arp_input(NETBUF_HDR_T *netbuf_p);
ARP_ENTRY_T *arp_request(UINT32 ipaddr);
void arp_flush_cache(void);
ARP_ENTRY_T *arp_lookup(UINT32 ipaddr);

void ip_input(NETBUF_HDR_T *netbuf_p);
void ip_output(NETBUF_HDR_T *netbuf_p, UINT32 host, int protocol, ARP_ENTRY_T *arp);
int ip_verify_addr(UINT32 ipaddr);
int ip_verify_netmask(UINT32 netmask);
int ip_is_local_net(UINT32 addr);

void icmp_input(NETBUF_HDR_T *netbuf_p);
void icmp_ping(UINT32 host, UINT32 count, UINT32 size, UINT32 timeout_ms);

int udp_init(void);
int udp_input(NETBUF_HDR_T *netbuf_p);
int udp_socket(UINT32 server);
int udp_close(int s);
int udp_sendto(int s, char *datap, int len, UINT32 host, UINT16 sport, UINT16 dport);
int udp_recvfrom(int s, char *datap, UINT16 listen_port, UINT16 *server_port, int max_size, int timeout_ms);

int tcp_init(void);
int tcp_input(NETBUF_HDR_T *netbuf_p);
int tcp_recvfrom(int s, char *datap, UINT16 listen_port, UINT16 *server_port, int max_size, int timeout_ms);
int tcp_sendto(TCP_SOCK_T *sock, char *datap, int len, UINT32 host, UINT16 sport, UINT16 dport);
int tcp_socket(int sock_num,UINT32 server, u16 local_port, u16 remote_port);
int tcp_close(TCP_SOCK_T *sock);

#define _HTTP_PROTOCOL "HTTP/1.1"
#define _HTTP_SERVER "Cortinaweb v0.1"

void tx_http_packet(unsigned char *szData, unsigned int nLength, TCP_SOCK_T *sock);
void rx_http_packet(unsigned char *szData, unsigned int nLength, TCP_SOCK_T *sock);


int tftpc_get(char *filename, UINT32 server, char *buf, int len, int *total_size);
int tftpc_put(char *filename, UINT32 server, char *buf, int len, int *total_size);
char *tftp_err_msg(int code);

#endif // _NET_H


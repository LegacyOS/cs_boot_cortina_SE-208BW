#ifndef _XMODEM_H_
#define _XMODEM_H_

#define XMODEM_SOH	0x01
#define XMODEM_STX	0x02
#define XMODEM_EOT	0x04
#define XMODEM_ACK	0x06
#define XMODEM_BSP	0x08
#define XMODEM_NAK	0x15
#define XMODEM_CAN	0x18
#define XMODEM_EOF	0x1A

typedef struct {
    unsigned char	pkt[1024];
    unsigned char	*bufp;
    unsigned char	blk;
    unsigned char	cblk;
    unsigned char	crc1;
    unsigned char	crc2;
    unsigned char	next_blk;
    int 			len;
    int				mode;
    int				total_retries;
    int 			total_XMODEM_SOH;
    int				total_XMODEM_STX;
    int				total_XMODEM_CAN;
    int				crc_mode;
    int				at_eof;
    int				tx_ack;
	int 			good_cnt;
	int 			tx_ack_cnt;
} XMODEM_T;

#define XMODEM_INPUT_TIMEOUT			(1000)	// ms
#define XMODEM_MAX_RETRIES_WITH_CRC		30
#define XMODEM_MAX_RETRIES				(XMODEM_MAX_RETRIES_WITH_CRC + 20)
#define XMODEM_CAN_COUNT				3    // numbers of CAN to wait before quit

// Define error code
#define XMODEM_ERR_TIMEOUT		-1
#define XMODEM_ERR_EOF      	-2
#define XMODEM_ERR_CANCEL   	-3
#define XMODEM_ERR_FRAME    	-4
#define XMODEM_ERR_CHKSUM		-5
#define XMODEM_ERR_SEQUENCE		-6

int   Xmodem_open(int *err);    
void  Xmodem_close(int *err);    
void  Xmodem_terminate(int method);    
int   Xmodem_read(char *buf, int size, int *err);    
char *Xmodem_get_error_msg(int err);

// extern getc_io_funcs_t xyzModem_io;

#endif // _XYZMODEM_H_

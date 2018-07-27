
#ifndef __LPC_H__
#define __LPC_H__
//#define SL2312_LPC_HOST_BASE            0x26000000      
//#define SL2312_LPC_IO_BASE              0x27000000

#define LPC_BASE						SL2312_LPC_IO_BASE
#define LPC_IRQ_BASE	                16

// MB PnP configuration register 
#define LPC_KEY_ADDR	                (LPC_BASE+0x2E)
#define LPC_DATA_ADDR	                (LPC_BASE+0x2F)

// Device LDN
#define LDN_SERIAL1		0x01
#define LDN_SERIAL2		0x02
#define LDN_PARALLEL	0x03

#define PAR_PORT_DATA			0x27000378
#define PAR_PORT_STAT			0x27000379
#define PAR_PORT_CTRL			0x2700037A


void LPCEnterMBPnP(void);
void LPCExitMBPnP(void);
void LPCSetConfig(char, char, char);
char LPCGetConfig(char, char);
int  SearchIT8712(void);
unsigned char lpc_get(void);
void lpc_send(unsigned char);
void PPSH_get(unsigned char *, int);
void PPSH_put(unsigned char *, int);
int  parallel_port_download(void);

#endif


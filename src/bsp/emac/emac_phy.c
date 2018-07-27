/****************************************************************************
* Copyright  Storlink Corp 2005.  All rights reserved.                
*--------------------------------------------------------------------------
* Name			: emac_phy.c
* Description	: 
*		Handle Ethernet PHY and MII interface
*
* History
*
*	Date		Writer		Description
*	-----------	-----------	-------------------------------------------------
*	04/25/2005	Gary Chen	Create and implement from Jason's Redboot code
*
****************************************************************************/
#include <define.h>
#include <board_config.h>
#include <sl2312.h>

#ifndef MIDWAY
#include "emac_sl2312.h"

#define diag_printf			printf
#define printk				printf

/************************************************/
/*                 function declare             */
/************************************************/
unsigned int mii_read(unsigned char phyad,unsigned char regad);
void mii_write(unsigned char phyad,unsigned char regad,unsigned int value);
void emac_set_phy_status(void);
void emac_get_phy_status(void);
void outb(unsigned int addr, unsigned int value);
unsigned char inb(unsigned int addr);

/****************************************/
/*	SPI Function Declare		*/
/****************************************/
void SPI_write(unsigned char addr,unsigned int value);
unsigned int SPI_read(unsigned char table,unsigned char addr);
void SPI_write_bit(char bit_EEDO);
unsigned int SPI_read_bit(void);
void SPI_default(int vlan_enabled);
void SPI_reset(unsigned char rstype,unsigned char port_cnt);
void SPI_pre_st(void);
void SPI_CS_enable(unsigned char enable);
void SPI_Set_VLAN(unsigned char LAN,unsigned int port_mask);
void SPI_Set_tag(unsigned int port,unsigned tag);
void SPI_Set_PVID(unsigned int PVID,unsigned int port_mask);
unsigned int SPI_Get_PVID(unsigned int port);
void SPI_mac_lock(unsigned int port, unsigned char lock);
void SPI_get_port_state(unsigned int port);
void SPI_port_enable(unsigned int port,unsigned char enable);
unsigned int SPI_get_identifier(void);
void SPI_get_status(unsigned int port);

/****************************************/
/*	VLAN Function Declare		        */
/****************************************/
extern unsigned int FLAG_SWITCH;	/* if 1-->switch chip presented. if 0-->switch chip unpresented */
extern unsigned int full_duplex;
extern unsigned int chip_id;

struct emac_conf VLAN_conf[] = { 
#ifdef CONFIG_ADM_6999	
	{ (struct net_device *)0,0x7F,1 }, 
	{ (struct net_device *)0,0x80,2 } 
#endif
#ifdef CONFIG_ADM_6996
	{ (struct net_device *)0,0x0F,1 }, 
	{ (struct net_device *)0,0x10,2 } 
#endif
};

#define NUM_VLAN_IF	(sizeof(VLAN_conf)/sizeof(struct emac_conf))


/************************************************/
/*                 function body                */
/************************************************/

#define emac_read_reg(offset)					REG32(EMAC_BASE_ADDR + offset)
#define emac_write_reg(offset, data, mask)		REG32(EMAC_BASE_ADDR + offset) = 	\
												(emac_read_reg(offset) & (~mask)) | \
												(data & mask)

#define readl(addr)								REG32(addr)
#define writel(value, addr)						REG32(addr) = value

        			
//module_init(emac_init_module);
//module_exit(emac_cleanup_module);

void emac_set_phy_status(void)
{
    unsigned int value;
    unsigned int i = 0;

    if (FLAG_SWITCH==1)
    {
        return; /* EMAC connects to a switch chip, not PHY */
    }
	
	return;

#ifndef LPC_IT8712
	// Disable GPIO pins switch to LPC PAD
    REG32(SL2312_GLOBAL_BASE + GLOBAL_MISC_CTRL) &=  ~0x00000018;
#endif
	
#define CONFIG_SL2312_ASIC            
#ifdef CONFIG_SL2312_ASIC    
    mii_write(PHY_ADDR,0x04,0x05e1); /* advertisement 100M full duplex, pause capable on */
#else
    mii_write(PHY_ADDR,0x04,0x0461); /* advertisement 10M full duplex, pause capable on */
#endif    
    mii_write(PHY_ADDR,0x00,0x1200);
	hal_delay_us(100000);
    while (((value=mii_read(PHY_ADDR,0x01)) & 0x00000004)!=0x04)
    {
    	hal_delay_us(1000);
        i++;
        if (i > 50)
        {
            break ;
        }
    }
	if (i > 50)
		diag_printf("Link Down\n");
	else
    	diag_printf("Link Up (%04x)\n",value);
    
    value = (mii_read(PHY_ADDR,0x05) & 0x05E0) >> 5;
    printk("PHY Status = %x \n", value);
    if ((value & 0x08)==0x08) /* 100M full duplex */
    {         
            full_duplex = 1;
            printk(" 100M/Full \n"); 
    }
    else if ((value & 0x04)==0x04) /* 100M half duplex */
    {               
            full_duplex = 0;
            printk(" 100M/Half \n"); 
    }
    else if ((value & 0x02)==0x02) /* 10M full duplex */
    {        
            full_duplex = 1;
            printk(" 10M/Full \n"); 
    }
    else if ((value & 0x01)==0x01) /* 10M half duplex */
    {
            full_duplex = 0;
            printk(" 10M/Half \n"); 
    }
    if ((value & 0x20)==0x20)
    {
        // flow_control_enable = 1;
        printk("Flow Control Enable. \n");
    }
    else
    {
        // flow_control_enable = 0;
        printk("Flow Control Disable. \n");
    }    

}
    
void emac_get_phy_status(void)
{
    EMAC_STATUS_T   status;
    unsigned int    reg_val;
    
    status.bits32 = 0;
    status.bits.phy_mode = 0;
    status.bits.mii_rmii = 0;
    /* read PHY status register */
    reg_val = mii_read(PHY_ADDR,0x01);
    if ((reg_val & 0x0024) == 0x0024) /* link is established and auto_negotiate process completed */
    {
        /* read PHY Auto-Negotiation Link Partner Ability Register */
        reg_val = (mii_read(PHY_ADDR,0x05) & 0x01E0) >> 5;
        switch (reg_val)
        {
            case 8: /* 100M full duplex */
                status.bits.duplex = 1;
                status.bits.speed = 1;
                break;
                
            case 4: /* 100M half duplex */
                status.bits.duplex = 0;
                status.bits.speed = 1;
                break;
                
            case 2: /* 10M full duplex */
                status.bits.duplex = 1;
                status.bits.speed = 0;
                break;
                
            case 1: /* 10M half duplex */
                status.bits.duplex = 0;
                status.bits.speed = 0;
                break;
        }
        status.bits.link = 1; /* link up */
    }
    else
    {
        status.bits.link = 0; /* link down */
    }
    status.bits.speed = 1; /* force EMAC to 100M */
    emac_write_reg(EMAC_STATUS,status.bits32,0x0000001f);
}    

/***************************************/
/* define GPIO module base address     */
/***************************************/
#define GPIO_BASE_ADDR      (0x21000000)
#define GPIO_MDC			(1 << BOARD_MDC_GPIO_PIN)	// 0x80000000
#define GPIO_MDIO			(1 << BOARD_MDIO_GPIO_PIN)	// 0x00400000
    
enum GPIO_REG
{
    GPIO_DATA_OUT   = 0x00,
    GPIO_DATA_IN    = 0x04,
    GPIO_PIN_DIR    = 0x08,
    GPIO_BY_PASS    = 0x0c,
    GPIO_DATA_SET   = 0x10,
    GPIO_DATA_CLEAR = 0x14,
};
/***********************/
/*    MDC : GPIO[31]   */
/*    MDIO: GPIO[22]   */
/***********************/
      
/***************************************************
* All the commands should have the frame structure:
*<PRE><ST><OP><PHYAD><REGAD><TA><DATA><IDLE>
****************************************************/

/*****************************************************************
* Inject a bit to NWay register through CSR9_MDC,MDIO
*******************************************************************/
void mii_serial_write(char bit_MDO) // write data into mii PHY
{
#if 1
    unsigned int addr;
    unsigned int value;

    addr = GPIO_BASE_ADDR + GPIO_PIN_DIR;
    value = readl(addr) | GPIO_MDC | GPIO_MDIO; /* set MDC/MDIO Pin to output */
    writel(value,addr);
    if(bit_MDO)
    {
        addr = (GPIO_BASE_ADDR + GPIO_DATA_SET);
        writel(GPIO_MDIO,addr); /* set MDIO to 1 */
        addr = (GPIO_BASE_ADDR + GPIO_DATA_SET);
        writel(GPIO_MDC,addr); /* set MDC to 1 */
        addr = (GPIO_BASE_ADDR + GPIO_DATA_CLEAR);
        writel(GPIO_MDC,addr); /* set MDC to 0 */                    
    }
    else
    {
        addr = (GPIO_BASE_ADDR + GPIO_DATA_CLEAR);
        writel(GPIO_MDIO,addr); /* set MDIO to 0 */
        addr = (GPIO_BASE_ADDR + GPIO_DATA_SET);
        writel(GPIO_MDC,addr); /* set MDC to 1 */
        addr = (GPIO_BASE_ADDR + GPIO_DATA_CLEAR);
        writel(GPIO_MDC,addr); /* set MDC to 0 */                    
    }   
#else    
    unsigned int *addr;
    unsigned int value;

    addr = (unsigned int *)(GPIO_BASE_ADDR + GPIO_PIN_DIR);
    value = (*addr) | 0x80400000;   /* set MDC/MDIO Pin to output */
    *addr = value;
    if(bit_MDO){
        addr = (unsigned int *)(GPIO_BASE_ADDR + GPIO_DATA_SET);
        *addr = 0x00400000; /* set MDIO to 1 */
        addr = (unsigned int *)(GPIO_BASE_ADDR + GPIO_DATA_SET);
        *addr = 0x80000000; /* set MDC to 1 */
        addr = (unsigned int *)(GPIO_BASE_ADDR + GPIO_DATA_CLEAR);
        *addr = 0x80000000; /* set MDC to 0 */                    
    }
    else{
        addr = (unsigned int *)(GPIO_BASE_ADDR + GPIO_DATA_CLEAR);
        *addr = 0x00400000; /* set MDIO to 0 */
        addr = (unsigned int *)(GPIO_BASE_ADDR + GPIO_DATA_SET);
        *addr = 0x80000000; /* set MDC to 1 */
        addr = (unsigned int *)(GPIO_BASE_ADDR + GPIO_DATA_CLEAR);
        *addr = 0x80000000; /* set MDC to 0 */                    
    }   
    return ;
#endif    
}

/**********************************************************************
* read a bit from NWay register through CSR9_MDC,MDIO
***********************************************************************/
unsigned int mii_serial_read(void) // read data from mii PHY
{
#if 1
    unsigned int addr;
    unsigned int value;
    
    addr = (GPIO_BASE_ADDR + GPIO_PIN_DIR);
    value = readl(addr) | GPIO_MDC & (~GPIO_MDIO); //0xffbfffff;   /* set MDC to output and MDIO to input */
    writel(value,addr);
    
    addr = (GPIO_BASE_ADDR + GPIO_DATA_SET);
    writel(GPIO_MDC,addr); /* set MDC to 1 */
    addr = (GPIO_BASE_ADDR + GPIO_DATA_CLEAR);
    writel(GPIO_MDC,addr); /* set MDC to 0 */                    

    addr = (GPIO_BASE_ADDR + GPIO_DATA_IN);
    value = readl(addr);
    value = (value & GPIO_MDIO) >> BOARD_MDIO_GPIO_PIN;
    return(value);
#else    
    unsigned int *addr;
    unsigned int value;
    
    addr = (unsigned int *)(GPIO_BASE_ADDR + GPIO_PIN_DIR);
    value = (*addr) & 0xffbfffff;   /* set MDC to output and MDIO to input */
    *addr = value;
    
    addr = (unsigned int *)(GPIO_BASE_ADDR + GPIO_DATA_SET);
    *addr = 0x80000000; /* set MDC to 1 */
    addr = (unsigned int *)(GPIO_BASE_ADDR + GPIO_DATA_CLEAR);
    *addr = 0x80000000; /* set MDC to 0 */                    

    value = *(unsigned int *)(GPIO_BASE_ADDR + GPIO_DATA_IN);
    value = (value & 0x00400000) >> 22;
    return(value);
#endif    
}

/***************************************
* preamble + ST
***************************************/
void mii_pre_st(void)
{
    unsigned char i;

    for(i=0;i<32;i++) // PREAMBLE
        mii_serial_write(1);
    mii_serial_write(0); // ST
    mii_serial_write(1);
}


/******************************************
* Read MII register
* phyad -> physical address
* regad -> register address
***************************************** */
unsigned int mii_read(unsigned char phyad,unsigned char regad)
{
    unsigned int i,value;
    unsigned int bit;
    
    mii_pre_st(); // PRE+ST
    mii_serial_write(1); // OP
    mii_serial_write(0);

    for (i=0;i<5;i++) { // PHYAD
        bit= ((phyad>>(4-i)) & 0x01) ? 1 :0 ;
        mii_serial_write(bit);
    }
    
    for (i=0;i<5;i++) { // REGAD
        bit= ((regad>>(4-i)) & 0x01) ? 1 :0 ;
        mii_serial_write(bit);
    }    

    mii_serial_read(); // TA_Z
//    if((bit=mii_serial_read()) !=0 ) // TA_0
//    {
//        return(0);        
//    }
    value=0;
    for (i=0;i<16;i++) { // READ DATA
        bit=mii_serial_read();
        value += (bit<<(15-i)) ;
    }
    
    mii_serial_write(0); // dumy clock
    mii_serial_write(0); // dumy clock
    return(value);
}

/******************************************
* Write MII register
* phyad -> physical address
* regad -> register address
* value -> value to be write
***************************************** */
void mii_write(unsigned char phyad,unsigned char regad,unsigned int value)
{
    unsigned int i;
    char bit;
    
    mii_pre_st(); // PRE+ST
    mii_serial_write(0); // OP
    mii_serial_write(1);
    for (i=0;i<5;i++) { // PHYAD
        bit= ((phyad>>(4-i)) & 0x01) ? 1 :0 ;
        mii_serial_write(bit);
    }
    
    for (i=0;i<5;i++) { // REGAD
        bit= ((regad>>(4-i)) & 0x01) ? 1 :0 ;
        mii_serial_write(bit);
    }
    mii_serial_write(1); // TA_1
    mii_serial_write(0); // TA_0
    
    for (i=0;i<16;i++) { // OUT DATA
        bit= ((value>>(15-i)) & 0x01) ? 1 : 0 ;
        mii_serial_write(bit);
    }
    mii_serial_write(0); // dumy clock
    mii_serial_write(0); // dumy clock
}









/*				NOTES
 *   The instruction set of the 93C66/56/46/26/06 chips are as follows:
 *
 *               Start  OP	    *
 *     Function   Bit  Code  Address**  Data     Description
 *     -------------------------------------------------------------------
 *     READ        1    10   A7 - A0             Reads data stored in memory,
 *                                               starting at specified address
 *     EWEN        1    00   11XXXXXX            Write enable must precede
 *                                               all programming modes
 *     ERASE       1    11   A7 - A0             Erase register A7A6A5A4A3A2A1A0
 *     WRITE       1    01   A7 - A0   D15 - D0  Writes register
 *     ERAL        1    00   10XXXXXX            Erase all registers
 *     WRAL        1    00   01XXXXXX  D15 - D0  Writes to all registers
 *     EWDS        1    00   00XXXXXX            Disables all programming
 *                                               instructions
 *    *Note: A value of X for address is a don't care condition.
 *    **Note: There are 8 address bits for the 93C56/66 chips unlike
 *	      the 93C46/26/06 chips which have 6 address bits.
 *
 *   The 93Cx6 has a four wire interface: clock, chip select, data in, and
 *   data out.While the ADM6996 uning three interface: clock, chip select,and data line.
 *   The input and output are the same pin. ADM6996 can only recognize the write cmd.
 *   In order to perform above functions, you need 
 *   1. to enable the chip select .
 *   2. send one clock of dummy clock
 *   3. send start bit and opcode
 *   4. send 8 bits address and 16 bits data
 *   5. to disable the chip select.
 *							Jason Lee 2003/07/30
 */

/***************************************/
/* define GPIO module base address     */
/***************************************/
#define GPIO_EECS	     0x04000000		/*   EECS: GPIO[22]   */
#define GPIO_MISO	     0x02000000         /*   EEDI: GPIO[30]   receive from 6996*/                         
#define GPIO_EECK	     0x01000000         /*   EECK: GPIO[31]   */


/*************************************************************
* SPI protocol for ADM6996 control
**************************************************************/
#define SPI_OP_LEN	     0x03		// the length of start bit and opcode
#define SPI_OPWRITE	     0X05		// write
#define SPI_OPREAD	     0X06		// read
#define SPI_OPERASE	     0X07		// erase
#define SPI_OPWTEN	     0X04		// write enable
#define SPI_OPWTDIS	     0X04		// write disable
#define SPI_OPERSALL	     0X04		// erase all
#define SPI_OPWTALL	     0X04		// write all

// #define CONFIG_ADM_6996	1		// move to board_config.h
// #define CONFIG_ADM_6999	1		// move to board_config.h
#define SPI_ADD_LEN	     8			// bits of Address
#define SPI_DAT_LEN	     16			// bits of Data
#define ADM6996_PORT_NO	     6			// the port number of ADM6996
#define ADM6999_PORT_NO	     9			// the port number of ADM6999
#define ADM699X_PORT_NO		ADM6996_PORT_NO
#define PORT0	0

#define PORT1	1
#define PORT2	2
#define PORT3	3
#define PORT4	4
#define PORT5	5

/****************************************/
/*	Function Declare		*/
/****************************************/
/*
void SPI_write(unsigned char addr,unsigned int value);
unsigned int SPI_read(unsigned char table,unsigned char addr);
void SPI_write_bit(char bit_EEDO);
unsigned int SPI_read_bit(void);
void SPI_default(int vlan_enabled);
void SPI_reset(unsigned char rstype,unsigned char port_cnt);
void SPI_pre_st(void);
void SPI_CS_enable(unsigned char enable);
void SPI_Set_VLAN(unsigned char LAN,unsigned int port_mask);
void SPI_Set_tag(unsigned int port,unsigned tag);
void SPI_Set_PVID(unsigned int PVID,unsigned int port_mask);
void SPI_mac_lock(unsigned int port, unsigned char lock);
void SPI_get_port_state(unsigned int port);
void SPI_port_enable(unsigned int port,unsigned char enable);

void SPI_get_status(unsigned int port);
*/

struct PORT_CONFIG
{
	unsigned char auto_negotiation;	// 0:Disable	1:Enable
	unsigned char speed;		// 0:10M	1:100M
	unsigned char duplex;		// 0:Half	1:Full duplex
	unsigned char Tag;		// 0:Untag	1:Tag
	unsigned char port_disable;	// 0:port enable	1:disable
	unsigned char pvid;		// port VLAN ID 0001
	unsigned char mdix;		// Crossover judgement. 0:Disable 1:Enable
	unsigned char mac_lock;		// MAC address Lock 0:Disable 1:Enable
};

struct PORT_STATUS
{
	unsigned char link;		// 0:not link	1:link established
	unsigned char speed;		// 0:10M	1:100M
	unsigned char duplex;		// 0:Half	1:Full duplex
	unsigned char flow_ctl;		// 0:flow control disable 1:enable
	unsigned char mac_lock;		// MAC address Lock 0:Disable 1:Enable
	unsigned char port_disable;	// 0:port enable	1:disable
	
	// Serial Management
	unsigned long rx_pac_count;		//receive packet count
	unsigned long rx_pac_byte;		//receive packet byte count
	unsigned long tx_pac_count;		//transmit packet count
	unsigned long tx_pac_byte;		//transmit packet byte count
	unsigned long collision_count;		//error count

	unsigned long rx_pac_count_overflow;		//overflow flag
	unsigned long rx_pac_byte_overflow;
	unsigned long tx_pac_count_overflow;
	unsigned long tx_pac_byte_overflow;
	unsigned long collision_count_overflow;
	unsigned long error_count ;
	unsigned long error_count_overflow;
};

struct PORT_CONFIG port_config[ADM6996_PORT_NO];	// 0~3:LAN , 4:WAN , 5:MII
static struct PORT_STATUS port_state[ADM6996_PORT_NO];

/******************************************
* SPI_write
* addr -> Write Address
* value -> value to be write
***************************************** */
void SPI_write(unsigned char addr,unsigned int value)
{
	int i,ad1;
	char bit, status;

#ifndef LPC_IT8712
   	ad1 = (GPIO_BASE_ADDR + GPIO_DATA_CLEAR);
	writel(GPIO_MISO,ad1); /* set MISO to 0 */
#endif
	
	SPI_CS_enable(1);
	
	SPI_write_bit(0);       //dummy clock
	
	//send write command (0x05)
	for(i=SPI_OP_LEN-1;i>=0;i--)
	{
		bit = (SPI_OPWRITE>>i)& 0x01;
		SPI_write_bit(bit);
	}
	// send 8 bits address (MSB first, LSB last)
	for(i=SPI_ADD_LEN-1;i>=0;i--)
	{
		bit = (addr>>i)& 0x01;
		SPI_write_bit(bit);
	}
	// send 16 bits data (MSB first, LSB last)
	for(i=SPI_DAT_LEN-1;i>=0;i--)
	{
		bit = (value>>i)& 0x01;
		SPI_write_bit(bit);
	}

	SPI_CS_enable(0);	// CS low

	for(i=0;i<0xFFF;i++) ;
#ifdef LPC_IT8712
	status = inb(LPC_BASE_ADDR + IT8712_GPIO_BASE + BASE_OFF);
	status &= ~(ADM_EDIO) ;		//EDIO low
	outb(LPC_BASE_ADDR + IT8712_GPIO_BASE + BASE_OFF,status);
#endif	
}


/************************************
* SPI_write_bit
* bit_EEDO -> 1 or 0 to be written
************************************/
void SPI_write_bit(char bit_EEDO)
{
#ifndef LPC_IT8712

	unsigned int addr;
	unsigned int value;

	addr = (GPIO_BASE_ADDR + GPIO_PIN_DIR);
	value = readl(addr) |GPIO_EECK |GPIO_MISO ;   // set EECK/MISO Pin to output 
	writel(value,addr);
	if(bit_EEDO)
	{
		addr = (GPIO_BASE_ADDR + GPIO_DATA_SET);
		writel(GPIO_MISO,addr); // set MISO to 1 
		writel(GPIO_EECK,addr); // set EECK to 1 
		addr = (GPIO_BASE_ADDR + GPIO_DATA_CLEAR);
		writel(GPIO_EECK,addr); // set EECK to 0 
	}
	else
	{
		addr = (GPIO_BASE_ADDR + GPIO_DATA_CLEAR);
		writel(GPIO_MISO,addr); // set MISO to 0 
		addr = (GPIO_BASE_ADDR + GPIO_DATA_SET);
		writel(GPIO_EECK,addr); // set EECK to 1 
		addr = (GPIO_BASE_ADDR + GPIO_DATA_CLEAR);
		writel(GPIO_EECK,addr); // set EECK to 0 
	}
#else
	unsigned char iomode,status;
	
	iomode = LPCGetConfig(LDN_GPIO, 0xc8 + BASE_OFF);
	iomode |= (ADM_EECK|ADM_EDIO|ADM_EECS) ;				// Set EECK,EDIO,EECS output
	LPCSetConfig(LDN_GPIO, 0xc8 + BASE_OFF, iomode);
	
	if(bit_EEDO)
	{
		status = inb(LPC_BASE_ADDR + IT8712_GPIO_BASE + BASE_OFF);
		status |= ADM_EDIO ;		//EDIO high
		outb(LPC_BASE_ADDR + IT8712_GPIO_BASE + BASE_OFF,status);	
	}
	else
	{
		status = inb(LPC_BASE_ADDR + IT8712_GPIO_BASE + BASE_OFF);
		status &= ~(ADM_EDIO) ;		//EDIO low
		outb(LPC_BASE_ADDR + IT8712_GPIO_BASE + BASE_OFF,status);
	}
	
	status |= ADM_EECK ;		//EECK high
	outb(LPC_BASE_ADDR + IT8712_GPIO_BASE + BASE_OFF,status);
	
	status &= ~(ADM_EECK) ;		//EECK low
	outb(LPC_BASE_ADDR + IT8712_GPIO_BASE + BASE_OFF,status);
#endif
}

/**********************************************************************
* read a bit from ADM6996 register
***********************************************************************/
unsigned int SPI_read_bit(void) // read data from
{
#ifndef LPC_IT8712
	unsigned int addr;
	unsigned int value;

	addr = (GPIO_BASE_ADDR + GPIO_PIN_DIR);
	value = readl(addr) & (~GPIO_MISO);   // set EECK to output and MISO to input
	writel(value,addr);

	addr =(GPIO_BASE_ADDR + GPIO_DATA_SET);
	writel(GPIO_EECK,addr); // set EECK to 1
	addr = (GPIO_BASE_ADDR + GPIO_DATA_CLEAR);
	writel(GPIO_EECK,addr); // set EECK to 0

	addr = (GPIO_BASE_ADDR + GPIO_DATA_IN);
	value = readl(addr) ;
	value = value >> 25;
	return value ;
#else
	unsigned char iomode,status;
	unsigned int value ;
	
	iomode = LPCGetConfig(LDN_GPIO, 0xc8 + BASE_OFF);
	iomode &= ~(ADM_EDIO) ;		// Set EDIO input
	iomode |= (ADM_EECS|ADM_EECK) ;		// Set EECK,EECS output
	LPCSetConfig(LDN_GPIO, 0xc8 + BASE_OFF, iomode);
	
	status = inb(LPC_BASE_ADDR + IT8712_GPIO_BASE + BASE_OFF);
	status |= ADM_EECK ;		//EECK high
	outb(LPC_BASE_ADDR + IT8712_GPIO_BASE + BASE_OFF,status);
		
	status &= ~(ADM_EECK) ;		//EECK low
	outb(LPC_BASE_ADDR + IT8712_GPIO_BASE + BASE_OFF,status);
	
	value = inb(LPC_BASE_ADDR + IT8712_GPIO_BASE + BASE_OFF);
	
	value = value>>2 ;
	value &= 0x01;
		
	return value ;
#endif
}

/******************************************
* SPI_default
* EEPROM content default value
*******************************************/
void SPI_default(int vlan_enabled)
{
	int i;
#ifdef CONFIG_ADM_6999
	if(chip_id >= 0xA3 || !vlan_enabled)
	{		//for A3 and later chip, redboot did not setup vlan
		for(i=1;i<8;i++)
			SPI_write(i,0x840F);	//	enable auto-crossover
		//SPI_write(0x08,0x842F);		
		SPI_write(0x08,0x840F);		
		return ;
	}
	else
	{
		SPI_write(0x11,0xFF30);
		for(i=1;i<8;i++)
			SPI_write(i,0x840F);
	
		SPI_write(0x08,0x880F);			//port 8 Untag, PVID=2
		SPI_write(0x09,0x881D);			//port 9 Tag, PVID=2 ,10M
		SPI_write(0x14,0x017F);			//Group 0~6,8 as VLAN 1
		SPI_write(0x15,0x0180);			//Group 7,8 as VLAN 2
	}
#endif

#ifdef CONFIG_ADM_6996
	if(chip_id > =0xA3  || !vlan_enabled)
	{		//for A3 and later chip, redboot did not setup vlan
		for(i=1;i<8;i+=2)
			SPI_write(i,0x840F);	//	enable auto-crossover
		//SPI_write(0x08,0x842F);		
		SPI_write(0x08,0x840F);		
		return ;
	}
	else
	{
		SPI_write(0x11,0xFF30);
		SPI_write(0x01,0x840F);			//port 0~3 Untag ,PVID=1 ,100M ,duplex
		SPI_write(0x03,0x840F);
		SPI_write(0x05,0x840F);
		SPI_write(0x07,0x840F);
		SPI_write(0x08,0x880F);			//port 4 Untag, PVID=2
		SPI_write(0x09,0x881D);			//port 5 Tag, PVID=2 ,10M
		SPI_write(0x14,0x0155);			//Group 0~3,5 as VLAN 1
		SPI_write(0x15,0x0180);			//Group 4,5 as VLAN 2
	}
#endif	
	
	for(i=0x16;i<=0x22;i++)
		SPI_write((unsigned char)i,0x0000);		// clean VLAN¡@map 3~15 
		
	for (i=0;i<NUM_VLAN_IF;i++) 				// Set VLAN ID map 1,2
		SPI_Set_PVID( VLAN_conf[i].vid,  VLAN_conf[i].portmap);
	
	for(i=0;i<ADM699X_PORT_NO;i++)				// reset count
		SPI_reset(0,i);

}

/*************************************************
* SPI_reset
* rstype -> reset type
*	    0:reset all count for 'port_cnt' port
*	    1:reset specified count 'port_cnt'
* port_cnt   ->  port number or counter index
***************************************************/
void SPI_reset(unsigned char rstype,unsigned char port_cnt)
{
	
	int i,add;
	char bit,status;
	
//	add = (GPIO_BASE_ADDR + GPIO_DATA_CLEAR);
//	writel(GPIO_MISO,add); /* set MISO to 0 */
	
	SPI_CS_enable(0);	// CS low
	
	SPI_pre_st(); // PRE+ST
	SPI_write_bit(0); // OP
	SPI_write_bit(1);

	SPI_write_bit(1);		// Table select, must be 1 -> reset Counter

	SPI_write_bit(0);		// Device Address
	SPI_write_bit(0);

	rstype &= 0x01;
	SPI_write_bit(rstype);		// Reset type 0:clear dedicate port's all counters 1:clear dedicate counter

	for (i=5;i>=0;i--) 		// port or cnt index
	{ 
		bit = port_cnt >> i ;
		bit &= 0x01 ;
		SPI_write_bit(bit);
	}

	SPI_write_bit(0); 		// dumy clock
	SPI_write_bit(0); 		// dumy clock
	
	status = inb(LPC_BASE_ADDR + IT8712_GPIO_BASE + BASE_OFF);
	status &= ~(ADM_EDIO) ;		//EDIO low
	outb(LPC_BASE_ADDR + IT8712_GPIO_BASE + BASE_OFF,status);
}

/*****************************************************
* SPI_pre_st
* preambler: 32 bits '1'   start bit: '01'
*****************************************************/
void SPI_pre_st(void)
{
	int i;

	for(i=0;i<32;i++) // PREAMBLE
		SPI_write_bit(1);
	SPI_write_bit(0); // ST
	SPI_write_bit(1);
}


/***********************************************************
* SPI_CS_enable
* before access ,you have to enable Chip Select. (pull high)
* When fisish, you should pull low !!
*************************************************************/
void SPI_CS_enable(unsigned char enable)
{
#ifndef LPC_IT8712
	unsigned int addr,value;

	addr = (GPIO_BASE_ADDR + GPIO_PIN_DIR);
	value = readl(addr) |GPIO_EECS |GPIO_EECK;   // set EECS/EECK Pin to output 
	writel(value,addr);

	if(enable)
	{
		addr = (GPIO_BASE_ADDR + GPIO_DATA_SET);
		writel(GPIO_EECS,addr); // set EECS to 1 
		
	}
	else
	{
		addr = (GPIO_BASE_ADDR + GPIO_DATA_CLEAR);
		writel(GPIO_EECS,addr); // set EECS to 0 
		addr = (GPIO_BASE_ADDR + GPIO_DATA_SET);
		writel(GPIO_EECK,addr); // set EECK to 1 	// at least one clock after CS low
		addr = (GPIO_BASE_ADDR + GPIO_DATA_CLEAR);
		writel(GPIO_EECK,addr); // set EECK to 0 
	}
#else
	unsigned char iomode,status;
	
	iomode = LPCGetConfig(LDN_GPIO, 0xc8 + BASE_OFF);
	iomode |= (ADM_EECK|ADM_EDIO|ADM_EECS) ;				// Set EECK,EDIO,EECS output
	LPCSetConfig(LDN_GPIO, 0xc8 + BASE_OFF, iomode);
	
	
	status = inb(LPC_BASE_ADDR + IT8712_GPIO_BASE + BASE_OFF);
	if(enable)
		status |= ADM_EECS ;		//EECS high
	else
		status &= ~(ADM_EECS) ;	//EECS low
		
	outb(LPC_BASE_ADDR + IT8712_GPIO_BASE + BASE_OFF,status);

		
	status |= ADM_EECK ;		//EECK high
	outb(LPC_BASE_ADDR + IT8712_GPIO_BASE + BASE_OFF,status);
		
	status &= ~(ADM_EECK) ;		//EECK low
	outb(LPC_BASE_ADDR + IT8712_GPIO_BASE + BASE_OFF,status);
#endif	
}

/*********************************************************
* SPI_Set_VLAN: group ports as VLAN
* LAN  -> VLAN number : 0~16
* port_mask -> ports which would group as LAN
* 	       ex. 0x03 = 0000 0011
*			port 0 and port 1 
*********************************************************/
void SPI_Set_VLAN(unsigned char LAN,unsigned int port_mask)
{
	unsigned int i,value=0;
	unsigned reg_add = 0x13 + LAN ;
	
	for(i=0;i<ADM6996_PORT_NO;i++)
	{	if(port_mask&0x01)
		{
			switch(i)
			{
				case 0: value|=0x0001;	break;	//port0:bit[0]
				case 1: value|=0x0004;	break;  //port1:bit[2]
				case 2: value|=0x0010;	break;  //port2:bit[4]
				case 3: value|=0x0040;	break;  //port3:bit[6]
				case 4: value|=0x0080;	break;  //port4:bit[7]
				case 5: value|=0x0100;	break;  //port5:bit[8]
			}
		}
		port_mask >>= 1;
	}
	
	SPI_write(reg_add,value);
}


/*******************************************
* SPI_Set_tag
* port -> port number to set tag or untag
* tag  -> 0/set untag,  1/set tag
* In general, tag is for MII port. LAN and
* WAN port is configed as untag!!
********************************************/
void SPI_Set_tag(unsigned int port,unsigned tag)
{
	unsigned int regadd,value;
	
	// mapping port's register !! (0,1,2,3,4,5) ==> (1,3,5,7,8,9)
	if(port<=3)
		regadd=2*port+1;
	else if(port==4) regadd = 8 ;
	else regadd = 9 ;
		
	
	value = SPI_read(0,regadd);		//read original setting 
	
	if(tag)
		value |= 0x0010 ;		// set tag 
	else
		value &= 0xFFEF ;		// set untag
	
	SPI_write(regadd,value);		// write back!!
}

/************************************************
* SPI_Set_PVID
* PVID -> PVID number : 
* port_mask -> ports which would group as LAN
* 	       ex. 0x0F = 0000 1111 ==> port 0~3 
************************************************/
void SPI_Set_PVID(unsigned int PVID,unsigned int port_mask)
{
	unsigned int i,value=0;
	
	PVID &= 0x000F ;

	for(i=0;i<ADM699X_PORT_NO;i++)
	{	if(port_mask&0x01)
		{
#ifdef CONFIG_ADM_6996
			switch(i)
			{
				case 0: 
					value = SPI_read(0,0x01);	// read original value
					value &= 0xC3FF ;			//set PVIC column as 0 first
					value |= PVID << 10 ;		//Set PVID column as PVID
					SPI_write(0x01,value);		//write back
					break;
				case 1: 
					value = SPI_read(0,0x03);
					value &= 0xC3FF ;
					value |= PVID << 10 ;
					SPI_write(0x03,value);
					break;
				case 2: 
					value = SPI_read(0,0x05);
					value &= 0xC3FF ;
					value |= PVID << 10 ;
					SPI_write(0x05,value);
					break;
				case 3: 
					value = SPI_read(0,0x07);
					value &= 0xC3FF ;
					value |= PVID << 10 ;
					SPI_write(0x07,value);
					break;
				case 4: 
					value = SPI_read(0,0x08);
					value &= 0xC3FF ;
					value |= PVID << 10 ;
					SPI_write(0x08,value);	
					break;
				case 5: 
					value = SPI_read(0,0x09);
					value &= 0xC3FF ;	
					value |= PVID << 10 ;
					SPI_write(0x09,value);
					break;
			}
#endif
#ifdef CONFIG_ADM_6999			
			value = SPI_read(0,(unsigned char)i+1);
			value &= 0xC3FF ;	
			value |= PVID << 10 ;
			SPI_write((unsigned char)i+1,value);
#endif
		}
		port_mask >>= 1;
	}
}


/************************************************
* SPI_get_PVID
* port -> which ports to VID
************************************************/
unsigned int SPI_Get_PVID(unsigned int port)
{
	unsigned int value=0;
	
	if (port>=ADM6996_PORT_NO)
		return 0;
		
	switch(port)
	{
		case 0: 
			value = SPI_read(0,0x01);	// read original value
			value &= 0x3C00 ;		// get VID 
			value = value >> 10 ;		// Shift
			break;
		case 1: 
			value = SPI_read(0,0x03);	
			value &= 0x3C00 ;		
			value = value >> 10 ;		
			break;
		case 2: 
			value = SPI_read(0,0x05);	
			value &= 0x3C00 ;		
			value = value >> 10 ;		
			break;
		case 3: 
			value = SPI_read(0,0x07);	
			value &= 0x3C00 ;		
			value = value >> 10 ;		
			break;
		case 4: 
			value = SPI_read(0,0x08);	
			value &= 0x3C00 ;		
			value = value >> 10 ;		
			break;
		case 5: 
			value = SPI_read(0,0x09);	
			value &= 0x3C00 ;		
			value = value >> 10 ;		
			break;
	}
	return value ;
}


/**********************************************
* SPI_mac_clone
* port -> the port which will lock or unlock
* lock -> 0/the port will be unlock   	
*	  1/the port will be locked
**********************************************/
void SPI_mac_lock(unsigned int port, unsigned char lock)
{
	unsigned int i,value=0;
	
	value = SPI_read(0,0x12);		// read original 
	
	for(i=0;i<ADM6996_PORT_NO;i++)
	{	if(lock)				// lock port
		{
			switch(port)
			{
				case 0: value|=0x0001;	break;	//port0:bit[0]
				case 1: value|=0x0004;	break;	//port1:bit[2]
				case 2: value|=0x0010;	break;	//port2:bit[4]
				case 3: value|=0x0040;	break;	//port3:bit[6]
				case 4: value|=0x0080;	break;	//port4:bit[7]
				case 5: value|=0x0100;	break;	//port5:bit[8]
			}
		}
		else
		{
			switch(i)			// unlock port
			{
				case 0: value&=0xFFFE;	break;
				case 1: value&=0xFFFB;	break;  
				case 2: value&=0xFFEF;	break;  
				case 3: value&=0xFFBF;	break;  
				case 4: value&=0xFF7F;	break;  
				case 5: value&=0xFEFF;	break;  
			}
		}
	}
	
	SPI_write(0x12,value);
}


/***************************************************
* SPI_learn_pause
* pause = 01-80-c2-00-00-01
* DA=distination address
* forward -> 0: if DA == pause then drop and stop mac learning
*	     1: if DA == pause ,then forward it
***************************************************/
void SPI_pause_cmd_forward(unsigned char forward)
{
	unsigned int value=0;
	
	value = SPI_read(0,0x2C);		// read original setting
	if(forward)
		value |= 0x2000;		// set bit[13] '1'
	else
		value &= 0xDFFF;		// set bit[13] '0'
	
	SPI_write(0x2C,value);
	
}


/************************************************
* SPI_read
* table -> which table to be read: 1/count  0/EEPROM
* addr  -> Address to be read
* return : Value of the register
*************************************************/
unsigned int SPI_read(unsigned char table,unsigned char addr)
{
	int i ;
	unsigned int ad1,value=0;
	unsigned int bit,status,iomode;

#ifndef LPC_IT8712	
	ad1 = (GPIO_BASE_ADDR + GPIO_DATA_CLEAR);
	writel(GPIO_MISO,ad1); /* set MISO to 0 */
#else
	status = inb(LPC_BASE_ADDR + IT8712_GPIO_BASE + BASE_OFF);
	status &= ~(ADM_EDIO) ;		//EDIO low
	outb(LPC_BASE_ADDR + IT8712_GPIO_BASE + BASE_OFF,status);
#endif
	
	SPI_CS_enable(0);
	
	SPI_pre_st(); // PRE+ST
	SPI_write_bit(1); // OPCODE '10' for read
	SPI_write_bit(0);
	
	(table==1) ? SPI_write_bit(1) : SPI_write_bit(0) ;	// table select

	SPI_write_bit(0);		// Device Address
	SPI_write_bit(0);


	// send 7 bits address to be read
	for (i=6;i>=0;i--) {
		bit= ((addr>>i) & 0x01) ? 1 :0 ;
		SPI_write_bit(bit);
	}


	// turn around
	SPI_read_bit(); // TA_Z

	value=0;
	for (i=31;i>=0;i--) { // READ DATA
		bit=SPI_read_bit();
		value |= bit << i ;
	}

	SPI_read_bit(); // dumy clock
	SPI_read_bit(); // dumy clock
    
	if(!table)					// EEPROM, only fetch 16 bits data
	{
	    if(addr&0x01)				// odd number content (register,register-1)
		    value >>= 16 ;			// so we remove the rear 16bits
	    else					// even number content (register+1,register),
		    value &= 0x0000FFFF ;		// so we keep the rear 16 bits
	}

	
	SPI_CS_enable(0);

#ifdef LPC_IT8712	
	iomode = LPCGetConfig(LDN_GPIO, 0xc8 + BASE_OFF);
	iomode |= (ADM_EECK|ADM_EDIO|ADM_EECS) ;		// Set EECK,EECS,EDIO output
	LPCSetConfig(LDN_GPIO, 0xc8 + BASE_OFF, iomode);
	
	status = inb(LPC_BASE_ADDR + IT8712_GPIO_BASE + BASE_OFF);
	status &= ~(ADM_EDIO) ;		//EDIO low
	outb(LPC_BASE_ADDR + IT8712_GPIO_BASE + BASE_OFF,status);
#endif
	
	return(value);

}



/**************************************************
* SPI_port_en
* port -> Number of port to config
* enable -> 1/ enable this port
*	    0/ disable this port
**************************************************/
void SPI_port_enable(unsigned int port,unsigned char enable)
{
	unsigned int reg_val ;
	unsigned char reg_add ;
	
	if(port<=3)
		reg_add=2*port+1;
	else if(port==4) reg_add = 8 ;
	else reg_add = 9 ;
	
	reg_val = SPI_read(0,reg_add);
	if(enable)
	{
		reg_val &= 0xFFDF ;
		SPI_write(reg_add,reg_val);
	}
	else
	{
		reg_val |= 0x0020 ;
		SPI_write(reg_add,reg_val);
	}
}

/********************************************************
* get port status
* port -> specify the port number to get configuration 
*********************************************************/
void SPI_get_status(unsigned int port)
{
	unsigned int reg_val,add_offset[6];
	struct PORT_STATUS *status;
	status = &port_state[port];
	
	if(port>(ADM6996_PORT_NO-1))
		return ;

	// Link estabilish , speed, deplex, flow control ?
	if(port < 5 )
	{	
		reg_val = SPI_read(1, 1) ;
		if(port < 4)
			reg_val >>= port*8 ;
		else
			reg_val >>=28 ;
		status->link = reg_val & 0x00000001 ;
		status->speed = reg_val  & 0x00000002 ;
		status->duplex = reg_val & 0x00000004 ;
		status->flow_ctl = reg_val & 0x00000008 ;
	}
	else if(port ==5 )
	{
		reg_val = SPI_read(1, 2) ;
		status->link = reg_val & 0x00000001 ;
		status->speed = reg_val  & 0x00000002 ;
		status->duplex = reg_val & 0x00000008 ;
		status->flow_ctl = reg_val & 0x00000010 ;
	}

	//   Mac Lock ?
	reg_val = SPI_read(0,0x12);
	switch(port)
	{
		case 0:	status->mac_lock = reg_val & 0x00000001;
		case 1:	status->mac_lock = reg_val & 0x00000004;
		case 2:	status->mac_lock = reg_val & 0x00000010;
		case 3:	status->mac_lock = reg_val & 0x00000040;
		case 4:	status->mac_lock = reg_val & 0x00000080;
		case 5:	status->mac_lock = reg_val & 0x00000100;
	}

	// port enable ?
	add_offset[0] = 0x01 ;		add_offset[1] = 0x03 ;
	add_offset[2] = 0x05 ;		add_offset[3] = 0x07 ;
	add_offset[4] = 0x08 ;		add_offset[5] = 0x09 ;
	reg_val = SPI_read(0,add_offset[port]);
	status->port_disable = reg_val & 0x0020;


	//  Packet Count ...
	add_offset[0] = 0x04 ;		add_offset[1] = 0x06 ;
	add_offset[2] = 0x08 ;		add_offset[3] = 0x0a ;
	add_offset[4] = 0x0b ;		add_offset[5] = 0x0c ;
	
	reg_val = SPI_read(1,add_offset[port]);
	status->rx_pac_count = reg_val ;
	reg_val = SPI_read(1,add_offset[port]+9);
	status->rx_pac_byte = reg_val ;
	reg_val = SPI_read(1,add_offset[port]+18);
	status->tx_pac_count = reg_val ;
	reg_val = SPI_read(1,add_offset[port]+27);
	status->tx_pac_byte = reg_val ;
	reg_val = SPI_read(1,add_offset[port]+36);
	status->collision_count = reg_val ;
	reg_val = SPI_read(1,add_offset[port]+45);
	status->error_count = reg_val ;
	reg_val = SPI_read(1, 0x3A);
	switch(port)
	{
		case 0:	status->rx_pac_count_overflow = reg_val & 0x00000001;
			status->rx_pac_byte_overflow = reg_val & 0x00000200 ;
		case 1:	status->rx_pac_count_overflow = reg_val & 0x00000004;
			status->rx_pac_byte_overflow = reg_val & 0x00000800 ;
		case 2:	status->rx_pac_count_overflow = reg_val & 0x00000010;
			status->rx_pac_byte_overflow = reg_val & 0x00002000 ;
		case 3:	status->rx_pac_count_overflow = reg_val & 0x00000040;;
			status->rx_pac_byte_overflow = reg_val & 0x00008000 ;
		case 4:	status->rx_pac_count_overflow = reg_val & 0x00000080;
			status->rx_pac_byte_overflow = reg_val & 0x00010000 ;
		case 5:	status->rx_pac_count_overflow = reg_val & 0x00000100;
			status->rx_pac_byte_overflow = reg_val & 0x00020000 ;
	}

	reg_val = SPI_read(1, 0x3B);
	switch(port)
	{
		case 0:	status->tx_pac_count_overflow = reg_val & 0x00000001;
			status->tx_pac_byte_overflow  = reg_val & 0x00000200 ;
		case 1:	status->tx_pac_count_overflow  = reg_val & 0x00000004;
			status->tx_pac_byte_overflow  = reg_val & 0x00000800 ;
		case 2:	status->tx_pac_count_overflow  = reg_val & 0x00000010;
			status->tx_pac_byte_overflow  = reg_val & 0x00002000 ;
		case 3:	status->tx_pac_count_overflow  = reg_val & 0x00000040;;
			status->tx_pac_byte_overflow  = reg_val & 0x00008000 ;
		case 4:	status->tx_pac_count_overflow  = reg_val & 0x00000080;
			status->tx_pac_byte_overflow  = reg_val & 0x00010000 ;
		case 5:	status->tx_pac_count_overflow  = reg_val & 0x00000100;
			status->tx_pac_byte_overflow  = reg_val & 0x00020000 ;
	}

}

unsigned int SPI_get_identifier()
{
#ifdef LPC_IT8712	
	unsigned int flag=0;

	if(SearchIT8712())
	{	
		diag_printf("IT8712 not found!!");
		return;
	}
	// initialize registers 
	// switch multi-function pins to GPIO (Set 4)
	LPCSetConfig(LDN_GPIO, 0x28, 0xff);

	// set simple I/O base address
	LPCSetConfig(LDN_GPIO, 0x62, IT8712_GPIO_BASE >> 8);
	LPCSetConfig(LDN_GPIO, 0x63, (unsigned char) IT8712_GPIO_BASE >> 8);

	// select GPIO to simple I/O (Set 4)
	LPCSetConfig(LDN_GPIO, 0xc3, 0xff);

	// enable internal pull-up
	LPCSetConfig(LDN_GPIO, 0xbb, 0xff);	
	
	flag = SPI_read(1,0x00);
//	diag_printf("Get ADM identifier %6x\n",flag);
	if ((flag & 0xFFFF0) == 0x21120)
		return 1;
	else
#endif	
		return 0;
}

void outb(unsigned int addr, unsigned int value)
{
/*    unsigned int *address;
    
    address = (unsigned int *)addr;
    *address = value;
*/   
    HAL_WRITE_UINT8(addr, value);
}

unsigned char inb(unsigned int addr)
{
    unsigned int  value;
/*    
    value = *(unsigned int *)addr;
*/    
    HAL_READ_UINT8(addr, value);
    
    return (value);
}

int Get_MAC(unsigned char* mac)
{
#if 0
	char mac1[14],mac2[14], ans[4], tmp;
	int j,stat, res;
	void *err_addr;
	unsigned char *offset;
	vctl_mheader *newhead, *mhead;
	vctl_entry *newentry, *entry;

	flash_enable();
	memset(mac1, 0, sizeof(mac1));
	memset(mac2, 0, sizeof(mac2));
	memcpy(TMP_RAM_BUF1 , CYGNUM_FLASH_BASE+VERCTL_ADDR, VERCTLSIZE);

	mhead =(struct vctl_mheader *)(const unsigned int*)TMP_RAM_BUF1 ;
	entry = (struct vctl_entry *)(const unsigned int*)(TMP_RAM_BUF1+ sizeof(vctl_mheader)) ;
	offset = entry;
	flash_disable();
//	diag_printf("entry num %x\n",mhead->entry_num);
	if(mhead->entry_num>128)
		return 1;
	for(j=0;j< mhead->entry_num;j++) {
		entry = offset ;
//		diag_printf("type: %d\n",entry->type);
//		diag_printf("size: %d\n",entry->size);
		if(entry->type == VCT_VLAN){

			memcpy(mac1, offset+27, 12);
//			diag_printf("MAC1: %s\n",mac1);
			memcpy(mac2, offset+64, 12);
//			diag_printf("MAC2: %s\n",mac2);
			
			for(j=0;j<ETHER_ADDR_LEN*2;j+=2) {
				mac[j/2]= (asc_2_hex(mac1[j]) << 4) + asc_2_hex(mac1[j+1]);
//				diag_printf("%x-",mac[j]);
			}
//			diag_printf("\n");
			return 1;
		}
		else {
			offset += entry->size ;
		}
	}
	diag_printf("MAC not found!!\n");
#endif
	return 0;
}

unsigned char asc_2_hex(char asc)
{
	unsigned char hex=0x05;
	if((asc>='0')&&(asc<='9')) {
		hex = asc - '0';
	}
	if ((asc>='A')&&(asc<='F')) {
		hex = asc - 'A' + 10;
	}
	if ((asc>='a')&&(asc<='f')) {
		hex = asc - 'a' + 10;
	}

	return hex;
}
/*
void flash_enable()
{
	unsigned int *addr,i;
	addr = SL2312_GLOBAL_BASE + GLOBAL_MISC_CTRL ;
	for(i=0;i<1000;i++) ;
	
	*addr &= ~0x01 ;
}

void flash_disable()
{
	unsigned int *addr,i;
	addr = SL2312_GLOBAL_BASE + GLOBAL_MISC_CTRL ;
	for(i=0;i<1000;i++) ;
	
	*addr |= 0x01 ;
}
*/

#endif // MIDWAY

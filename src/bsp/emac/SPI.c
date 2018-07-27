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
#include <define.h>
//#include <board_config.h>
//#include <sl2312.h>

#define GPIO_BASE_ADDR  0x4E000000
#define SL2312_GLOBAL_BASE_ADDR              0x40000000  
/***************************************/
/* define GPIO module base address     */
/***************************************/
//#define GPIO_EECS	     0x80000000		/*   EECS: GPIO[22]   */
//#define GPIO_MOSI	     0x20000000         /*   EEDO: GPIO[29]   send to 6996*/
//#define GPIO_MISO	     0x40000000         /*   EEDI: GPIO[30]   receive from 6996*/
//#define GPIO_EECK	     0x10000000         /*   EECK: GPIO[31]   */

static unsigned int	GPIO_EECS=0;
static unsigned int	GPIO_MOSI=0;
static unsigned int	GPIO_MISO=0;
static unsigned int	GPIO_EECK=0;
static unsigned int	GPIO_MISO_BIT=0;
static unsigned int	LAN_NUM=0;

#define ADM_EECS		0x01
#define ADM_EECK		0x02
#define ADM_EDIO		0x04
/*************************************************************
* SPI protocol for ADM6996 control
**************************************************************/
#define SPI_OP_LEN	     0x08		// the length of start bit and opcode
#define SPI_OPWRITE	     0X05		// write
#define SPI_OPREAD	     0X06		// read
#define SPI_OPERASE	     0X07		// erase
#define SPI_OPWTEN	     0X04		// write enable
#define SPI_OPWTDIS	     0X04		// write disable
#define SPI_OPERSALL	     0X04		// erase all
#define SPI_OPWTALL	     0X04		// write all

#define SPI_ADD_LEN	     8			// bits of Address
#define SPI_DAT_LEN	     32			// bits of Data
#define ADM6996_PORT_NO	     6			// the port number of ADM6996
#define ADM6999_PORT_NO	     9			// the port number of ADM6999
#ifdef CONFIG_ADM_6996
	#define ADM699X_PORT_NO		ADM6996_PORT_NO
#endif
#ifdef CONFIG_ADM_6999
	#define ADM699X_PORT_NO		ADM6999_PORT_NO
#endif
#define LPC_GPIO_SET		3
#define LPC_BASE_ADDR			IO_ADDRESS(IT8712_IO_BASE)

extern int it8712_exist;

#define inb_gpio(x)			inb(LPC_BASE_ADDR + IT8712_GPIO_BASE + x)
#define outb_gpio(x, y)		outb(y, LPC_BASE_ADDR + IT8712_GPIO_BASE + x)

#define readl(addr)								REG32(addr)
#define writel(value, addr)						REG32(addr) = value

enum GPIO_REG
{
    GPIO_DATA_OUT   = 0x00,
    GPIO_DATA_IN    = 0x04,
    GPIO_PIN_DIR    = 0x08,
    GPIO_BY_PASS    = 0x0c,
    GPIO_DATA_SET   = 0x10,
    GPIO_DATA_CLEAR = 0x14,
};

/****************************************/
/*	Function Declare		*/
/****************************************/

unsigned int SPI_read_bit(void);
void SPI_write_bit(char bit_EEDO);
unsigned int SPI_read_bit(void);
void SPI_CS_enable(unsigned char enable);
void SPI_CS_enable(unsigned char enable);
unsigned int SPI_read_bit(void);
unsigned int SPI_read(unsigned char block,unsigned char subblock,unsigned char addr);
/******************************************
* SPI_write
* addr -> Write Address
* value -> value to be write
***************************************** */
void SPI_write(unsigned char block,unsigned char subblock,unsigned char addr,unsigned int value)
{
	int     i;
	char    bit;
	int     ad1;
	unsigned int data;
	unsigned int 		flag,tmp;

		flag = readl(SL2312_GLOBAL_BASE_ADDR + 0x04);	
		tmp = readl(SL2312_GLOBAL_BASE_ADDR + 0x0);	

		if((flag&0x40000000)&&((tmp&0x000000FF)==0xc3))
		{
				flag = readl(SL2312_GLOBAL_BASE_ADDR + 0x30);
				tmp = flag;
				flag &= ~(1 << 4);
				writel(flag,SL2312_GLOBAL_BASE_ADDR + 0x30);
		}
		else
			tmp = 0;
	
	SPI_CS_enable(1);
	
	data = (block<<5) | 0x10 | subblock;

	//send write command 
	for(i=SPI_OP_LEN-1;i>=0;i--)
	{
		bit = (data>>i)& 0x01;
		SPI_write_bit(bit);
	}
	
	// send 8 bits address (MSB first, LSB last)
	for(i=SPI_ADD_LEN-1;i>=0;i--)
	{
		bit = (addr>>i)& 0x01;
		SPI_write_bit(bit);
	}
	// send 32 bits data (MSB first, LSB last)
	for(i=SPI_DAT_LEN-1;i>=0;i--)
	{
		bit = (value>>i)& 0x01;
		SPI_write_bit(bit);
	}

	SPI_CS_enable(0);	// CS low

	if(tmp)
		writel(tmp,SL2312_GLOBAL_BASE_ADDR + 0x30);
}

void phy_write(unsigned char port_no,unsigned char reg,unsigned int val)
{
	unsigned int cmd;
	
	cmd = (port_no<<21)|(reg<<16)|val;
	SPI_write(3,0,1,cmd);
}

unsigned int phy_read(unsigned char port_no,unsigned char reg)
{
	unsigned int cmd,reg_val;
	
	cmd = BIT(26)|(port_no<<21)|(reg<<16);
	SPI_write(3,0,1,cmd);
	hal_delay_us(5 * 1000);
	reg_val = SPI_read(3,0,2);
	return reg_val;
}

void phy_write_masked(unsigned char port_no,unsigned char reg,unsigned int val,unsigned int mask)
{
	unsigned int cmd,reg_val;
	
	cmd = BIT(26)|(port_no<<21)|(reg<<16);	// Read reg_val
	SPI_write(3,0,1,cmd);
	hal_delay_us(2 * 1000);
	reg_val = SPI_read(3,0,2);
	reg_val &= ~mask;			// Clear masked bit
	reg_val |= (val&mask) ;			// set masked bit ,if true
	cmd = (port_no<<21)|(reg<<16)|reg_val;
	SPI_write(3,0,1,cmd);
}

void init_seq_7385(unsigned char port_no) 
{
	 unsigned char rev;

    phy_write(port_no, 31, 0x2a30);
	phy_write_masked(port_no, 8, 0x0200, 0x0200);
	phy_write(port_no, 31, 0x52b5);
	phy_write(port_no, 16, 0xb68a);
	phy_write_masked(port_no, 18, 0x0003, 0xff07);
	phy_write_masked(port_no, 17, 0x00a2, 0x00ff);
	phy_write(port_no, 16, 0x968a);
	phy_write(port_no, 31, 0x2a30);
	phy_write_masked(port_no, 8, 0x0000, 0x0200);
	phy_write(port_no, 31, 0x0000); /* Read revision */ 
	rev = phy_read(port_no, 3) & 0x000f;
	//printf("Port %d PHY REV:%x\n",port_no,rev);
	if (rev == 0)
	{
		phy_write(port_no, 31, 0x2a30);
		phy_write_masked(port_no, 8, 0x0200, 0x0200);
		phy_write(port_no, 31, 0x52b5);
		phy_write(port_no, 18, 0x0000);
		phy_write(port_no, 17, 0x0689);
		phy_write(port_no, 16, 0x8f92);
		phy_write(port_no, 31, 0x52B5);
		phy_write(port_no, 18, 0x0000);
		phy_write(port_no, 17, 0x0E35);
		phy_write(port_no, 16, 0x9786);
		phy_write(port_no, 31, 0x2a30);
		phy_write_masked(port_no, 8, 0x0000, 0x0200);
		phy_write(port_no, 23, 0xFF80);
		phy_write(port_no, 23, 0x0000);
	}
	phy_write(port_no, 31, 0x0000);
	phy_write(port_no, 18, 0x0048);
	if (rev == 0)
	{
		phy_write(port_no, 31, 0x2a30);
		phy_write(port_no, 20, 0x6600);
		phy_write(port_no, 31, 0x0000);
		phy_write(port_no, 24, 0xa24e);
	}
	else
	{
		phy_write(port_no, 31, 0x2a30);
		phy_write_masked(port_no, 22, 0x0240, 0x0fc0);
		phy_write_masked(port_no, 20, 0x4000, 0x6000);
		phy_write(port_no, 31, 1);
		phy_write_masked(port_no, 20, 0x6000, 0xe000);
		phy_write(port_no, 31, 0x0000);
	}
}

void phy_receiver_init (unsigned char port_no)
{
    phy_write(port_no,31,0x2a30);
    phy_write_masked(port_no, 12, 0x0200, 0x0300);
    phy_write(port_no,31,0);
}

void phy_receiver_reconfig (unsigned char port_no)
{
    unsigned char vga_state_a;

    phy_write(port_no, 31, 0x52b5);
    phy_write(port_no, 16, 0xaff0);
    vga_state_a = (phy_read(port_no, 17) >> 4) & 0x01f;
    if ((vga_state_a < 16) || (vga_state_a > 20)) {
        phy_write(port_no, 31, 0x2a30);
        phy_write_masked(port_no, 12, 0x0000, 0x0300);
    }
    phy_write(port_no, 31, 0);
}

/************************************
* SPI_write_bit
* bit_EEDO -> 1 or 0 to be written
************************************/
void SPI_write_bit(char bit_EEDO)
{
	unsigned int addr;
	unsigned int value;

	addr = (GPIO_BASE_ADDR + GPIO_PIN_DIR);
	value = readl(addr) |GPIO_EECK |GPIO_MOSI ;   /* set EECK/MISO Pin to output */
	writel(value,addr);
	if(bit_EEDO)
	{
		addr = (GPIO_BASE_ADDR + GPIO_DATA_SET);
		writel(GPIO_MOSI,addr); /* set MISO to 1 */
		
	}
	else
	{
		addr = (GPIO_BASE_ADDR + GPIO_DATA_CLEAR);
		writel(GPIO_MOSI,addr); /* set MISO to 0 */
	}
	addr = (GPIO_BASE_ADDR + GPIO_DATA_SET);
	writel(GPIO_EECK,addr); /* set EECK to 1 */
	addr = (GPIO_BASE_ADDR + GPIO_DATA_CLEAR);
	writel(GPIO_EECK,addr); /* set EECK to 0 */

	//return ;
}

/**********************************************************************
* read a bit from ADM6996 register
***********************************************************************/
unsigned int SPI_read_bit(void) // read data from
{
	unsigned int addr;
	unsigned int value;

	addr = (GPIO_BASE_ADDR + GPIO_PIN_DIR);
	value = readl(addr) & (~GPIO_MISO);   // set EECK to output and MISO to input
	writel(value,addr);

	addr =(GPIO_BASE_ADDR + GPIO_DATA_SET);
	writel(GPIO_EECK,addr); // set EECK to 1


	addr = (GPIO_BASE_ADDR + GPIO_DATA_IN);
	value = readl(addr) ;

	addr = (GPIO_BASE_ADDR + GPIO_DATA_CLEAR);
	writel(GPIO_EECK,addr); // set EECK to 0
	
	
	value = (value&GPIO_MISO) >> GPIO_MISO_BIT;
	return value ;
}

/******************************************
* SPI_default
* EEPROM content default value
*******************************************/
void SPI_default(void)
{
	int i;
	unsigned reg_val,cmd;
	unsigned char timeout;
	
	hal_delay_us(30*1000);
	
	for(i=0;i<15;i++){
		if(i!=6 && i!=7)
			SPI_write(3,2,0,0x1010400+i);		// Initial memory
		hal_delay_us(1*1000);
	}
	
	hal_delay_us(30*1000);
			
	SPI_write(2,0,0xB0,0x05);			// Clear MAC table
	SPI_write(2,0,0xD0,0x03);			// Clear VLAN
	
	//for(i=0;i<5;i++)
	SPI_write(1,6,0x19,0x2C);			// Double Data rate
	
	for(i=0;i<LAN_NUM;i++){
		SPI_write(1,i,0x00,0x30050472);		// MAC configure
		SPI_write(1,i,0x00,0x10050442);		// MAC configure
		SPI_write(1,i,0x10,0x5F4);		// Max length
		SPI_write(1,i,0x04,0x00030000);		// Flow control
		SPI_write(1,i,0xDF,0x00000001);		// Flow control
		SPI_write(1,i,0x08,0x000050c2);		// Flow control mac high
		SPI_write(1,i,0x0C,0x002b00f1);		// Flow control mac low
		SPI_write(1,i,0x6E,BIT(3));		// forward pause frame
	}
	if(LAN_NUM==4)
		SPI_write(1,i,0x00,0x20000030);			// set port 4 as reset
	
	SPI_write(1,6,0x00,0x300701B1);			// MAC configure
	SPI_write(1,6,0x00,0x10070181);			// MAC configure
	SPI_write(1,6,0x10,0x5F4);			// Max length
	SPI_write(1,6,0x04,0x00030000);		// Flow control
	SPI_write(1,6,0xDF,0x00000002);		// Flow control
	SPI_write(1,6,0x08,0x000050c2);		// Flow control mac high
	SPI_write(1,6,0x0C,0x002b00f1);		// Flow control mac low
	SPI_write(1,6,0x6E,BIT(3));		// forward pause frame

	
	//SPI_write(7,0,0x05,0x31);			// MII delay for loader
	//SPI_write(7,0,0x05,0x01);			// MII delay for kernel
	SPI_write(7,0,0x05,0x33);
	
	if(LAN_NUM==5)
		SPI_write(2,0,0x10,0x7F);			
	else	
		SPI_write(2,0,0x10,0x4F);			// Receive mask
	
	hal_delay_us(50*1000);
	
	SPI_write(7,0,0x14,0x02);			// Release Reset
	
	hal_delay_us(3*1000);
	
	for(i=0;i<LAN_NUM;i++){
		init_seq_7385(i);
		phy_receiver_init(i);
		cmd = BIT(26)|(i<<21)|(0x1B<<16);	// Config LED
		SPI_write(3,0,1,cmd);
		hal_delay_us(10*1000);
		reg_val = SPI_read(3,0,2);
		reg_val &= 0xFF00;
		reg_val |= 0x61;
		cmd = (i<<21)|(0x1B<<16)|reg_val;
		SPI_write(3,0,1,cmd);
		
		cmd = BIT(26)|(i<<21)|(0x04<<16);	// Pause enable
		SPI_write(3,0,1,cmd);
		hal_delay_us(10*1000);
		reg_val = SPI_read(3,0,2);
		reg_val |= BIT(10)|BIT(11);
		cmd = (i<<21)|(0x04<<16)|reg_val;
		SPI_write(3,0,1,cmd);
		
		cmd = BIT(26)|(i<<21)|(0x0<<16);	// collision test and re-negotiation
		SPI_write(3,0,1,cmd);
		hal_delay_us(10*1000);
		reg_val = SPI_read(3,0,2);
		reg_val |= BIT(7)|BIT(8)|BIT(9);
		cmd = (i<<21)|(0x0<<16)|reg_val;
		SPI_write(3,0,1,cmd);
	}
	init_seq_7385(i);
	phy_receiver_init(i);
	
}

/***********************************************************
* SPI_CS_enable
* before access ,you have to enable Chip Select. (pull high)
* When fisish, you should pull low !!
*************************************************************/
void SPI_CS_enable(unsigned char enable)
{

	unsigned int addr,value;

	addr = (GPIO_BASE_ADDR + GPIO_PIN_DIR);
	value = readl(addr) |GPIO_EECS |GPIO_EECK;   /* set EECS/EECK Pin to output */
	writel(value,addr);

	if(enable)
	{
		addr = (GPIO_BASE_ADDR + GPIO_DATA_CLEAR);
		writel(GPIO_EECK,addr); /* set EECK to 0 */	// pull low clk first
		addr = (GPIO_BASE_ADDR + GPIO_DATA_CLEAR);
		writel(GPIO_EECS,addr); /* set EECS to 0 */

	}
	else
	{
		addr = (GPIO_BASE_ADDR + GPIO_DATA_SET);
		writel(GPIO_EECK,addr); /* set EECK to 1 */	// pull high clk before disable
		writel(GPIO_EECS,addr); /* set EECS to 1 */
	}
}


/************************************************
* SPI_read
* table -> which table to be read: 1/count  0/EEPROM
* addr  -> Address to be read
* return : Value of the register
*************************************************/
unsigned int SPI_read(unsigned char block,unsigned char subblock,unsigned char addr)
{
	int     i;
	char    bit;
	int     ad1;
	unsigned int data,value=0;
	unsigned int 		flag,tmp;

		flag = readl(SL2312_GLOBAL_BASE_ADDR + 0x04);	
		tmp = readl(SL2312_GLOBAL_BASE_ADDR + 0x0);	

		if((flag&0x40000000)&&((tmp&0x000000FF)==0xc3))
		{
				flag = readl(SL2312_GLOBAL_BASE_ADDR + 0x30);
				tmp = flag;
				flag &= ~(1 << 4);
				writel(flag,SL2312_GLOBAL_BASE_ADDR + 0x30);
		}
		else
			tmp = 0;
			
	SPI_CS_enable(1);
	
	data = (block<<5) | subblock;

	//send write command 
	for(i=SPI_OP_LEN-1;i>=0;i--)
	{
		bit = (data>>i)& 0x01;
		SPI_write_bit(bit);
	}
	
	// send 8 bits address (MSB first, LSB last)
	for(i=SPI_ADD_LEN-1;i>=0;i--)
	{
		bit = (addr>>i)& 0x01;
		SPI_write_bit(bit);
	}
	
	// dummy read for chip ready
	for(i=0;i<8;i++)
		SPI_read_bit();

	
	// read 32 bits data (MSB first, LSB last)
	for(i=SPI_DAT_LEN-1;i>=0;i--)
	{
		bit = SPI_read_bit();
		value |= bit<<i;
	}

	SPI_CS_enable(0);	// CS low
	
	if(tmp)
		writel(tmp,SL2312_GLOBAL_BASE_ADDR + 0x30);
	
	return(value);

}

unsigned int SPI_get_identifier(void)
{
	//unsigned int flag=0;
	unsigned int 		flag,tmp;

		flag = readl(SL2312_GLOBAL_BASE_ADDR + 0x04);	
		tmp = readl(SL2312_GLOBAL_BASE_ADDR + 0x0);	

		if((flag&0x40000000)&&((tmp&0x000000FF)==0xc3))
		{
				GPIO_EECS=0x00000200;
				GPIO_MOSI=0x00000080;//0x00000080
				GPIO_MISO=0x00000400;//0x00000400
				GPIO_EECK=0x00000020;
				GPIO_MISO_BIT = 10;
				LAN_NUM = 5;
		}
		else
		{
			GPIO_EECS=0x80000000;
			GPIO_MOSI=0x20000000;
			GPIO_MISO=0x40000000;
			GPIO_EECK=0x10000000;
			GPIO_MISO_BIT = 30;
			LAN_NUM = 4;
		}
		
		flag = 0;

	SPI_write(7,0,0x01,0x01);
	flag = SPI_read(7,0,0x18);		// chip id
	//printf("Get VSC-switch ID 0x%08x\n",flag);
	if(((flag&0x0ffff000)>>12) == 0x7385)
	{
		printf("Init VSC-switch ...\n");
		return 1;
	}
	else
		return 0;
}

void phy_optimize_receiver_init(unsigned char port_no)		// link down
{
	/* BZ 1776/1860/2095/2107, part 1/3 and 2/3 */
	phy_write(port_no, 31, 0x2A30);
	phy_write_masked(port_no, 12, 0x0200, 0x0300);
	phy_write(port_no, 31, 0);
    
	phy_write(port_no, 31, 0x2A30);
	phy_write_masked(port_no, 20, 0x1500, 0x1F00);
	phy_write_masked(port_no, 20, 0x1500, 0x1F00);
	phy_write(port_no, 31, 0);
}

void  phy_optimize_receiver_reconfig(unsigned char port_no)	// link up
{
	unsigned short vga_state_a;

    	/* BZ 1776/1860/2095/2107 part 3/3 */
	phy_write(port_no, 31, 0x52b5);
	phy_write(port_no, 16, 0xaff0);
	vga_state_a = phy_read(port_no, 17);
	vga_state_a = ((vga_state_a >> 4) & 0x01f);
	if ((vga_state_a < 16) || (vga_state_a > 20)) {
	    phy_write(port_no, 31, 0x2a30);
	    phy_write_masked(port_no, 12, 0x0000, 0x0300);
	}
	phy_write(port_no, 31, 0);
}


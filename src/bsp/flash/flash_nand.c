#include <define.h>
#include <board_config.h>
//#ifdef FLASH_TYPE_NAND

#include <sl2312.h>
#include "flash_drv.h"
#include "flash_nand_parts.h"
//#include <flash_nand_parts.h>

// Low level debugging
//   1 - command level - prints messages about read/write/erase commands
//   2 - hardware level - shows all NAND device I/O data
#ifdef BOARD_NAND_BOOT		
UINT32 chip0_en=0,nwidth=0,ndirect=NFLASH_DIRECT,def_width=0;
UINT32 ADD,ADD2,ADD3,ADD4,ADD5;
UINT32 opcode=0,nmode=0,ecc_cmp=0;
UINT32 info_flash[12];
//UINT8 ck_buf[PAGE512_SIZE];
//UINT8 tmp_buf[PAGE2K_RAW_SIZE];
UINT8 *ck_buf=NULL,*tmp_buf=NULL, *wt_buff=NULL, *oob=NULL, *dev_oob=NULL;
UINT8 ecc_buf[4];
extern int nand_present;
extern void FLASH_CTRL_WRITE_REG(unsigned int addr,unsigned int data);
extern unsigned int FLASH_CTRL_READ_REG(unsigned int addr);
UINT32	nand_block_markbad(UINT64 page_add);
//----------------------------------------------------------------------------
// Now that device properties are defined, include magic for defining
// accessor type and constants.
//----------------------------------------------------------------------------
// Information about supported devices


static const flash_dev_info_t* flash_dev_info;

#define NUM_DEVICES (sizeof(supported_devices)/sizeof(flash_dev_info_t))
#define FLASH_BASE BOARD_FLASH_BASE_ADDR
//----------------------------------------------------------------------------
// Functions that put the flash device into non-read mode must reside
// in RAM.
void flash_query(void* data) ;
static int  flash_erase_block(void* block, unsigned int size);
static int  flash_program_buf(void* addr, void* data, int len);
UINT32	VeffBad(UINT64 page_add);

#ifdef BOARD_SUPPORT_SOFTECC
	UINT32 nand_oob_16[6]={8,9,10,13,14,15};
	UINT32 nand_oob_64[24]={40, 41, 42, 43, 44, 45,
							46, 47, 48, 49, 50, 51,
							52, 53, 54, 55, 56, 57,
							58, 59, 60, 61, 62, 63};
#else
	UINT32 nand_oob_16[3]={0,1,2};
	UINT32 nand_oob_64[12]={52, 53, 54, 55, 56, 57,58, 59, 60, 61, 62, 63};
#endif

unsigned int FLASH_READ_DMA_REG(unsigned int addr)
{
    unsigned int *base;
    unsigned int data;
    
    base = (unsigned int *)(SL2312_GENERAL_DMA_BASE + addr);
    data = *base;
    return (data);
}

void FLASH_WRITE_DMA_REG(unsigned int addr,unsigned int data)
{
    unsigned int *base;
    
    base = (unsigned int *)(SL2312_GENERAL_DMA_BASE + addr);
    *base = data;
    return;
}

unsigned int StatusCheck(UINT32 chip_en, UINT32 nwidth)                              //status check
{
	UINT8 status,cnt=5;
	

RD_STATUS:
	
	FLASH_CTRL_WRITE_REG(NFLASH_ECC_CONTROL, 0x0); //set 31b = 0
	FLASH_CTRL_WRITE_REG(NFLASH_COUNT, 0x7f000070); //set only command no address and two data
	
	FLASH_CTRL_WRITE_REG(NFLASH_COMMAND_ADDRESS, Read_Status_cmd); //write read status command
	
	
	opcode = FLASH_START_BIT|FLASH_RD|chip0_en|nwidth;//|ndirect; //set start bit & 8bits read command
	FLASH_CTRL_WRITE_REG(NFLASH_ACCESS, opcode);
	
	while(opcode&FLASH_START_BIT) //polling flash access 31b
	{
		opcode=FLASH_CTRL_READ_REG(NFLASH_ACCESS);
		flash_delay();
	}
	
	status=FLASH_CTRL_READ_REG(NFLASH_DATA);
	
	if(status==0)
	{
		cnt--;
		goto RD_STATUS;
	}
	//printf("\n<--- StatusCheck : %x \n ",status);
	
	if(flash_dev_info->page_size < PAGE512_RAW_SIZE)
	{
		if((status!= (STS_WP|STS_READY))&&(cnt>0))
		{
			cnt--;
			goto RD_STATUS;
		}
	
		if (status == (STS_WP|STS_READY))      return  PASS;
		else 			 return  FAIL;
	}
	else
	{
		if((status!= (STS_WP|STS_READY|STS_TRUE_READY))&&(cnt>0))
		{
			cnt--;
			goto RD_STATUS;
		}
	
		if (status == (STS_WP|STS_READY|STS_TRUE_READY))      return  PASS;
		else 			 return  FAIL;
	}
}

int
flash_hwr_init(FLASH_INFO_T *info)
{
    UINT8 id[4];
    int i,j;
	unsigned char *ptr;

	chip0_en = NFLASH_CHIP0_EN;

    flash_query(id);

    // Check that flash_id data is matching the one the driver was
    // configured for.

    // Check manufacturer
    if ((id[0] != CYGNUM_FLASH_VENDOR_TOSHIBA)&&(id[0] != CYGNUM_FLASH_VENDOR_SAMSUNG)&&(id[0] != CYGNUM_FLASH_VENDOR_STMICRO)) {
        printf("Can't identify FLASH - manufacturer is: %x\n", id[0]);
        return FLASH_ERR_DRV_WRONG_PART;
    }

    // Look through table for device data
    flash_dev_info = supported_devices;
    for (i = 0; i < NUM_DEVICES; i++) {
        if ((flash_dev_info->device_id == id[1]) && (flash_dev_info->vendor_id == id[0]))
            break;
        flash_dev_info++;
    }

	
    // Did we find the device? If not, return error.
    if (NUM_DEVICES == i) {
        printf("Can't identify FLASH - device is: %x, supported: ", id[1]);
        for (i = 0;  i < NUM_DEVICES;  i++) {
            printf("%x ", supported_devices[i].device_id);
        }
        printf("\n");
        return FLASH_ERR_DRV_WRONG_PART;
    }

	info_flash[1] = flash_dev_info->device_size >> 20;
	info_flash[2] = flash_dev_info->block_count ;
	info_flash[3] = flash_dev_info->block_size / flash_dev_info->page_size;
	//info_flash[4] = flash_dev_info->page_size+ (flash_dev_info->page_size>PAGE512_SIZE)?64:PAGE512_OOB_SIZE;
	info_flash[5] = flash_dev_info->page_size;
	info_flash[4] = info_flash[5] + (info_flash[5]>PAGE512_SIZE?PAGE2K_OOB_SIZE:PAGE512_OOB_SIZE);
	info_flash[6] = (flash_dev_info->page_size>PAGE512_SIZE)?PAGE2K_OOB_SIZE:PAGE512_OOB_SIZE;
	nand_present = 1;


    
    // Hard wired for now
   	info->erase_block 	= (void *)flash_erase_block;
    info->program 		= (void *)flash_program_buf;
    info->block_size 	= flash_dev_info->block_size;
    info->blocks 		= flash_dev_info->block_count;
    info->start 		= (void *)FLASH_BASE;
    info->end 			= (void *)(FLASH_BASE+ (flash_dev_info->device_size));
    info->vendor		= id[0]; 
    info->chip_id		= id[1];
    info->sub_id1       = 0;
    info->sub_id2       = 0;
    
    //erase all block with bad block
    //for(i=0x0;i<info_flash[2];i++){
    ////	if(VeffBad((unsigned char *)(i*(UINTPAGE2K_OOB_SIZE)flash_dev_info->block_size))==PASS)
	//		if(flash_erase_block((UINT64)i*(UINT64)flash_dev_info->block_size, flash_dev_info->block_size)==FLASH_ERR_ERASE)   	    // Executing erase operation only on the valid blocks
	//		{
	//			nand_block_markbad((UINT64)(i*(UINT64)flash_dev_info->block_size));
	//			ptr = (unsigned char *)(i*(UINT64)flash_dev_info->block_size+SL2312_FLASH_BASE);
	//			for(j=0;j<PAGE512_RAW_SIZE;j++)
	//				tmp_buf[j] = *ptr++;
	//			printf("\nBlock %x bad !!\n",i);
	//		}
	// }


	ck_buf = (char *)malloc(PAGE512_SIZE);
	tmp_buf = (char *)malloc(PAGE2K_RAW_SIZE);
	wt_buff = (char *)malloc(PAGE2K_RAW_SIZE);
	dev_oob = (char *)malloc(PAGE2K_OOB_SIZE);
	oob = (char *)malloc(PAGE2K_OOB_SIZE);
    if (!ck_buf || !tmp_buf || !wt_buff || !dev_oob || !oob)
	{
		printf("No free memory !\n");
		return 1;
	}
    return FLASH_ERR_OK;
}

//----------------------------------------------------------------------------
// Map a hardware status to a package error
static int
flash_hwr_map_error(int e)
{
    return e;
}


//----------------------------------------------------------------------------
// See if a range of FLASH addresses overlaps currently running code
static bool
flash_code_overlaps(void *start, void *end)
{
    extern unsigned char _stext[], _etext[];

    return ((((unsigned long)&_stext >= (unsigned long)start) &&
             ((unsigned long)&_stext < (unsigned long)end)) ||
            (((unsigned long)&_etext >= (unsigned long)start) &&
             ((unsigned long)&_etext < (unsigned long)end)));
}

static void
put_NAND(volatile UINT8 *ROM, UINT8 val)
{
    *ROM = val;
#if FLASH_DEBUG > 1
    printf("%02x ", val);
#endif
}

//----------------------------------------------------------------------------
// Flash Query
//
// Only reads the manufacturer and part number codes for the first
// device(s) in series. It is assumed that any devices in series
// will be of the same type.

void
flash_query(void* data)
{
	UINT8* id = (UINT8*) data;
	UINT32 i, dent_bit;
	UINT8 m_code,d_code,extid=0;

	
	dent_bit=FLASH_CTRL_READ_REG(FLASH_TYPE);
	if((dent_bit&FLASH_WIDTH_MASK)==FLASH_WIDTH_MASK)
		nwidth=NFLASH_WiDTH16;
	else
		nwidth=NFLASH_WiDTH8;
	
	
	FLASH_CTRL_WRITE_REG(NFLASH_ECC_CONTROL, 0x0); 		//set 31b = 0
	if((dent_bit&FLASH_SIZE_MASK)==0)
		FLASH_CTRL_WRITE_REG(NFLASH_COUNT, 0x7f000100); 		//set only command & address and two data
	else if((dent_bit&FLASH_SIZE_MASK) == FLASH_SIZE_MASK)
	{
		FLASH_CTRL_WRITE_REG(NFLASH_COUNT, 0x7f000400); 		//set only command & address and 4 data
		extid = 3;
	}
	else
	{
		FLASH_CTRL_WRITE_REG(NFLASH_COUNT, 0x7f000300); 		//set only command & address and 4 data
		extid = 2;
	}
	
	FLASH_CTRL_WRITE_REG(NFLASH_COMMAND_ADDRESS, FLASH_Read_ID); 	//write read id command
	FLASH_CTRL_WRITE_REG(NFLASH_ADDRESS, 0x0); 			//write address 0x00
	
	/* read maker code */
	opcode = FLASH_START_BIT|FLASH_RD|chip0_en|nwidth|ndirect; //set start bit & 8bits read command
	FLASH_CTRL_WRITE_REG(NFLASH_ACCESS, opcode);
	opcode=FLASH_CTRL_READ_REG(NFLASH_ACCESS);
	while(opcode&FLASH_START_BIT) //polling flash access 31b
	{
		opcode=FLASH_CTRL_READ_REG(NFLASH_ACCESS);
		flash_delay();
	}
	if(nwidth == NFLASH_WiDTH8)
	{
		m_code=FLASH_CTRL_READ_REG(NFLASH_DATA);
		//FLASH_CTRL_WRITE_REG(NFLASH_DATA, 0x0);
		/* read device code */
		opcode = FLASH_START_BIT|FLASH_RD|chip0_en|nwidth|ndirect; //set start bit & 8bits read command
		FLASH_CTRL_WRITE_REG(NFLASH_ACCESS, opcode);
		opcode=FLASH_CTRL_READ_REG(NFLASH_ACCESS);
		while(opcode&FLASH_START_BIT) //polling flash access 31b
		{
			opcode=FLASH_CTRL_READ_REG(NFLASH_ACCESS);
			flash_delay();
		}
	
		d_code = (FLASH_CTRL_READ_REG(NFLASH_DATA)>>8);
	
		dent_bit=FLASH_CTRL_READ_REG(FLASH_TYPE);
		if((dent_bit&FLASH_SIZE_MASK)>0) //size > 32MB
		{
			for(i=0 ; i<extid ; i++)
			{
				//data cycle 3 & 4 ->not use
				opcode = FLASH_START_BIT|FLASH_RD|chip0_en|nwidth|ndirect; //set start bit & 8bits read command
				FLASH_CTRL_WRITE_REG(NFLASH_ACCESS, opcode);
				opcode=FLASH_CTRL_READ_REG(NFLASH_ACCESS);
				while(opcode&FLASH_START_BIT) //polling flash access 31b
				{
					opcode=FLASH_CTRL_READ_REG(NFLASH_ACCESS);
					flash_delay();
				}
	
				dent_bit=FLASH_CTRL_READ_REG(NFLASH_DATA);
			}
		}
	}
	else	// 16-bit
	{
		dent_bit = FLASH_CTRL_READ_REG(NFLASH_DATA);
		m_code = dent_bit;
		d_code = dent_bit>>8;
	
		//data cycle 3 & 4 ->not use
		dent_bit=FLASH_CTRL_READ_REG(FLASH_TYPE);
		if(((dent_bit&FLASH_SIZE_MASK)>0)&&(nwidth == NFLASH_WiDTH16)) // size >32MB
		{
			opcode = FLASH_START_BIT|FLASH_RD|chip0_en|nwidth|ndirect; //set start bit & 8bits read command
			FLASH_CTRL_WRITE_REG(NFLASH_ACCESS, opcode);
			opcode=FLASH_CTRL_READ_REG(NFLASH_ACCESS);
			while(opcode&FLASH_START_BIT) //polling flash access 31b
			{
				opcode=FLASH_CTRL_READ_REG(NFLASH_ACCESS);
				flash_delay();
			}
	
			dent_bit=FLASH_CTRL_READ_REG(NFLASH_DATA);
		}
	}
	
	
	//printf("\rManufacturer's code is %x\n", m_code);
	//printf("Device code is %x\n", d_code);
	
	id[0] = m_code;	// vendor ID
	id[1] = d_code;	// device ID
	opcode = NFLASH_DIRECT;
	FLASH_CTRL_WRITE_REG(NFLASH_ACCESS,opcode);

}

//----------------------------------------------------------------------------
// Erase Block
static int
flash_erase_block(void* block, unsigned int size)
{
	UINT32 page_add = (UINT32)block;
//#ifndef BOARD_SUPPORT_NAND_INDIRECT	
//	if(page_add > SL2312_FLASH_BASE)
//		page_add -=SL2312_FLASH_BASE;
//#endif		
	page_add = page_add / flash_dev_info->page_size;
	
	
	FLASH_CTRL_WRITE_REG(NFLASH_ECC_CONTROL, 0x0); //set 31b = 0
	
	if(flash_dev_info->block_count*(flash_dev_info->block_size / flash_dev_info->page_size)> 0x10000)//if(info_flash[1]>=64)
		FLASH_CTRL_WRITE_REG(NFLASH_COUNT, 0x7f0fff21);  //3 address & 2 command
	else
		FLASH_CTRL_WRITE_REG(NFLASH_COUNT, 0x7f0fff11);  //2 address & 2 command
	
	FLASH_CTRL_WRITE_REG(NFLASH_COMMAND_ADDRESS, ((FLASH_Start_Erase<<8)|FLASH_Block_Erase));//0x0000d060); //write read id command
	FLASH_CTRL_WRITE_REG(NFLASH_ADDRESS, page_add); //write address 0x00
	
	
	
	/* read maker code */
	//opcode = FLASH_START_BIT|FLASH_WT|chip0_en|nwidth|ndirect; //set start bit & 8bits write command
	opcode = FLASH_START_BIT|FLASH_WT; //set start bit & 8bits write command
	FLASH_CTRL_WRITE_REG(NFLASH_ACCESS, opcode);
	
	while(opcode&FLASH_START_BIT) //polling flash access 31b
	{
		opcode=FLASH_CTRL_READ_REG(NFLASH_ACCESS);
		flash_delay();
	}
	
	
	//printf("\n--->BlockErase: \n");
	
	if(StatusCheck(chip0_en, def_width)!=FAIL)
		return FLASH_ERR_OK;
	else
		return  FLASH_ERR_ERASE;
	

}

#ifdef BOARD_SUPPORT_SOFTECC
static const unsigned char _nand_ecc_precalc_table[] = {
    0x00, 0x55, 0x56, 0x03, 0x59, 0x0c, 0x0f, 0x5a, 0x5a, 0x0f, 0x0c, 0x59, 0x03, 0x56, 0x55, 0x00,
    0x65, 0x30, 0x33, 0x66, 0x3c, 0x69, 0x6a, 0x3f, 0x3f, 0x6a, 0x69, 0x3c, 0x66, 0x33, 0x30, 0x65,
    0x66, 0x33, 0x30, 0x65, 0x3f, 0x6a, 0x69, 0x3c, 0x3c, 0x69, 0x6a, 0x3f, 0x65, 0x30, 0x33, 0x66,
    0x03, 0x56, 0x55, 0x00, 0x5a, 0x0f, 0x0c, 0x59, 0x59, 0x0c, 0x0f, 0x5a, 0x00, 0x55, 0x56, 0x03,
    0x69, 0x3c, 0x3f, 0x6a, 0x30, 0x65, 0x66, 0x33, 0x33, 0x66, 0x65, 0x30, 0x6a, 0x3f, 0x3c, 0x69,
    0x0c, 0x59, 0x5a, 0x0f, 0x55, 0x00, 0x03, 0x56, 0x56, 0x03, 0x00, 0x55, 0x0f, 0x5a, 0x59, 0x0c,
    0x0f, 0x5a, 0x59, 0x0c, 0x56, 0x03, 0x00, 0x55, 0x55, 0x00, 0x03, 0x56, 0x0c, 0x59, 0x5a, 0x0f,
    0x6a, 0x3f, 0x3c, 0x69, 0x33, 0x66, 0x65, 0x30, 0x30, 0x65, 0x66, 0x33, 0x69, 0x3c, 0x3f, 0x6a,
    0x6a, 0x3f, 0x3c, 0x69, 0x33, 0x66, 0x65, 0x30, 0x30, 0x65, 0x66, 0x33, 0x69, 0x3c, 0x3f, 0x6a,
    0x0f, 0x5a, 0x59, 0x0c, 0x56, 0x03, 0x00, 0x55, 0x55, 0x00, 0x03, 0x56, 0x0c, 0x59, 0x5a, 0x0f,
    0x0c, 0x59, 0x5a, 0x0f, 0x55, 0x00, 0x03, 0x56, 0x56, 0x03, 0x00, 0x55, 0x0f, 0x5a, 0x59, 0x0c,
    0x69, 0x3c, 0x3f, 0x6a, 0x30, 0x65, 0x66, 0x33, 0x33, 0x66, 0x65, 0x30, 0x6a, 0x3f, 0x3c, 0x69,
    0x03, 0x56, 0x55, 0x00, 0x5a, 0x0f, 0x0c, 0x59, 0x59, 0x0c, 0x0f, 0x5a, 0x00, 0x55, 0x56, 0x03,
    0x66, 0x33, 0x30, 0x65, 0x3f, 0x6a, 0x69, 0x3c, 0x3c, 0x69, 0x6a, 0x3f, 0x65, 0x30, 0x33, 0x66,
    0x65, 0x30, 0x33, 0x66, 0x3c, 0x69, 0x6a, 0x3f, 0x3f, 0x6a, 0x69, 0x3c, 0x66, 0x33, 0x30, 0x65,
    0x00, 0x55, 0x56, 0x03, 0x59, 0x0c, 0x0f, 0x5a, 0x5a, 0x0f, 0x0c, 0x59, 0x03, 0x56, 0x55, 0x00
};

static void 
_nand_trans_result(unsigned char reg2, unsigned char reg3,
                   unsigned char *ecc0, unsigned char *ecc1)
{
    unsigned char a, b, i, tmp1, tmp2;
	
    /* Initialize variables */
    a = b = 0x80;
    tmp1 = tmp2 = 0;
	
    /* Calculate first ECC byte */
    for (i = 0; i < 4; i++) {
        if (reg3 & a)		/* LP15,13,11,9 --> ecc_code[0] */
            tmp1 |= b;
        b >>= 1;
        if (reg2 & a)		/* LP14,12,10,8 --> ecc_code[0] */
            tmp1 |= b;
        b >>= 1;
        a >>= 1;
    }
	
    /* Calculate second ECC byte */
    b = 0x80;
    for (i = 0; i < 4; i++) {
        if (reg3 & a)		/* LP7,5,3,1 --> ecc_code[1] */
            tmp2 |= b;
        b >>= 1;
        if (reg2 & a)		/* LP6,4,2,0 --> ecc_code[1] */
            tmp2 |= b;
        b >>= 1;
        a >>= 1;
    }
	
    /* Store two of the ECC bytes */
    *ecc0 = tmp1;    
    *ecc1 = tmp2;
}

//
// Calculate 3 byte ECC on 256 bytes of data
//
static void
_nand_page_ECC(unsigned char *data, unsigned char *ecc0,
               unsigned char *ecc1, unsigned char *ecc2)
{
    unsigned char idx, reg1, reg2, reg3;
    int j;
	
    /* Initialize variables */
    reg1 = reg2 = reg3 = 0;
    *ecc0 = *ecc1 = *ecc2 = 0;
	
    /* Build up column parity */ 
    for(j = 0; j < 256; j++) {
        /* Get CP0 - CP5 from table */
        idx = _nand_ecc_precalc_table[*data++];
        reg1 ^= (idx & 0x3f);
        /* All bit XOR = 1 ? */
        if (idx & 0x40) {
            reg3 ^= (unsigned char) j;
            reg2 ^= ~((unsigned char) j);
        }
    }
	
    /* Create non-inverted ECC code from line parity */
    _nand_trans_result(reg2, reg3, ecc0, ecc1);
	
    /* Calculate final ECC code */
    *ecc0 = ~*ecc0;
    *ecc1 = ~*ecc1;
    *ecc2 = ((~reg1) << 2) | 0x03;
}

//
// Correct a buffer via ECC (1 bit, 256 byte block)
//  Return: 0 => No error
//          1 => Corrected
//          2 => Not corrected, ECC updated
//         -1 => Not correctable
//
int 
_nand_correct_data(unsigned char *dat, unsigned char *read_ecc, unsigned char *calc_ecc)
{
    unsigned char a, b, c, d1, d2, d3, add, bit, i;
	
    /* Do error detection */ 
    d1 = calc_ecc[0] ^ read_ecc[0];
    d2 = calc_ecc[1] ^ read_ecc[1];
    d3 = calc_ecc[2] ^ read_ecc[2];
	
    if ((d1 | d2 | d3) == 0) {
        /* No errors */
        return 0;
    } else {
        a = (d1 ^ (d1 >> 1)) & 0x55;
        b = (d2 ^ (d2 >> 1)) & 0x55;
        c = (d3 ^ (d3 >> 1)) & 0x54;
		
        /* Found and will correct single bit error in the data */
        if ((a == 0x55) && (b == 0x55) && (c == 0x54)) {
            c = 0x80;
            add = 0;
            a = 0x80;
            for (i=0; i<4; i++) {
                if (d1 & c)
                    add |= a;
                c >>= 2;
                a >>= 1;
            }
            c = 0x80;
            for (i=0; i<4; i++) {
                if (d2 & c)
                    add |= a;
                c >>= 2;
                a >>= 1;
            }
            bit = 0;
            b = 0x04;
            c = 0x80;
            for (i=0; i<3; i++) {
                if (d3 & c)
                    bit |= b;
                c >>= 2;
                b >>= 1;
            }
            b = 0x01;
            a = dat[add];
            a ^= (b << bit);
            dat[add] = a;
            return 1;
        } else {
            i = 0;
            while (d1) {
                if (d1 & 0x01)
                    ++i;
                d1 >>= 1;
            }
            while (d2) {
                if (d2 & 0x01)
                    ++i;
                d2 >>= 1;
            }
            while (d3) {
                if (d3 & 0x01)
                    ++i;
                d3 >>= 1;
            }
            if (i == 1) {
                /* ECC Code Error Correction */
                read_ecc[0] = calc_ecc[0];
                read_ecc[1] = calc_ecc[1];
                read_ecc[2] = calc_ecc[2];
                return 2;
            } else {
                /* Uncorrectable Error */
                return -1;
            }
        }
    }
	
    /* Should never happen */
    return -1;
}
#endif

//----------------------------------------------------------------------------
// Program Buffer
static int flash_program_buf(void* addr, void* data, int len)
{
	int res=FLASH_ERR_OK, size;
	unsigned char *paddr=addr;
	unsigned char *pdata=data;
	//UINT8 wt_buff[PAGE2K_RAW_SIZE];

	while(len>0) {

		//if(StatusCheck(chip0_en, def_width)!=FAIL)
		{
			if(len < flash_dev_info->page_size)
			{
				memset(wt_buff,0xff,PAGE2K_RAW_SIZE);
				memcpy(wt_buff,pdata,len);
				size = len;
				if( pagewrite(paddr, wt_buff, flash_dev_info->page_size) != PASS)
				return FLASH_ERR_PROGRAM;
			}
			else
			{
				size = flash_dev_info->page_size;
				if( pagewrite(paddr, pdata, flash_dev_info->page_size) != PASS)
					return FLASH_ERR_PROGRAM;
			}
			flash_delay();
			len -= size;
			paddr += size;
			pdata += size;
		}
	}
	return res;

}

//----------------------------------------------------------------------------
// Read data into buffer
int
flash_read_buf(void* addr, void* buff, int len)
{
	unsigned int i,res=FLASH_ERR_OK,oobsize,blocka;
	UINT8 read_buff[PAGE2K_RAW_SIZE],*pbuff=(UINT8*)buff;
	oobsize = PAGE512_OOB_SIZE*flash_dev_info->page_size/PAGE512_SIZE;
	
	
	while(len>0){
NEXT_BLOCK:		
		blocka = (unsigned int)addr;
		if((blocka & (flash_dev_info->block_size-1))==0)
		{
			while(VeffBad(addr)==FAIL)
			{
				//mark bad block
				addr += flash_info.block_size;
				goto NEXT_BLOCK;
			}
		}

		if(StatusCheck(chip0_en, def_width)!=FAIL)
		{
			if(pageread((UINT64)addr,read_buff)!=PASS){
				res = FLASH_ERR_HWR;
				printf("Flash read error!!\n");
				break;
			}
        	
			//memcpy(pbuff,read_buff,(len>flash_dev_info->page_size)?flash_dev_info->page_size:len);
			//if(len < (flash_dev_info->page_size+oobsize))
				memcpy(pbuff,read_buff,(len>flash_dev_info->page_size)?flash_dev_info->page_size:len);
			
			flash_delay();
			len -= (len>flash_dev_info->page_size)?flash_dev_info->page_size:len;//flash_dev_info->page_size;
			addr += (len>flash_dev_info->page_size)?flash_dev_info->page_size:len;//flash_dev_info->page_size;
			pbuff += (len>flash_dev_info->page_size)?flash_dev_info->page_size:len;//flash_dev_info->page_size;
		}	
	}
	return res;
}



int pageread(UINT64 page_add, UINT8* page_data)
{

	UINT32 i,j,data,nwidth,nopcode,dent_bit;//,po;
	UINT8  *adr=page_add,ecc,ecc_byte,ecc_bit,oobsize;
	//UINT8 dev_oob[PAGE2K_OOB_SIZE],oob[PAGE2K_OOB_SIZE];
	nwidth = NFLASH_WiDTH8;
	UINT32 *oobsel, *data32;
	
	
	
	ADD5=ADD4=ADD3=ADD2=0;
	//ndirect = 0;
	//	nmode = 1 ;
	ndirect = NFLASH_DIRECT;
		nmode = 1 ;	

	oobsize = PAGE512_OOB_SIZE*flash_dev_info->page_size/PAGE512_SIZE;
	
	if(oobsize>PAGE512_OOB_SIZE)
		oobsel = &nand_oob_64;
	else
		oobsel = &nand_oob_16;
		
#ifndef BOARD_SUPPORT_SOFTECC	
	FLASH_CTRL_WRITE_REG(NFLASH_ECC_CONTROL, 0x80000001); //set 31b = 0
#endif	
	
#ifdef BOARD_SUPPORT_NAND_INDIRECT
	page_add = (UINT32)page_add;
	//if(page_add > SL2312_FLASH_BASE)
	// page_add -= SL2312_FLASH_BASE;
	page_add /= flash_dev_info->page_size;
	
	if(flash_dev_info->page_size < PAGE512_RAW_SIZE)
	{
		ADD5 = (page_add & 0xff000000)>>24;
		ADD4 = (page_add & 0x00ff0000)>>16;
		ADD3 = (page_add & 0x0000ff00)>>8;
		ADD2 = (page_add & 0x000000ff);
    }
    else
    {
		ADD5=(UINT32)((page_add<<16)>>32)&0xff;
		ADD4=(UINT32)((page_add<<24)>>32)&0xff;
		ADD3=(UINT32)((page_add<<32)>>32)&0xff;
		
    }
    
    dent_bit=FLASH_CTRL_READ_REG(FLASH_TYPE);
	switch(dent_bit&FLASH_SIZE_MASK)
	{
		case FLASH_SIZE_32:
			FLASH_CTRL_WRITE_REG(NFLASH_COUNT, 0x0f01ff20);
		    nopcode = 0x0;
		break;
		
		case FLASH_SIZE_64:
		case FLASH_SIZE_128:
			FLASH_CTRL_WRITE_REG(NFLASH_COUNT, 0x0f01ff30);
		    nopcode = 0x0;
		break;
		
		case FLASH_SIZE_256:
			FLASH_CTRL_WRITE_REG(NFLASH_COUNT, 0x3f07ff41);
		    nopcode = 0x00003000;
		break;
	}
		nopcode |= (ADD5<<24);
		FLASH_CTRL_WRITE_REG(NFLASH_COMMAND_ADDRESS, nopcode); //write address 0x00
		
			nopcode = 0x0|(ADD4<<24)|(ADD3<<16)|(ADD2<<8);
		FLASH_CTRL_WRITE_REG(NFLASH_ADDRESS, nopcode); //write address 0x00
#if 1
//4 bytes access
		data32 = (UINT32 *) page_data;
		for(i=0;i<((flash_dev_info->page_size+oobsize)/4);i++)
		{
			nopcode = FLASH_START_BIT | FLASH_RD|chip0_en|NFLASH_WiDTH32|NFLASH_INDIRECT; //set start bit & 8bits read command
			FLASH_CTRL_WRITE_REG(NFLASH_ACCESS, nopcode); 
			
			while(nopcode&FLASH_START_BIT) //polling flash access 31b
      			{
        		   nopcode=FLASH_CTRL_READ_REG(NFLASH_ACCESS);
        		   hal_delay_us(2);
      			}
      			    
      		 	    data = FLASH_CTRL_READ_REG(NFLASH_DATA);    
      		 	   // printf(" %08x ",data);
      		 	    ///////////
    				////////
      		 	    data32[i] = data;
      		 	    hal_delay_us(2); 
		}
#else
//1 byte access		
		for(i=0;i<(flash_dev_info->page_size+oobsize);i++)
		{
			nopcode = FLASH_START_BIT | FLASH_RD|NFLASH_CHIP0_EN|NFLASH_WiDTH8|NFLASH_INDIRECT; //set start bit & 8bits read command
			FLASH_CTRL_WRITE_REG(NFLASH_ACCESS, nopcode); 
			
			while(nopcode&FLASH_START_BIT) //polling flash access 31b
      			{
        		   nopcode=FLASH_CTRL_READ_REG(NFLASH_ACCESS);
        		   hal_delay_us(2);
      			}
      			    
      		 	    data = FLASH_CTRL_READ_REG(NFLASH_DATA);    
      		 	    ///////////  		 	
      		 	    if((i&ECC_CHK_MASK)==0x01)
    					data>>=8;
    				else if((i&ECC_CHK_MASK)==0x02)
    					data>>=16;
    				else if((i&ECC_CHK_MASK)==0x03)
    					data>>=24;
    					
    				////////
      		 	    page_data[i] = (UINT8)data;
		}
#endif    
#else    
	FLASH_CTRL_WRITE_REG(NFLASH_ACCESS, NFLASH_DIRECT);

		for(i=0;i<(flash_dev_info->page_size+oobsize);i++)
		{
			//hal_delay_us(1);
			page_data[i] = *adr++;

		}

#endif
	for(j=0;j<oobsize;j++)		// Read OOB
	{
			dev_oob[j] = page_data[flash_dev_info->page_size+j];
	}
	
#ifdef BOARD_SUPPORT_SOFTECC
	int eccsteps=0,datidx=0,eccidx=0,ecc_status = 0;
		
		eccsteps = (flash_dev_info->page_size/256);
		for (; eccsteps; eccsteps--) {
			_nand_page_ECC(&page_data[datidx], &ecc_buf[0], &ecc_buf[1], &ecc_buf[2]);

			ecc_status = _nand_correct_data(&page_data[datidx],&dev_oob[oobsel[eccidx]],&ecc_buf[0]);
			if(ecc_status == -1)  {
				printf("nand_read_ecc: Failed ECC read(read: %x %x %x cal: %x %x %x)\n", ecc_buf[0], ecc_buf[1], ecc_buf[2], dev_oob[oobsel[eccidx]], dev_oob[oobsel[eccidx]+1], dev_oob[oobsel[eccidx]+2]);
						return FAIL;
			}
			datidx += 256;
			eccidx +=3;
		
		}
		
#else	
	// Waiting ECC generation completey
	opcode=FLASH_CTRL_READ_REG(NFLASH_ECC_STATUS);
	while(!(opcode&FLASH_START_BIT)) //polling flash access 31b
	{
		opcode=FLASH_CTRL_READ_REG(NFLASH_ECC_STATUS);
		hal_delay_us(5);
	}
	
	for(j=0;j<(flash_dev_info->page_size/PAGE512_SIZE);j++)
	{//for 512 bytes ~ 2k page
	
		opcode = 0x0|dev_oob[oobsel[j*3]]<<16|dev_oob[oobsel[j*3+1]]<<8|dev_oob[oobsel[j*3+2]];		    
		
		//opcode=FLASH_CTRL_READ_REG(NFLASH_ECC_CODE_GEN0+(j*4));
	
		FLASH_CTRL_WRITE_REG(NFLASH_ECC_OOB, opcode);
		opcode = 0x0|(j<<8); //select ECC code generation 0
		FLASH_CTRL_WRITE_REG(NFLASH_ECC_CONTROL, opcode); //???
	
		opcode=FLASH_CTRL_READ_REG(NFLASH_ECC_STATUS);
		
		ndirect = NFLASH_DIRECT;
		nmode = 1 ;
		if((opcode&ECC_CHK_MASK)==ECC_UNCORRECTABLE)		// un-correctable
		{
			printf("\nPageRead Uncorrectable error !!\n");
			return FAIL;
		}
		else if((opcode&ECC_CHK_MASK)==ECC_1BIT_DATA_ERR)	// 1-bit data error
		{
			ecc_byte = (opcode|ECC_ERR_BYTE)>>7;
			ecc_bit = (opcode|ECC_ERR_BIT)>>3;
			printf("\nPageRead One bit data error !!\n");
			printf("Correct bit%x of byte%x\n",ecc_bit,ecc_byte);
			ecc = page_data[j*PAGE512_SIZE+ecc_byte];
			ecc ^= (1<<ecc_bit);
			page_data[j*PAGE512_SIZE+ecc_byte] = ecc;
		}
		else if((opcode&ECC_CHK_MASK)==ECC_1BIT_ECC_ERR)	// 1-bit ecc error
		{
			printf("\nPageRead One bit ECC error !!\n");
			printf("Ecc comparison error bit : %x \n",(opcode|ECC_ERR_BIT));
			return FAIL;
	
		}
		else if((opcode&ECC_CHK_MASK)==ECC_NO_ERR)	// no error
		{
			return PASS;
		}
	
	}
#endif

	return PASS;

}

int pagewrite(void* addr, void* data, int len)
{
	UINT32 i,j,tt,nwidth,err_flag=PASS,dent_bit,nopcode;
	UINT32 page_add = (UINT32*)addr;
	UINT8  *adr=addr,ecc_buf[3];//,oob[PAGE2K_OOB_SIZE];
	UINT8  *write_data = (UINT8*)data ;
	UINT32 *oobsel,oobsize,*wdata32;
	
	nwidth = NFLASH_WiDTH8;
	
	//ndirect = 0;
	//	nmode = 1 ;
	ndirect = NFLASH_DIRECT;
		nmode = 1;	
	
	oobsize = PAGE512_OOB_SIZE*flash_dev_info->page_size/PAGE512_SIZE;
	
	memset(oob, 0xff, oobsize);	

#ifdef BOARD_SUPPORT_NAND_INDIRECT		

	page_add = (UINT32)page_add;
	//if(page_add > SL2312_FLASH_BASE)
	// page_add -= SL2312_FLASH_BASE;
	page_add /= flash_dev_info->page_size;
	
	ADD5=ADD4=ADD3=ADD2=0;
	if(flash_dev_info->page_size < PAGE512_RAW_SIZE)
	{
		ADD5=(UINT32)(((UINT64)page_add<<8)>>32)&0xff;
		ADD4=(UINT32)(((UINT64)page_add<<16)>>32)&0xff;
		ADD3=(UINT32)(((UINT64)page_add<<24)>>32)&0xff;
		ADD2=(UINT32)(((UINT64)page_add<<32)>>32)&0xff;
	}
	else
	{
		ADD5=(UINT32)(((UINT64)page_add<<16)>>32)&0xff;
		ADD4=(UINT32)(((UINT64)page_add<<24)>>32)&0xff;
		ADD3=(UINT32)(((UINT64)page_add<<32)>>32)&0xff;
	
	}
#else
	FLASH_CTRL_WRITE_REG(NFLASH_ACCESS, NFLASH_DIRECT);
#endif	
	if(oobsize>16)
		oobsel = &nand_oob_64;
	else
		oobsel = &nand_oob_16;
	

#ifndef BOARD_SUPPORT_SOFTECC
	FLASH_CTRL_WRITE_REG(NFLASH_ECC_CONTROL, FLASH_START_BIT|ECC_PAUSE_EN|ECC_CLR); //set 31b = 0 & ECC pause enable & Ecc clear
#endif	

#ifdef BOARD_SUPPORT_NAND_INDIRECT
		dent_bit=FLASH_CTRL_READ_REG(FLASH_TYPE);
		switch(dent_bit&FLASH_SIZE_MASK)
		{
			case FLASH_SIZE_32:
				FLASH_CTRL_WRITE_REG(NFLASH_COUNT, 0x0f01ff21); 
		    	nopcode = 0x00001080;
			break;
			
			case FLASH_SIZE_64:
			case FLASH_SIZE_128:
				FLASH_CTRL_WRITE_REG(NFLASH_COUNT, 0x0f01ff31); 
		    	nopcode = 0x00001080;
			break;
			
			case FLASH_SIZE_256:
				FLASH_CTRL_WRITE_REG(NFLASH_COUNT, 0x3f07ff41); 
		    	nopcode = 0x00001080;
			break;
		}
	
		nopcode |= (ADD5<<24);
		FLASH_CTRL_WRITE_REG(NFLASH_COMMAND_ADDRESS, nopcode); 
		    
		nopcode = 0x0|(ADD4<<24)|(ADD3<<16)|(ADD2<<8);
		FLASH_CTRL_WRITE_REG(NFLASH_ADDRESS, nopcode); 
#if 1
//4 bytes access 
		wdata32 = (UINT32 *)write_data;
		for(i=0;i<(flash_dev_info->page_size/4);i++)
		{
			tt = wdata32[i];

			//FLASH_CTRL_WRITE_REG(NFLASH_DATA, write_data32[i]); //write address 0x00
			FLASH_CTRL_WRITE_REG(NFLASH_DATA, tt); //write address 0x00
			nopcode = FLASH_START_BIT | FLASH_WT |chip0_en|NFLASH_WiDTH32|NFLASH_INDIRECT; //set start bit & 8bits read command
			FLASH_CTRL_WRITE_REG(NFLASH_ACCESS, nopcode); 
			
			while(nopcode&FLASH_START_BIT) //polling flash access 31b
      			{
        		   nopcode=FLASH_CTRL_READ_REG(NFLASH_ACCESS);
        		   hal_delay_us(2);
      			}  
      			hal_delay_us(2);  		 	
		}
#else
//1 byte access		
		for(i=0;i<flash_dev_info->page_size;i++)
		{
			tt = write_data[i];
			////////
    		if((i&0x03)==0x01)
    			tt<<=8;
    		else if((i&0x03)==0x02)
    			tt<<=16;
    		else if((i&0x03)==0x03)
    			tt<<=24;
    		////////
			
			FLASH_CTRL_WRITE_REG(NFLASH_DATA, tt); 
			nopcode = FLASH_START_BIT | FLASH_WT |NFLASH_CHIP0_EN|NFLASH_WiDTH8|NFLASH_INDIRECT; //set start bit & 8bits read command
			FLASH_CTRL_WRITE_REG(NFLASH_ACCESS, nopcode); 
			
			while(nopcode&FLASH_START_BIT) 
      		{
        	   nopcode=FLASH_CTRL_READ_REG(NFLASH_ACCESS);
        	   hal_delay_us(2);
      		}    		 	
		}
#endif		
#else
		FLASH_CTRL_WRITE_REG(NFLASH_ACCESS, NFLASH_DIRECT);
		
		for(i=0;i<flash_dev_info->page_size;i++)
		{
			*adr++ = write_data[i] ;
		}
#endif		
		
#ifdef BOARD_SUPPORT_SOFTECC		
		int eccsteps=0,datidx=0,eccidx=0;
		
		eccsteps = (flash_dev_info->page_size/256);
		for (; eccsteps; eccsteps--) {
			_nand_page_ECC(&write_data[datidx], &ecc_buf[0], &ecc_buf[1], &ecc_buf[2]);
			for (i = 0; i < 3; i++, eccidx++)
				oob[oobsel[eccidx]] = ecc_buf[i];
			datidx += 256;
		}
#else
		///////////////
		opcode=FLASH_CTRL_READ_REG(NFLASH_ECC_STATUS);
		while(!(opcode&FLASH_START_BIT)) //polling flash access 31b
		{
			opcode=FLASH_CTRL_READ_REG(NFLASH_ECC_STATUS);
			hal_delay_us(5);
		}
	
	
		for(i=0;i<(flash_dev_info->page_size/PAGE512_SIZE);i++)
		{
			opcode=FLASH_CTRL_READ_REG(NFLASH_ECC_CODE_GEN0+(i*4));
    		      
			for(j=3;j>0;j--)
    		      oob[oobsel[(i*3)+(3-j)]] = (opcode<<((4-j)*8)) >>24;
		}
	
		//disable ecc
		FLASH_CTRL_WRITE_REG(NFLASH_ECC_CONTROL, 0x0);
#endif		

#ifdef BOARD_SUPPORT_NAND_INDIRECT
#if 1
//4 bytes access
		wdata32 = (UINT32 *)oob;
		for(i=0;i<(oobsize/4);i++)
		{
			tt = wdata32[i];

			//FLASH_CTRL_WRITE_REG(NFLASH_DATA, write_data32[i]); //write address 0x00
			FLASH_CTRL_WRITE_REG(NFLASH_DATA, tt); //write address 0x00
			nopcode = FLASH_START_BIT | FLASH_WT |chip0_en|NFLASH_WiDTH32|NFLASH_INDIRECT; //set start bit & 8bits read command
			FLASH_CTRL_WRITE_REG(NFLASH_ACCESS, nopcode); 
			
			while(nopcode&FLASH_START_BIT) //polling flash access 31b
      			{
        		   nopcode=FLASH_CTRL_READ_REG(NFLASH_ACCESS);
        		   hal_delay_us(2);
      			}  
      			hal_delay_us(2);  		 	
		}
#else
//1 byte access	
		for(i=0;i<oobsize;i++)
		{
			tt = oob[i];
			////////
    		if((i&0x03)==0x01)
    			tt<<=8;
    		else if((i&0x03)==0x02)
    			tt<<=16;
    		else if((i&0x03)==0x03)
    			tt<<=24;
    		////////
			
			//FLASH_CTRL_WRITE_REG(NFLASH_DATA, write_data[i]); //write address 0x00
			FLASH_CTRL_WRITE_REG(NFLASH_DATA, tt); //write address 0x00
			nopcode = FLASH_START_BIT | FLASH_WT |NFLASH_CHIP0_EN|NFLASH_INDIRECT|NFLASH_WiDTH8; //set start bit & 8bits read command
			FLASH_CTRL_WRITE_REG(NFLASH_ACCESS, nopcode); 
			
			while(nopcode&FLASH_START_BIT) //polling flash access 31b
      			{
        		   nopcode=FLASH_CTRL_READ_REG(NFLASH_ACCESS);
        		   hal_delay_us(2);
      			}    		 	
		}
#endif		
#else
		for(i=0;i<oobsize;i++)
		{
			//hal_delay_us(1);
			*adr++ = oob[i] ;
		}
#endif		
	
		err_flag = PASS;

	if( StatusCheck(chip0_en, def_width) ==FAIL)
		err_flag = FLASH_ERR_PROGRAM;
#ifdef BOARD_SUPPORT_NAND_INDIRECT
		pageread((page_add*flash_dev_info->page_size), tmp_buf);
#else	
		pageread(page_add, tmp_buf);
#endif		
	
	for(i=0;i<flash_dev_info->page_size;i++){
		if(*(write_data+i)!=*(tmp_buf+i)){
		err_flag=FAIL;
		break;
		}
	}
	if(err_flag!=PASS){
		printf("PageWrite Verify error(%x) : Invalid Blocks are detected while Real Time Mapping(ADD= %x )!!\n",err_flag,(page_add*flash_dev_info->page_size));
	return FAIL;
	}
	
	return PASS;
	

}

int pagereadfs(UINT64 page_add, UINT8* page_data)
{

	UINT32 i,j,data,nwidth,nopcode,dent_bit;//,po;
	UINT8  *adr=page_add,ecc,ecc_byte,ecc_bit,oobsize;
	//UINT8 dev_oob[PAGE2K_OOB_SIZE],oob[PAGE2K_OOB_SIZE];
	nwidth = NFLASH_WiDTH8;
	UINT32 *oobsel,*data32;
	
	page_add = (UINT32)page_add;
	//if(page_add > SL2312_FLASH_BASE)
	// page_add -= SL2312_FLASH_BASE;
	page_add /= flash_dev_info->page_size;
	
	ndirect = NFLASH_DIRECT;
		nmode = 1 ;	

	oobsize = PAGE512_OOB_SIZE*flash_dev_info->page_size/PAGE512_SIZE;
	
	if(oobsize>PAGE512_OOB_SIZE)
		oobsel = &nand_oob_64;
	else
		oobsel = &nand_oob_16;

#ifdef BOARD_SUPPORT_NAND_INDIRECT
	if(flash_dev_info->page_size < PAGE512_RAW_SIZE)
	{
/*		ADD5=(UINT32)((page_add<<8)>>32);
		ADD4=(UINT32)((page_add<<16)>>32);
		ADD3=(UINT32)((page_add<<24)>>32);
		ADD2=(UINT32)((page_add<<32)>>32);
*/
		ADD5 = (page_add & 0xff000000)>>24;
		ADD4 = (page_add & 0x00ff0000)>>16;
		ADD3 = (page_add & 0x0000ff00)>>8;
		ADD2 = (page_add & 0x000000ff);
    }
    else
    {
		ADD5=(UINT32)((page_add<<16)>>32)&0xff;
		ADD4=(UINT32)((page_add<<24)>>32)&0xff;
		ADD3=(UINT32)((page_add<<32)>>32)&0xff;
		
    }
    
    dent_bit=FLASH_CTRL_READ_REG(FLASH_TYPE);
	switch(dent_bit&FLASH_SIZE_MASK)
	{
		case FLASH_SIZE_32:
			FLASH_CTRL_WRITE_REG(NFLASH_COUNT, 0x0f01ff20);
		    nopcode = 0x0;
		break;
		
		case FLASH_SIZE_64:
		case FLASH_SIZE_128:
			FLASH_CTRL_WRITE_REG(NFLASH_COUNT, 0x0f01ff30);
		    nopcode = 0x0;
		break;
		
		case FLASH_SIZE_256:
			FLASH_CTRL_WRITE_REG(NFLASH_COUNT, 0x3f07ff41);
		    nopcode = 0x00003000;
		break;
	}
		nopcode |= (ADD5<<24);
		FLASH_CTRL_WRITE_REG(NFLASH_COMMAND_ADDRESS, nopcode); //write address 0x00
		
			nopcode = 0x0|(ADD4<<24)|(ADD3<<16)|(ADD2<<8);
		FLASH_CTRL_WRITE_REG(NFLASH_ADDRESS, nopcode); //write address 0x00
#if 1
//4 bytes access
		data32 = (UINT32 *) page_data;
		for(i=0;i<((flash_dev_info->page_size+oobsize)/4);i++)
		{
			nopcode = FLASH_START_BIT | FLASH_RD|chip0_en|NFLASH_WiDTH32|NFLASH_INDIRECT; //set start bit & 8bits read command
			FLASH_CTRL_WRITE_REG(NFLASH_ACCESS, nopcode); 
			
			while(nopcode&FLASH_START_BIT) //polling flash access 31b
      			{
        		   nopcode=FLASH_CTRL_READ_REG(NFLASH_ACCESS);
        		   hal_delay_us(2);
      			}
      			    
      		 	    data = FLASH_CTRL_READ_REG(NFLASH_DATA);    
      		 	   // printf(" %08x ",data);
      		 	    ///////////
    				////////
      		 	    data32[i] = data;
      		 	    hal_delay_us(2); 
		}
#else	
//1 byte access	
		for(i=0;i<(flash_dev_info->page_size+oobsize);i++)
		{
			nopcode = FLASH_START_BIT | FLASH_RD|NFLASH_CHIP0_EN|NFLASH_WiDTH8|NFLASH_INDIRECT; //set start bit & 8bits read command
			FLASH_CTRL_WRITE_REG(NFLASH_ACCESS, nopcode); 
			
			while(nopcode&FLASH_START_BIT) //polling flash access 31b
      			{
        		   nopcode=FLASH_CTRL_READ_REG(NFLASH_ACCESS);
        		   hal_delay_us(2);
      			}
      			    
      		 	    data = FLASH_CTRL_READ_REG(NFLASH_DATA);    
      		 	    ///////////  		 	
      		 	    if((i&ECC_CHK_MASK)==0x01)
    					data>>=8;
    				else if((i&ECC_CHK_MASK)==0x02)
    					data>>=16;
    				else if((i&ECC_CHK_MASK)==0x03)
    					data>>=24;
    				////////
      		 	    page_data[i] = (UINT8)data;
		}
#endif    
#else   	
	FLASH_CTRL_WRITE_REG(NFLASH_ACCESS, NFLASH_DIRECT);
		

		for(i=0;i<(flash_dev_info->page_size+oobsize);i++)
		{
			page_data[i] = *adr++;
		}
#endif	
	return PASS;

}

int pagewritefs(void* addr, void* data, int len, int verify)
{
	UINT32 i,j,tt,nwidth,err_flag=PASS,dent_bit,nopcode;
	UINT32 page_add = (UINT32*)addr;
	UINT8  *adr=addr,ecc_buf[3];//,oob[PAGE2K_OOB_SIZE];
	UINT8  *write_data = (UINT8*)data ;
	UINT32 *oobsel,oobsize,*wdata32;
	
	nwidth = NFLASH_WiDTH8;
	
	//ndirect = 0;
	//	nmode = 1 ;
	ndirect = NFLASH_DIRECT;
		nmode = 1;	
	
	oobsize = PAGE512_OOB_SIZE*flash_dev_info->page_size/PAGE512_SIZE;
	
	FLASH_CTRL_WRITE_REG(NFLASH_ACCESS, NFLASH_DIRECT);
	memset(oob, 0xff, oobsize);	
		
	
	
	if(oobsize>PAGE512_OOB_SIZE)
		oobsel = &nand_oob_64;
	else
		oobsel = &nand_oob_16;

		
		
		//disable ecc
	FLASH_CTRL_WRITE_REG(NFLASH_ECC_CONTROL, 0x0);
#ifdef BOARD_SUPPORT_NAND_INDIRECT	
	page_add = (UINT32)page_add;
	//if(page_add > SL2312_FLASH_BASE)
	// page_add -= SL2312_FLASH_BASE;
	page_add /= flash_dev_info->page_size;
	
	ADD5=ADD4=ADD3=ADD2=0;
	if(flash_dev_info->page_size < PAGE512_RAW_SIZE)
	{
		ADD5=(UINT32)(((UINT64)page_add<<8)>>32)&0xff;
		ADD4=(UINT32)(((UINT64)page_add<<16)>>32)&0xff;
		ADD3=(UINT32)(((UINT64)page_add<<24)>>32)&0xff;
		ADD2=(UINT32)(((UINT64)page_add<<32)>>32)&0xff;
	}
	else
	{
		ADD5=(UINT32)(((UINT64)page_add<<16)>>32)&0xff;
		ADD4=(UINT32)(((UINT64)page_add<<24)>>32)&0xff;
		ADD3=(UINT32)(((UINT64)page_add<<32)>>32)&0xff;
	
	}
	dent_bit=FLASH_CTRL_READ_REG(FLASH_TYPE);
		switch(dent_bit&FLASH_SIZE_MASK)
		{
			case FLASH_SIZE_32:
				FLASH_CTRL_WRITE_REG(NFLASH_COUNT, 0x0f01ff21); 
		    	nopcode = 0x00001080;
			break;
			
			case FLASH_SIZE_64:
			case FLASH_SIZE_128:
				FLASH_CTRL_WRITE_REG(NFLASH_COUNT, 0x0f01ff31); 
		    	nopcode = 0x00001080;
			break;
			
			case FLASH_SIZE_256:
				FLASH_CTRL_WRITE_REG(NFLASH_COUNT, 0x3f07ff41); 
		    	nopcode = 0x00001080;
			break;
		}
	
		nopcode |= (ADD5<<24);
		FLASH_CTRL_WRITE_REG(NFLASH_COMMAND_ADDRESS, nopcode); 
		    
		nopcode = 0x0|(ADD4<<24)|(ADD3<<16)|(ADD2<<8);
		FLASH_CTRL_WRITE_REG(NFLASH_ADDRESS, nopcode); 
		for(i=0;i<(flash_dev_info->page_size+oobsize);i++)
		{
			tt = write_data[i];
			////////
    		if((i&0x03)==0x01)
    			tt<<=8;
    		else if((i&0x03)==0x02)
    			tt<<=16;
    		else if((i&0x03)==0x03)
    			tt<<=24;
    		////////
			
			FLASH_CTRL_WRITE_REG(NFLASH_DATA, tt); 
			nopcode = FLASH_START_BIT | FLASH_WT |NFLASH_CHIP0_EN|NFLASH_WiDTH8|NFLASH_INDIRECT; //set start bit & 8bits read command
			FLASH_CTRL_WRITE_REG(NFLASH_ACCESS, nopcode); 
			
			while(nopcode&FLASH_START_BIT) 
      		{
        	   nopcode=FLASH_CTRL_READ_REG(NFLASH_ACCESS);
        	   hal_delay_us(2);
      		}    		 	
		}
		
#else	
		FLASH_CTRL_WRITE_REG(NFLASH_ACCESS, NFLASH_DIRECT);
		for(i=0;i<(flash_dev_info->page_size+oobsize);i++)
		{
			*adr++ = write_data[i] ;
		}
#endif	
		err_flag = PASS;

	if( StatusCheck(chip0_en, def_width) ==FAIL)
		err_flag = FLASH_ERR_PROGRAM;
		
	if(verify)
	{
		pagereadfs((page_add*flash_dev_info->page_size), tmp_buf);
	
		for(i=0;i<(flash_dev_info->page_size+oobsize);i++){
			if(*(write_data+i)!=*(tmp_buf+i)){
			err_flag=FAIL;
			break;
			}
		}
		if(err_flag!=PASS){
			printf("PageWritefs Verify error : Invalid Blocks are detected while Real Time Mapping(ADD= %x )!!\n",(page_add*flash_dev_info->page_size));
		return FAIL;
		}
	}
	
	return PASS;
	

}

UINT32	VeffBad(UINT64 page_add)
{
	//UINT32	j,buf,count=1;
	UINT32	i,j,dent_bit,nopcode,data,tt;
    unsigned int oobsize, markp;
    unsigned char *oobp;
    UINT64 addr;
    
    page_add = (UINT32)page_add;
	oobsize = PAGE512_OOB_SIZE*flash_dev_info->page_size/PAGE512_SIZE;	
		if(oobsize>PAGE512_OOB_SIZE)
			markp = 0;//first byte
		else
			markp = 5; //6th byte
		
			//check is bad block

				for(j=0;j<2;j++) //check first and second page
				{
					// tmp = page_add;
					// for(i=0;i<(flash_dev_info->page_size/4);i++)
					// 	noob[i%4]=*tmp++;
					memset(tmp_buf,0xff,flash_dev_info->page_size+oobsize);
#ifdef BOARD_SUPPORT_NAND_INDIRECT

						//pageread((UINT64)page_add,tmp_buf);
					page_add = (UINT32)page_add;
					//if(page_add > SL2312_FLASH_BASE)
					// page_add -= SL2312_FLASH_BASE;
					addr = page_add;
					addr /= flash_dev_info->page_size;
					
					if(flash_dev_info->page_size < PAGE512_RAW_SIZE)
					{
						ADD5 = (addr & 0xff000000)>>24;
						ADD4 = (addr & 0x00ff0000)>>16;
						ADD3 = (addr & 0x0000ff00)>>8;
						ADD2 = (addr & 0x000000ff);
    				}
    				else
    				{
						ADD5=(UINT32)((addr<<16)>>32)&0xff;
						ADD4=(UINT32)((addr<<24)>>32)&0xff;
						ADD3=(UINT32)((addr<<32)>>32)&0xff;
						
    				}
    				
    				dent_bit=FLASH_CTRL_READ_REG(FLASH_TYPE);
					switch(dent_bit&FLASH_SIZE_MASK)
					{
						case FLASH_SIZE_32:
							FLASH_CTRL_WRITE_REG(NFLASH_COUNT, 0x0f01ff20);
						    nopcode = 0x0;
						break;
						
						case FLASH_SIZE_64:
						case FLASH_SIZE_128:
							FLASH_CTRL_WRITE_REG(NFLASH_COUNT, 0x0f01ff30);
						    nopcode = 0x0;
						break;
						
						case FLASH_SIZE_256:
							FLASH_CTRL_WRITE_REG(NFLASH_COUNT, 0x3f07ff41);
						    nopcode = 0x00003000;
						break;
					}
					nopcode |= (ADD5<<24);
					FLASH_CTRL_WRITE_REG(NFLASH_COMMAND_ADDRESS, nopcode); //write address 0x00
					
						nopcode = 0x0|(ADD4<<24)|(ADD3<<16)|(ADD2<<8);
					FLASH_CTRL_WRITE_REG(NFLASH_ADDRESS, nopcode); //write address 0x00
					for(i=0;i<(flash_dev_info->page_size+oobsize);i++)
					{
						nopcode = FLASH_START_BIT | FLASH_RD|NFLASH_CHIP0_EN|NFLASH_WiDTH8|NFLASH_INDIRECT; //set start bit & 8bits read command
						FLASH_CTRL_WRITE_REG(NFLASH_ACCESS, nopcode); 
						
						while(nopcode&FLASH_START_BIT) //polling flash access 31b
      						{
        					   nopcode=FLASH_CTRL_READ_REG(NFLASH_ACCESS);
        					   hal_delay_us(2);
      						}
      						    
      					 	    data = FLASH_CTRL_READ_REG(NFLASH_DATA);    
      					 	    ///////////  		 	
      					 	    if((i&ECC_CHK_MASK)==0x01)
    								data>>=8;
    							else if((i&ECC_CHK_MASK)==0x02)
    								data>>=16;
    							else if((i&ECC_CHK_MASK)==0x03)
    								data>>=24;
    								
    							////////
      					 	    tmp_buf[i] = (UINT8)data;
					}
#else    
    				FLASH_CTRL_WRITE_REG(NFLASH_ACCESS, NFLASH_DIRECT);
									
					if(page_add<SL2312_FLASH_BASE)
						page_add += SL2312_FLASH_BASE;
						
						oobp = (unsigned char *)page_add;
					 for(i=0;i<(flash_dev_info->page_size+oobsize);i++)
					 	tmp_buf[i]=*oobp++;
#endif
					
					if(tmp_buf[flash_dev_info->page_size+markp]==0xff)
					{
						page_add += flash_dev_info->page_size;
					}
					else
						return FAIL;
				}

	return PASS;
	
} 

UINT32	nand_block_markbad(UINT64 page_add)
{
	UINT32	i,j,err_flag=0;
	UINT64 addr;
    unsigned int oobsize, markp, dent_bit,nopcode,tt;
    unsigned char *oobp;
    
    
	oobsize = PAGE512_OOB_SIZE*flash_dev_info->page_size/PAGE512_SIZE;
	
	page_add = (UINT32)page_add;
	printf("\nBlock(addr : 0x%x)mark bad!\n",page_add);
	
		if(oobsize>PAGE512_OOB_SIZE)
			markp = 0;//first byte
		else
			markp = 5; //6th byte
		
			//check is bad block
				memset(wt_buff,0xff,flash_dev_info->page_size+oobsize);
				//memset(tmp_buf+flash_dev_info->page_size,0xff,oobsize);
				wt_buff[flash_dev_info->page_size+markp] = 0;
				for(j=0;j<2;j++) //check first and second page
				{
					
#ifdef BOARD_SUPPORT_NAND_INDIRECT
					pagewritefs(page_add, wt_buff, flash_dev_info->page_size+oobsize, 0);
#if 0
					addr = page_add;
					addr /= flash_dev_info->page_size;
					
					if(flash_dev_info->page_size < PAGE512_RAW_SIZE)
					{
						ADD5 = (addr & 0xff000000)>>24;
						ADD4 = (addr & 0x00ff0000)>>16;
						ADD3 = (addr & 0x0000ff00)>>8;
						ADD2 = (addr & 0x000000ff);
    				}
    				else
    				{
						ADD5=(UINT32)((addr<<16)>>32)&0xff;
						ADD4=(UINT32)((addr<<24)>>32)&0xff;
						ADD3=(UINT32)((addr<<32)>>32)&0xff;
						
    				}
    				
    				dent_bit=FLASH_CTRL_READ_REG(FLASH_TYPE);
					switch(dent_bit&FLASH_SIZE_MASK)
					{
						case FLASH_SIZE_32:
							FLASH_CTRL_WRITE_REG(NFLASH_COUNT, 0x0f01ff20);
						    nopcode = 0x0;
						break;
						
						case FLASH_SIZE_64:
						case FLASH_SIZE_128:
							FLASH_CTRL_WRITE_REG(NFLASH_COUNT, 0x0f01ff30);
						    nopcode = 0x0;
						break;
						
						case FLASH_SIZE_256:
							FLASH_CTRL_WRITE_REG(NFLASH_COUNT, 0x3f07ff41);
						    nopcode = 0x00003000;
						break;
					}
					nopcode |= (ADD5<<24);
					FLASH_CTRL_WRITE_REG(NFLASH_COMMAND_ADDRESS, nopcode); //write address 0x00
					
						nopcode = 0x0|(ADD4<<24)|(ADD3<<16)|(ADD2<<8);
					FLASH_CTRL_WRITE_REG(NFLASH_ADDRESS, nopcode); //write address 0x00
					for(i=0;i<(flash_dev_info->page_size+oobsize);i++)
					{
						tt = tmp_buf[i];
						////////
    					if((i&0x03)==0x01)
    						tt<<=8;
    					else if((i&0x03)==0x02)
    						tt<<=16;
    					else if((i&0x03)==0x03)
    						tt<<=24;
    					////////
    					if(i==flash_dev_info->page_size)
    					 printf("ii\n");
						
						FLASH_CTRL_WRITE_REG(NFLASH_DATA, tt); 
						nopcode = FLASH_START_BIT | FLASH_WT |NFLASH_CHIP0_EN|NFLASH_WiDTH8|NFLASH_INDIRECT; //set start bit & 8bits read command
						FLASH_CTRL_WRITE_REG(NFLASH_ACCESS, nopcode); 
						
						while(nopcode&FLASH_START_BIT) 
      					{
        				   nopcode=FLASH_CTRL_READ_REG(NFLASH_ACCESS);
        				   hal_delay_us(2);
      					}    
					}
#endif					
#else
					
					FLASH_CTRL_WRITE_REG(NFLASH_ACCESS, NFLASH_DIRECT);
					oobp = (unsigned char *)page_add;
					if(page_add<SL2312_FLASH_BASE)
						oobp +=SL2312_FLASH_BASE;
						
					 for(i=0;i<(flash_dev_info->page_size+oobsize);i++)
					 	*oobp++=tmp_buf[i];
					 	
#endif
					page_add += flash_dev_info->page_size;
				}
				
	if( StatusCheck(chip0_en, def_width) ==FAIL)
		err_flag = FLASH_ERR_PROGRAM;

	return PASS;
}

/*----------------------------------------------------------------------
* cli_nand_cmd
*----------------------------------------------------------------------*/

void cli_nand_cmd(char argc, char *argv[])
{
    int				err, got_size = 0;
    unsigned int 	i,start, end,sts, oob_size;
    char			*cp, *bufp;
    static UINT32	location=0, size=1;
    hal_flash_enable();
	err = 0;
	oob_size = PAGE512_OOB_SIZE*flash_dev_info->page_size/PAGE512_SIZE;
    while(argc > 0 && !err)
    {
		if (argc == 2 && strcmp(argv[1],"info") == 0) {
			printf("nand flash info\n");
			printf("Vender ID: 0x%x ,Device ID: 0x%x  \n",flash_dev_info->vendor_id,flash_dev_info->device_id);
			printf("Flash size : 0x%x(%dMB)\n",flash_dev_info->device_size,(flash_dev_info->device_size >> 20));
			printf("Block cnt : 0x%x\n",flash_dev_info->block_count);
			printf("Block size: 0x%x (page cnt : 0x%x)\n",flash_dev_info->block_size,flash_dev_info->block_size / flash_dev_info->page_size);
			printf("Page size: 0x%x (Oob size:0x%x)\n",flash_dev_info->page_size,(flash_dev_info->page_size>PAGE512_SIZE)?PAGE2K_OOB_SIZE:PAGE512_OOB_SIZE);
			
			break;
		}                   
		else if (argc == 5 && strcmp(argv[1],"erase") == 0) {
			start = str2value(argv[3]);
			start = start & (~(flash_dev_info->block_size-1));
			end = (start + str2value(argv[4])) & (~(flash_dev_info->block_size-1));
			
			printf("nand erase from 0x%x to 0x%x\n",start,end);
			
    		for(i=start;i<end;i+=flash_dev_info->block_size){
    			if (strcmp(argv[2],"nobad") == 0)
    			{
    				sts = VeffBad((unsigned char *)i);
    			}
    			else
    				sts = PASS;
    			
    				if(sts == PASS)
    				{
						if(flash_erase_block((UINT64)i, flash_dev_info->block_size)==FLASH_ERR_ERASE)   	    // Executing erase operation only on the valid blocks
						{
							nand_block_markbad((UINT64)i);
							printf("\nBlock %x bad !!\n",i/flash_dev_info->block_size);
						}
					}
			}
			break;
		}
		else if (argc == 3 && strcmp(argv[1],"read") == 0) {
			
			start = str2value(argv[2]);
			start = start & (~(flash_dev_info->page_size-1));
			printf("nand read a page addr : 0x%x\n",start);
#ifndef BOARD_SUPPORT_NAND_INDIRECT						
			start += SL2312_FLASH_BASE; 
#endif			
			
			memset(tmp_buf, 0xff, (flash_dev_info->page_size+oob_size));
			
			if(StatusCheck(chip0_en, def_width)!=FAIL)
			{
				if(pageread((UINT64)start,tmp_buf)!=PASS){
					err = FLASH_ERR_HWR;
					printf("Flash read error!!\n");
					break;
				}
			}
	
			for(i=0;i<(flash_dev_info->page_size+oob_size);i++)
			{
				if((i%0x10) == 0)
					printf("\n0x%08x :",i);
				printf(" %02x",tmp_buf[i]);
			}
			printf("\n");
			break;
		}
		else if (argc == 4 && strcmp(argv[1],"write") == 0) {
			start = str2value(argv[2]);
			start = start & (~(flash_dev_info->page_size-1));
			end = str2value(argv[3]);
			printf("nand write addr : 0x%x  size: 0x%x\n",start,end);
#ifndef BOARD_SUPPORT_NAND_INDIRECT			
			start += SL2312_FLASH_BASE; 
#endif			
			bufp = (char *)malloc(end);
			if (!bufp)
			{
				printf("No free memory !\n");
				return;
			}
			
			if (!do_tftp_download(bufp, end, &got_size))
			{
				free(bufp);
				return;
			}
			
			if(got_size < end)
				printf("got size less then write size!!\n");
			flash_program_buf(start,bufp,end);
			break;
		}
		else if (argc == 4 && strcmp(argv[1],"writefs") == 0) {
			start = str2value(argv[2]);
			start = start & (~(flash_dev_info->page_size-1));
			end = str2value(argv[3]);
			printf("nand write fs addr : 0x%x  size: 0x%x\nPls",start,end);
#ifndef BOARD_SUPPORT_NAND_INDIRECT						
			start += SL2312_FLASH_BASE; 
#endif			
			bufp = (char *)malloc(end);
			if (!bufp)
			{
				printf("No free memory !\n");
				return ;
			}
			
			if (!do_tftp_download(bufp, end, &got_size))
			{
				free(bufp);
				return ;
			}
			
			if(got_size < end)
				printf("got fs size less then write size!!\n");
			flash_fs_nand_program(start,bufp,end);
			break;
		}
		else if (argc == 3 && strcmp(argv[1],"readfs") == 0) {
			
			start = str2value(argv[2]);
			start = start & (~(flash_dev_info->page_size-1));
			printf("nand readfs a page addr : 0x%x\n",start);
#ifndef BOARD_SUPPORT_NAND_INDIRECT						
			start += SL2312_FLASH_BASE; 
#endif			
			
			memset(tmp_buf, 0xff, (flash_dev_info->page_size+oob_size));
			
			if(StatusCheck(chip0_en, def_width)!=FAIL)
			{
				if(pagereadfs((UINT64)start,tmp_buf)!=PASS){
					err = FLASH_ERR_HWR;
					printf("Flash readfs error!!\n");
					break;
				}
			}
	
			for(i=0;i<(flash_dev_info->page_size+oob_size);i++)
			{
				if((i%0x10) == 0)
					printf("\n0x%08x :",i);
				printf(" %02x",tmp_buf[i]);
			}
			printf("\n");
			break;
		}
		else if (argc == 2 && strcmp(argv[1],"bad") == 0) {
			printf("nand show bad block.\n");
			for(i=0;i<flash_dev_info->block_count;i++){
				if(VeffBad((unsigned char *)(i*(UINT64)flash_dev_info->block_size))!=PASS)
					printf("Block(%x) : 0x%x bad !!\n",i,i*(UINT64)flash_dev_info->block_size);

			}
			break;
		}
		else if (argc == 3 && strcmp(argv[1],"markbad") == 0) {
			start = str2value(argv[2]);
			start = start & (~(flash_dev_info->block_size-1));
			printf("nand mark 0x%x bad\n",start);
#ifndef BOARD_SUPPORT_NAND_INDIRECT				
			start += SL2312_FLASH_BASE;
#endif			
			if(VeffBad((unsigned char *)(start))==PASS)
			{
				nand_block_markbad((UINT64)start);
				printf("Mark bad OK\n");
			}
			else
				printf("Block was bad already!!\n");
			
			break;
		}
		else {
			err = 1;
			
    		printf("Usage:  nand info - show available NAND devices\n" );
    		printf("	nand erase [nobad|bad] <offset> <len> - erase from <addr> total size <len>\n");
    		printf("	nand read <page_addr> - read a page data include oob\n"  );
    		printf("	nand write <page_addr> <size> - write a page data\n"  );
    		printf("	nand bad - show bad blocks\n"  );
    		printf("	nand markbad <block_addr> - mark bad block at <block_addr>\n");
			break;
		} 
    }; // while        
	hal_flash_disable();
	printf("\n");
    
}


int flash_fs_nand_program(void *addr, void *data, int len, unsigned long *err_addr, unsigned long bound)
{
	int res=FLASH_ERR_OK, size, page_size;
	unsigned char *paddr=addr;
	unsigned char *pdata=data;
	//UINT8 wt_buff[PAGE2K_RAW_SIZE];
	
	page_size = flash_dev_info->page_size>PAGE512_SIZE?PAGE2K_RAW_SIZE:PAGE512_RAW_SIZE;
	
	while(len>0) {

		//if(StatusCheck(chip0_en, def_width)!=FAIL)
		{
			if(len < page_size)
			{
				memset(wt_buff,0xff,PAGE2K_RAW_SIZE);
				memcpy(wt_buff,pdata,len);
				size = len;
				if( pagewritefs(paddr, wt_buff, page_size, 1) != PASS)
				return FLASH_ERR_PROGRAM;
			}
			else
			{
				size = page_size;
				if( pagewritefs(paddr, pdata, page_size, 1) != PASS)
					return FLASH_ERR_PROGRAM;
			}
			flash_delay();
			len -= size;
			paddr += flash_dev_info->page_size;
			pdata += size;
		}
	}
	return res;
}
	


#endif //#ifdef BOARD_NAND_BOOT		
//#endif // CYGONCE_DEVS_FLASH_TOSHIBA_TC58XXX_INL

// EOF flash_tc58xxx.inl
//#endif // FLASH_TYPE_NAND

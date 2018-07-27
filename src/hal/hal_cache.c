/****************************************************************************
* Copyright  Storlink Corp 2005.  All rights reserved.                
*--------------------------------------------------------------------------
* Name			: hal_cache.c
* Description	: 
*		Handle cache functions
*
* History
*
*	Date		Writer		Description
*	-----------	-----------	-------------------------------------------------
*	04/19/2005	Gary Chen	Create and implement from Jason's Redboot code
*
****************************************************************************/
#include <stdarg.h>
#include <define.h>
#include <board_config.h>
#include <sl2312.h>


// -------------------------------------------------------------------------
// MMU initialization:
// 
// These structures are laid down in memory to define the translation
// table.
// 

// ARM Translation Table Base Bit Masks
#define ARM_TRANSLATION_TABLE_MASK               0xFFFFC000

// ARM Domain Access Control Bit Masks
#define ARM_ACCESS_TYPE_NO_ACCESS(domain_num)    (0x0 << (domain_num)*2)
#define ARM_ACCESS_TYPE_CLIENT(domain_num)       (0x1 << (domain_num)*2)
#define ARM_ACCESS_TYPE_MANAGER(domain_num)      (0x3 << (domain_num)*2)

struct ARM_MMU_FIRST_LEVEL_FAULT {
    int id : 2;
    int sbz : 30;
};
#define ARM_MMU_FIRST_LEVEL_FAULT_ID 0x0

struct ARM_MMU_FIRST_LEVEL_PAGE_TABLE {
    int id : 2;
    int imp0 : 2;
    int sb1  : 1;
    int domain : 4;
    int imp1 : 1;
    int base_address : 22;
};
#define ARM_MMU_FIRST_LEVEL_PAGE_TABLE_ID 0x1

struct ARM_MMU_FIRST_LEVEL_SECTION {
    int id : 2;
    int b : 1;
    int c : 1;
    int imp : 1;
    int domain : 4;
    int sbz0 : 1;
    int ap : 2;
    int sbz1 : 8;
    int base_address : 12;
};
#define ARM_MMU_FIRST_LEVEL_SECTION_ID 0x2

struct ARM_MMU_FIRST_LEVEL_RESERVED {
    int id : 2;
    int sbz : 30;
};
#define ARM_MMU_FIRST_LEVEL_RESERVED_ID 0x3

#define ARM_MMU_FIRST_LEVEL_DESCRIPTOR_ADDRESS(ttb_base, table_index) \
   (unsigned long *)((unsigned long)(ttb_base) + ((table_index) << 2))

#define ARM_FIRST_LEVEL_PAGE_TABLE_SIZE 0x4000

#define ARM_MMU_SECTION(ttb_base, actual_base, virtual_base,              \
                        cacheable, bufferable, perm)                      \
        register union ARM_MMU_FIRST_LEVEL_DESCRIPTOR desc;               \
                                                                          \
        desc.word = 0;                                                    \
        desc.section.id = ARM_MMU_FIRST_LEVEL_SECTION_ID;                 \
        desc.section.imp = 1;                                             \
        desc.section.domain = 0;                                          \
        desc.section.c = (cacheable);                                     \
        desc.section.b = (bufferable);                                    \
        desc.section.ap = (perm);                                         \
        desc.section.base_address = (actual_base);                        \
        *ARM_MMU_FIRST_LEVEL_DESCRIPTOR_ADDRESS(ttb_base, (virtual_base)) \
                            = desc.word;                                  \

#define X_ARM_MMU_SECTION(abase,vbase,size,cache,buff,access)      \
    { int i; int j = abase >> 20; int k = vbase >> 20;             \
      for (i = size; i > 0 ; i--,j++,k++)                          \
      {                                                            \
        ARM_MMU_SECTION(ttb_base, j, k, cache, buff, access);      \
      }                                                            \
    }

union ARM_MMU_FIRST_LEVEL_DESCRIPTOR {
    unsigned long word;
    struct ARM_MMU_FIRST_LEVEL_FAULT fault;
    struct ARM_MMU_FIRST_LEVEL_PAGE_TABLE page_table;
    struct ARM_MMU_FIRST_LEVEL_SECTION section;
    struct ARM_MMU_FIRST_LEVEL_RESERVED reserved;
};

#define ARM_UNCACHEABLE                         0
#define ARM_CACHEABLE                           1
#define ARM_UNBUFFERABLE                        0
#define ARM_BUFFERABLE                          1

#define ARM_ACCESS_PERM_NONE_NONE               0
#define ARM_ACCESS_PERM_RO_NONE                 0
#define ARM_ACCESS_PERM_RO_RO                   0
#define ARM_ACCESS_PERM_RW_NONE                 1
#define ARM_ACCESS_PERM_RW_RO                   2
#define ARM_ACCESS_PERM_RW_RW                   3

extern char ttb_base[];
// char ttb_base[ARM_FIRST_LEVEL_PAGE_TABLE_SIZE] __attribute__((aligned(0x4000)));			
/*----------------------------------------------------------------------
* hal_mmu_init
*----------------------------------------------------------------------*/
void hal_mmu_init(void)
{
    // unsigned long 	ttb_base = SL2312_DRAM_BASE + 0x40000;
    unsigned long 	i;
    char 			*cp;

    // Set the TTB register
    asm volatile ("mcr  p15,0,%0,c2,c0,0" : : "r"(ttb_base) /*:*/);

    // Set the Domain Access Control Register
    i = ARM_ACCESS_TYPE_MANAGER(0)    | 
        ARM_ACCESS_TYPE_NO_ACCESS(1)  |
        ARM_ACCESS_TYPE_NO_ACCESS(2)  |
        ARM_ACCESS_TYPE_NO_ACCESS(3)  |
        ARM_ACCESS_TYPE_NO_ACCESS(4)  |
        ARM_ACCESS_TYPE_NO_ACCESS(5)  |
        ARM_ACCESS_TYPE_NO_ACCESS(6)  |
        ARM_ACCESS_TYPE_NO_ACCESS(7)  |
        ARM_ACCESS_TYPE_NO_ACCESS(8)  |
        ARM_ACCESS_TYPE_NO_ACCESS(9)  |
        ARM_ACCESS_TYPE_NO_ACCESS(10) |
        ARM_ACCESS_TYPE_NO_ACCESS(11) |
        ARM_ACCESS_TYPE_NO_ACCESS(12) |
        ARM_ACCESS_TYPE_NO_ACCESS(13) |
        ARM_ACCESS_TYPE_NO_ACCESS(14) |
        ARM_ACCESS_TYPE_NO_ACCESS(15);
    asm volatile ("mcr  p15,0,%0,c3,c0,0" : : "r"(i) /*:*/);

    // First clear all TT entries - ie Set them to Faulting
    cp = (char *)ttb_base;
    for (i=0; i<ARM_FIRST_LEVEL_PAGE_TABLE_SIZE; i++, cp)
    	*cp++ = 0x00;

    // Memory layout. This is set up in hal_platform_setup.h with
    // definitions from sl2312.h
    //

    //               Actual  Virtual  Size   Attributes                                                    Function
    //		     Base     Base     MB      cached?           buffered?        access permissions
    //             xxx00000  xxx00000
#ifdef BOARD_DRAM_CACHE_BASE
    X_ARM_MMU_SECTION(BOARD_DRAM_CACHE_BASE, 
    				  BOARD_DRAM_CACHE_BASE, 
    				  BOARD_DRAM_CACHE_SIZE / (1024 * 1024),  
    				  ARM_CACHEABLE,   ARM_BUFFERABLE,   ARM_ACCESS_PERM_RW_RW); // SDRAM space
    X_ARM_MMU_SECTION(BOARD_DRAM_NONCACHE_BASE, 
    				  BOARD_DRAM_NONCACHE_BASE,
    				  BOARD_DRAM_NONCACHE_SIZE / (1024 * 1024),
    				  ARM_UNCACHEABLE,   ARM_UNBUFFERABLE,   ARM_ACCESS_PERM_RW_RW); // SDRAM space
#else
    X_ARM_MMU_SECTION(0x00000000,  0x00000000,   128,  ARM_CACHEABLE,   ARM_BUFFERABLE,   ARM_ACCESS_PERM_RW_RW); // SDRAM space
#endif
#if 1
#ifndef MIDWAY
    X_ARM_MMU_SECTION(0x10000000,  0x10000000,   128,  ARM_CACHEABLE,   ARM_BUFFERABLE,   ARM_ACCESS_PERM_RW_RW); // Duplicate SDRAM space
    X_ARM_MMU_SECTION(0x20000000,  0x20000000,   256,  ARM_UNCACHEABLE, ARM_UNBUFFERABLE, ARM_ACCESS_PERM_RW_RW); // Peripherial
    X_ARM_MMU_SECTION(0x30000000,  0x30000000,   256,  ARM_UNCACHEABLE, ARM_UNBUFFERABLE, ARM_ACCESS_PERM_RW_RW); // PCI register
    X_ARM_MMU_SECTION(0x40000000,  0x40000000,   256,  ARM_UNCACHEABLE, ARM_UNBUFFERABLE, ARM_ACCESS_PERM_RW_RW); // PCI memory
    X_ARM_MMU_SECTION(0x50000000,  0x50000000,   128,  ARM_UNCACHEABLE, ARM_UNBUFFERABLE, ARM_ACCESS_PERM_RW_RW); // Peripherial
    X_ARM_MMU_SECTION(0x70000000,  0x70000000,    16,  ARM_UNCACHEABLE, ARM_BUFFERABLE,   ARM_ACCESS_PERM_RW_RW); // Flash
#else
    X_ARM_MMU_SECTION(0x10000000,  0x10000000,   128,  ARM_CACHEABLE,   ARM_BUFFERABLE,   ARM_ACCESS_PERM_RW_RW); // Duplicate SDRAM space
    X_ARM_MMU_SECTION(0x20000000,  0x20000000,   256,  ARM_UNCACHEABLE, ARM_UNBUFFERABLE, ARM_ACCESS_PERM_RW_RW); // Peripherial
    X_ARM_MMU_SECTION(0x30000000,  0x30000000,   256,   ARM_UNCACHEABLE, ARM_BUFFERABLE,   ARM_ACCESS_PERM_RW_RW); // Flash
    X_ARM_MMU_SECTION(0x40000000,  0x40000000,   256,  ARM_UNCACHEABLE, ARM_UNBUFFERABLE, ARM_ACCESS_PERM_RW_RW); // PCI memory
    X_ARM_MMU_SECTION(0x50000000,  0x50000000,   256,  ARM_UNCACHEABLE, ARM_UNBUFFERABLE, ARM_ACCESS_PERM_RW_RW); // Peripherial
    X_ARM_MMU_SECTION(0x60000000,  0x60000000,   256,  ARM_UNCACHEABLE, ARM_UNBUFFERABLE, ARM_ACCESS_PERM_RW_RW); // Peripherial
#endif
#else
#ifdef SL2312_RAM_BASE
    X_ARM_MMU_SECTION(SL2312_RAM_BASE,  SL2312_RAM_BASE,   1,
    				ARM_CACHEABLE,   ARM_BUFFERABLE,   ARM_ACCESS_PERM_RW_RW); // Duplicate SDRAM space
#endif    
#ifdef SL2312_FLASH_SHADOW
    X_ARM_MMU_SECTION(SL2312_FLASH_SHADOW,  SL2312_FLASH_SHADOW,    16,  
    				ARM_UNCACHEABLE, ARM_BUFFERABLE,   ARM_ACCESS_PERM_RW_RW); // Flash
#endif
#ifdef SL2312_GLOBAL_BASE
    X_ARM_MMU_SECTION(SL2312_GLOBAL_BASE,  SL2312_GLOBAL_BASE,   1,  
    				ARM_UNCACHEABLE, ARM_UNBUFFERABLE, ARM_ACCESS_PERM_RW_RW);
#endif
#ifdef SL2312_WAQTCHDOG_BASE
    X_ARM_MMU_SECTION(SL2312_WAQTCHDOG_BASE,  SL2312_WAQTCHDOG_BASE,   1,  
    				ARM_UNCACHEABLE, ARM_UNBUFFERABLE, ARM_ACCESS_PERM_RW_RW);
#endif
#ifdef SL2312_UART_BASE
    X_ARM_MMU_SECTION(SL2312_UART_BASE,  SL2312_UART_BASE,   1,  
    				ARM_UNCACHEABLE, ARM_UNBUFFERABLE, ARM_ACCESS_PERM_RW_RW);
#endif
#ifdef SL2312_TIMER_BASE
    X_ARM_MMU_SECTION(SL2312_TIMER_BASE,  SL2312_TIMER_BASE,   1,  
    				ARM_UNCACHEABLE, ARM_UNBUFFERABLE, ARM_ACCESS_PERM_RW_RW);
#endif
#ifdef SL2312_LCD_BASE
    X_ARM_MMU_SECTION(SL2312_LCD_BASE,  SL2312_LCD_BASE,   1,  
    				ARM_UNCACHEABLE, ARM_UNBUFFERABLE, ARM_ACCESS_PERM_RW_RW);
#endif
#ifdef SL2312_RTC_BASE
    X_ARM_MMU_SECTION(SL2312_RTC_BASE,  SL2312_RTC_BASE,   1,  
    				ARM_UNCACHEABLE, ARM_UNBUFFERABLE, ARM_ACCESS_PERM_RW_RW);
#endif
#ifdef SL2312_SATA_BASE
    X_ARM_MMU_SECTION(SL2312_SATA_BASE,  SL2312_SATA_BASE,   1,  
    				ARM_UNCACHEABLE, ARM_UNBUFFERABLE, ARM_ACCESS_PERM_RW_RW);
#endif
#ifdef SL2312_LPC_HOST_BASE
    X_ARM_MMU_SECTION(SL2312_LPC_HOST_BASE,  SL2312_LPC_HOST_BASE,   1,  
    				ARM_UNCACHEABLE, ARM_UNBUFFERABLE, ARM_ACCESS_PERM_RW_RW);
#endif
#ifdef SL2312_LPC_IO_BASE
    X_ARM_MMU_SECTION(SL2312_LPC_IO_BASE,  SL2312_LPC_IO_BASE,   1,  
    				ARM_UNCACHEABLE, ARM_UNBUFFERABLE, ARM_ACCESS_PERM_RW_RW);
#endif
#ifdef SL2312_INTERRUPT_BASE
    X_ARM_MMU_SECTION(SL2312_INTERRUPT_BASE,  SL2312_INTERRUPT_BASE,   1,  
    				ARM_UNCACHEABLE, ARM_UNBUFFERABLE, ARM_ACCESS_PERM_RW_RW);
#endif
#ifdef SL2312_INTERRUPT1_BASE
    X_ARM_MMU_SECTION(SL2312_INTERRUPT1_BASE,  SL2312_INTERRUPT1_BASE,   1,  
    				ARM_UNCACHEABLE, ARM_UNBUFFERABLE, ARM_ACCESS_PERM_RW_RW);
#endif
#ifdef SL2312_SSP_CTRL_BASE
    X_ARM_MMU_SECTION(SL2312_SSP_CTRL_BASE,  SL2312_SSP_CTRL_BASE,   1,  
    				ARM_UNCACHEABLE, ARM_UNBUFFERABLE, ARM_ACCESS_PERM_RW_RW);
#endif
#ifdef SL2312_POWER_CTRL_BASE
    X_ARM_MMU_SECTION(SL2312_POWER_CTRL_BASE,  SL2312_POWER_CTRL_BASE,   1,  
    				ARM_UNCACHEABLE, ARM_UNBUFFERABLE, ARM_ACCESS_PERM_RW_RW);
#endif
#ifdef SL2312_CIR_BASE
    X_ARM_MMU_SECTION(SL2312_CIR_BASE,  SL2312_CIR_BASE,   1,  
    				ARM_UNCACHEABLE, ARM_UNBUFFERABLE, ARM_ACCESS_PERM_RW_RW);
#endif
#ifdef SL2312_GPIO_BASE
    X_ARM_MMU_SECTION(SL2312_GPIO_BASE,  SL2312_GPIO_BASE,   1,  
    				ARM_UNCACHEABLE, ARM_UNBUFFERABLE, ARM_ACCESS_PERM_RW_RW);
#endif
#ifdef SL2312_GPIO_BASE1
    X_ARM_MMU_SECTION(SL2312_GPIO_BASE1,  SL2312_GPIO_BASE1,   1,  
    				ARM_UNCACHEABLE, ARM_UNBUFFERABLE, ARM_ACCESS_PERM_RW_RW);
#endif
#ifdef SL2312_GPIO_BASE2
    X_ARM_MMU_SECTION(SL2312_GPIO_BASE2,  SL2312_GPIO_BASE2,   1,  
    				ARM_UNCACHEABLE, ARM_UNBUFFERABLE, ARM_ACCESS_PERM_RW_RW);
#endif
#ifdef SL2312_PCI_IO_BASE
    X_ARM_MMU_SECTION(SL2312_PCI_IO_BASE,  SL2312_PCI_IO_BASE,   1,  
    				ARM_UNCACHEABLE, ARM_UNBUFFERABLE, ARM_ACCESS_PERM_RW_RW);
#endif
#ifdef SL2312_PCI_MEM_BASE
    X_ARM_MMU_SECTION(SL2312_PCI_MEM_BASE,  SL2312_PCI_MEM_BASE,   128,  
    				ARM_UNCACHEABLE, ARM_UNBUFFERABLE, ARM_ACCESS_PERM_RW_RW);
#endif
#ifdef SL2312_GMAC0_BASE
    X_ARM_MMU_SECTION(SL2312_GMAC0_BASE,  SL2312_GMAC0_BASE,   1,  
    				ARM_UNCACHEABLE, ARM_UNBUFFERABLE, ARM_ACCESS_PERM_RW_RW);
#endif
#ifdef SL2312_GMAC1_BASE
    X_ARM_MMU_SECTION(SL2312_GMAC1_BASE,  SL2312_GMAC1_BASE,   1,  
    				ARM_UNCACHEABLE, ARM_UNBUFFERABLE, ARM_ACCESS_PERM_RW_RW);
#endif
#ifdef SL2312_EMAC_BASE
    X_ARM_MMU_SECTION(SL2312_EMAC_BASE,  SL2312_EMAC_BASE,   1,  
    				ARM_UNCACHEABLE, ARM_UNBUFFERABLE, ARM_ACCESS_PERM_RW_RW);
#endif
#ifdef SL2312_SECURITY_BASE
    X_ARM_MMU_SECTION(SL2312_SECURITY_BASE,  SL2312_SECURITY_BASE,   1,  
    				ARM_UNCACHEABLE, ARM_UNBUFFERABLE, ARM_ACCESS_PERM_RW_RW);
#endif
#ifdef SL2312_IDE_BASE
    X_ARM_MMU_SECTION(SL2312_IDE_BASE,  SL2312_IDE_BASE,   1,  
    				ARM_UNCACHEABLE, ARM_UNBUFFERABLE, ARM_ACCESS_PERM_RW_RW);
#endif
#ifdef SL2312_IDE0_BASE
    X_ARM_MMU_SECTION(SL2312_IDE0_BASE,  SL2312_IDE0_BASE,   128,  
    				ARM_UNCACHEABLE, ARM_UNBUFFERABLE, ARM_ACCESS_PERM_RW_RW);
#endif
//#ifdef SL2312_IDE1_BASE
//    X_ARM_MMU_SECTION(SL2312_IDE1_BASE,  SL2312_IDE1_BASE,   1,  
//    				ARM_UNCACHEABLE, ARM_UNBUFFERABLE, ARM_ACCESS_PERM_RW_RW);
//#endif
#ifdef SL2312_RAID_BASE
    X_ARM_MMU_SECTION(SL2312_RAID_BASE,  SL2312_RAID_BASE,   128,  
    				ARM_UNCACHEABLE, ARM_UNBUFFERABLE, ARM_ACCESS_PERM_RW_RW);
#endif
#ifdef SL2312_FLASH_CTRL_BASE
    X_ARM_MMU_SECTION(SL2312_FLASH_CTRL_BASE,  SL2312_FLASH_CTRL_BASE,   1,  
    				ARM_UNCACHEABLE, ARM_UNBUFFERABLE, ARM_ACCESS_PERM_RW_RW);
#endif
#ifdef SL2312_DRAM_CTRL_BASE
    X_ARM_MMU_SECTION(SL2312_DRAM_CTRL_BASE,  SL2312_DRAM_CTRL_BASE,   1,  
    				ARM_UNCACHEABLE, ARM_UNBUFFERABLE, ARM_ACCESS_PERM_RW_RW);
#endif
#ifdef SL2312_GENERAL_DMA_BASE
    X_ARM_MMU_SECTION(SL2312_GENERAL_DMA_BASE,  SL2312_GENERAL_DMA_BASE,   1,  
    				ARM_UNCACHEABLE, ARM_UNBUFFERABLE, ARM_ACCESS_PERM_RW_RW);
#endif
#ifdef SL2312_USB_BASE
    X_ARM_MMU_SECTION(SL2312_USB_BASE,  SL2312_USB_BASE,   1,  
    				ARM_UNCACHEABLE, ARM_UNBUFFERABLE, ARM_ACCESS_PERM_RW_RW);
#endif
#ifdef SL2312_USB0_BASE
    X_ARM_MMU_SECTION(SL2312_USB0_BASE,  SL2312_USB0_BASE,   1,  
    				ARM_UNCACHEABLE, ARM_UNBUFFERABLE, ARM_ACCESS_PERM_RW_RW);
#endif
#ifdef SL2312_USB1_BASE
    X_ARM_MMU_SECTION(SL2312_USB1_BASE,  SL2312_USB1_BASE,   1,  
    				ARM_UNCACHEABLE, ARM_UNBUFFERABLE, ARM_ACCESS_PERM_RW_RW);
#endif
#endif // 0
}

/*----------------------------------------------------------------------
* hal_get_mmu_status
*----------------------------------------------------------------------*/
int hal_get_mmu_status(void)
{
    int mmu;
	
	asm volatile (	
		"mrc p15,0,%0,c1,c0,0;"
        "nop;"
        "nop;"
        "nop;"
        :"=r"(mmu)	/* output */
        :					/* input */
        //:"r0"				/* affected registers */
	);
	
 	return (mmu & 0x0001) ? 1 : 0;
}

/*----------------------------------------------------------------------
* hal_get_icache_status
*----------------------------------------------------------------------*/
int hal_get_icache_status(void)
{
    int mmu;
	
	asm volatile (	
		"mrc p15,0,%0,c1,c0,0;"
        "nop;"
        "nop;"
        "nop;"
        :"=r"(mmu)	/* output */
        :					/* input */
        //:"r0"				/* affected registers */
	);
	
 	return (mmu & 0x1000) ? 1 : 0;
}

/*----------------------------------------------------------------------
* hal_get_dcache_status
*----------------------------------------------------------------------*/
int hal_get_dcache_status(void)
{
    int mmu;
	
	asm volatile (	
		"mrc p15,0,%0,c1,c0,0;"
        "nop;"
        "nop;"
        "nop;"
        :"=r"(mmu)	/* output */
        :					/* input */
        //:"r0"				/* affected registers */
	);
	
 	return (mmu & 0x0004) ? 1 : 0;
}


/*----------------------------------------------------------------------
* hal_set_icache
*	i/p: 1: enable, 0: disable
*----------------------------------------------------------------------*/
int hal_set_icache(int flag)
{
    int mmu;
	
	asm volatile (	
		"mrc p15,0,%0,c1,c0,0;"
        "nop;"
        "nop;"
        "nop;"
        :"=r"(mmu)	/* output */
        :					/* input */
        //:"r0"				/* affected registers */
	);
	
 	return (mmu & 0x1000) ? 1 : 0;
}


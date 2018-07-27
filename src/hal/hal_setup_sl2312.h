//****************************************************************************
// * Copyright  Storlink Corp 2002-2003.  All rights reserved.                
// *--------------------------------------------------------------------------
// * Name			: hal_setup_sl2312.c
// * Description	: 
// *		Setup SL2312 platform
// *
// * History
// *
// *	Date		Writer		Description
// *	-----------	-----------	-------------------------------------------------
// *	04/18/2005	Gary Chen	Porting from Jason's redboot code
// *
// ****************************************************************************
#ifndef _HAL_SETUP_SL2312
#define _HAL_SETUP_SL2312

//#define ICE_LOAD						1
#define DEBUG                           1
#define DDR                             1
#define DDR_DELAY						0x03030303
#define IO_DRIVEN						0x000000ff
#define IO_SLEW							0x00000000
#define VER_ID							0x00351600
#define VER_MASK						0x00FFFF00
#define BOARD_MASK					0x000000FF

#define NFLASH_TYPE 		0x0000000c
#define NFLASH_CTRL 		0x65000000
#define page_1       	   	0x30000200 
#define page_4	     	   	0x30000800 
#define page_8	     	   	0x30001000 
#define reg_glo	     	   	0x30001000


        .macro  HAL_SETUP_SL2312

#ifdef _HAL_INIT_PLATFORM_
#ifndef ICE_LOAD        
#ifdef MIDWAY

	   ldr    r0,=SL2312_FLASH_CTRL_BASE
       add    r0,r0,#0x0c 
       ldr    r1,[r0]
       and    r2,r1,#0x1800  
       cmp    r2,#0x0000        
       bne    nserial
	   ldr    r0,=SL2312_GLOBAL_BASE + GLOBAL_MISC_CTRL         //DRAM mode control
        ldr    r5,[r0]  
        mov    r4,#0x6
        orr    r5,r5,r4                                 //Enable train mode
        str    r5,[r0]
nserial:	   
       ldr    r0,=NFLASH_CTRL
       add    r0,r0,#0x0c  
       ldr    r1,[r0]
       and    r2,r1,#0x1800  
       cmp    r2,#0x1800        
       beq    n2kp
       cmp    r2,#0x1000
       bne    P_FLASH1
       
       
       ldr    r0,=SL2312_FLASH_CTRL_BASE
       add    r0,r0,#0x30  //nand flash access 
       ldr    r1,[r0]
       orr    r1,r1,#0x4400  //enable direct access
       str    r1,[r0]
       
       ldr    r0,=SL2312_FLASH_BASE
       //ldr	  r5,=page_1
       add    r5,r0,#0x200
       add    r6,r0,#0x1000
       //ldr	  r6,=page_8
       mov	   r4,#0x200	  // ;sram start address

n512p:

	   mov	   r1,#0x10
delay1:	   
	   nop
	   subs	   r1, r1, #1
	   bne     delay1
	   
   	   mov	   r1,#0x210	//

n512_p:
	   ldr    r7,[r5],#4	//;start of rom
       str    r7,[r4],#4	//;ram
       subs	   r1, r1, #4		//		    ; loop
       bne	   n512_p
       
       sub     r5,r5,#0x10
       sub     r4,r4,#0x10
	   cmp     r5,r6		//;end address of data
       blo     n512p
       b 	   P_FLASH

n2kp:
	   ldr    r0,=SL2312_FLASH_CTRL_BASE
       add    r0,r0,#0x30  //nand flash access 
       ldr    r1,[r0]
       orr    r1,r1,#0x4400  //enable direct access
       str    r1,[r0]
//adr     r5,page_1  //;nand flash start address
       ldr	  r5,=page_4
       //adr     r6,page_4  //;nand flash start address
       ldr	  r6,=page_8
       mov	   r4,#0x800	  // ;sram start address

n2kp1:
   	   mov	   r1,#0x840	//;add a page code data
n2kp_p:
	   ldr    r7,[r5],#4	//;start of rom
       str    r7,[r4],#4	//;ram
       subs	   r1, r1, #4		//		    ; loop
       bne	   n2kp_p
       
       sub     r5,r5,#0x40
       sub     r4,r4,#0x40
	   cmp     r5,r6		//;end address of data
       blo     n2kp1
       
       b 	   P_FLASH

P_FLASH1:
#if defined(GEMINI_ASIC) || defined(LEPUS_ASIC)

			//move more code
			ldr    r0,=SL2312_FLASH_CTRL_BASE
       		add    r0,r0,#0x20  //nor flash access 
       		ldr    r1,[r0]
       		orr    r1,r1,#0x4200  //enable direct access
       		str    r1,[r0]
			
       		//ldr	  r5,=page_4
       		ldr	  r5,=SL2312_FLASH_BASE  //page_4  //0x30000800
       		add    r5,r5,#0x800
       		
       		//ldr	  r6,=page_8
       		mov	  r6,#0x00  //page_8  //0x30001000
       		add    r6,r6,#0x1000
       		
      		mov	   r1,#0x800
more_p:
	   		ldrh    r7,[r5],#2	//;start of rom
       		strh    r7,[r6],#2	//;ram
       		subs	   r1, r1, #2		//		    ; loop
       		bne	   more_p      	
P_FLASH:
				// check board version, if 3512 then use 16bit dram init
			 ldr	r3, =SL2312_GLOBAL_BASE+0x00   //revsion id
	 		 ldr	r4, [r3]			
	 		 ldr 	r5, =VER_MASK
	 		 and  r4,r4,r5
	 		 ldr 	r5, =VER_ID
	 		 cmp  r4,r5
			 bne  DRAM_16
    	 
       ldr    r0,=SL2312_DRAM_CTRL_BASE + DRAM_SDTYPE      //DRAM type register         
       ldr    r1,=SDRAM_SDTYPE_128M                     //Set to 128M  
       str    r1,[r0]

       ldr    r0,=SL2312_DRAM_CTRL_BASE + DRAM_SDRMR       //DRAM mode control   
       ldr	  r1,=SDRMR_DISABLE_DLL      
       str    r1,[r0]
       b			DRAM_ok
       
DRAM_16:
			 ldr    r0,=SL2312_DRAM_CTRL_BASE + DRAM_SDTYPE      //DRAM type register         
       ldr    r1,=SDRAM_16_SDTYPE_64M                     //Set to 128M  
       str    r1,[r0]

       ldr    r0,=SL2312_DRAM_CTRL_BASE + DRAM_SDRMR       //DRAM mode control   
       ldr	  r1,=SDRMR_16_DISABLE_DLL      
       str    r1,[r0]

DRAM_ok:       
			 
			 ldr	r3, =SL2312_GLOBAL_BASE+0x00   //revsion id
	 		 ldr	r4, [r3]			
	 		 ldr 	r5, =BOARD_MASK
	 		 and  r4,r4,r5  
	 		 cmp  r4,#0xc0
			 bne  END_PREFETCH
			 
			//disable prefetch for a0 board
        ldr    r0,=SL2312_DRAM_CTRL_BASE + DRAM_AHBCR         //DRAM mode control
        ldr    r5,=MCR_PREFETCH_DISABLE                       //Enable train mode
        str    r5,[r0]
               //end disable prefetch 
END_PREFETCH:
#endif               
#if defined(GEMINI_ASIC) || defined(LEPUS_ASIC)
		
			 	//dram init       
    	 ldr	r3, =SL2312_GLOBAL_BASE+0x08
	 		 ldr	r4, [r3]			
	 		 orr    r4,r4,#0x00007000 
    	 str    r4, [r3]
    	  	
    	 ldr	r3, =SL2312_GLOBAL_BASE+0x10
	 		 ldr	r4, [r3]			
	 		 orr    r4,r4,#0x00000f00
	 		 orr    r4,r4,#0x000000ff
    	 str    r4, [r3]	
			
	   ldr    r0,=SL2312_DRAM_CTRL_BASE + DRAM_SDRTR       //set DDR timing parameter 
       ldr	  r1,=SDRMR_TIMING      
       str    r1,[r0]
       
       ldr    r0,=SL2312_DRAM_CTRL_BASE + DRAM_RD_DLL       //set DDR DLL delay
       ldr	  r1,=SDRMR_RD_DLLDLY      
       str    r1,[r0]
       
       ldr    r0,=SL2312_DRAM_CTRL_BASE + DRAM_WR_DLL       //set DDR timing parameter 
       ldr	  r1,=SDRMR_WR_DLLDLY      
       str    r1,[r0]
       
       ldr    r0,=SL2312_DRAM_CTRL_BASE + DRAM_MCR         //start training, data capture timing
       ldr    r1,=MCR_DLLTM_ENABLE                              //Enable train mode
       str    r1,[r0]
       
       
#else

	   ldr    r0,=SL2312_DRAM_CTRL_BASE + DRAM_SDTYPE      //DRAM type register         
       ldr    r1,=SDRAM_SDTYPE_128M                     //Set to 128M  
       str    r1,[r0]

       ldr    r0,=SL2312_DRAM_CTRL_BASE + DRAM_SDRMR       //DRAM mode control   
       ldr	  r1,=SDRMR_DISABLE_DLL      
       str    r1,[r0]
	   ldr    r0,=SL2312_DRAM_CTRL_BASE + DRAM_MCR         //DRAM mode control
       ldr    r1,[r0]
       ldr    r5,=MCR_DLLTM_ENABLE
       orr    r1,r1,r5                                     //Enable train mode
       str    r1,[r0]

#endif
// temporary disable for fpga test		 
        
        ldr    r2,=SL2312_RAM_BASE
        mov    r4,#0xa0
1:    
        ldr    r3,[r2]                                      //Read data          
        subs   r4, r4, #1	                             //Decrement loop count
        bge    1b

        ldr    r5,=MCR_DLLTM_MASK         
        and    r1,r1,r5                                     //Disable train mode
        str    r1,[r0]
        
#ifdef LPC_IT8712
		ldr    r0,=SL2312_GLOBAL_BASE + GLOBAL_MISC_CTRL         //DRAM mode control
        ldr    r5,[r0]  
        ldr    r4,=LPC_CLK_ENABLE
        orr    r5,r5,r4                                 //Enable train mode
        str    r5,[r0]
#endif   //LPC_IT8712 	
	 	
	   ldr    r5,=SL2312_ROM_BASE   // Relocate FLASH/ROM to SDRAM
       ldr    r4,=SL2312_RAM_BASE   // ram base & length
	   ldr    r6,=__ram_data_end
	   orr	  r6,r6,#0x30000000
	   
	   ldr    r0,=NFLASH_CTRL
       add    r0,r0,#0x0c     
       ldr    r1,[r0]
       and    r2,r1,#0x1800   //;#0x3500
       cmp    r2,#0x1800   //;#0x3500        
       beq    n2k
       and    r2,r1,#0x1000  //;#0x2200
       cmp    r2,#0x1000     //;#0x2200    
       beq    n512
       bne	  flash_page
       
       
n2k:   
   	   mov	   r1,#0x840
n2k_page:
	   ldr    r7,[r5],#4	//;start of rom
       str    r7,[r4],#4	//;ram
       subs	   r1, r1, #4				   // ; delay loop
       bne	   n2k_page
       
       sub     r5,r5,#0x40
       sub     r4,r4,#0x40

	   cmp     r5,r6		//;end address of data
       blo     n2k
       
       b	   wr_ok


n512:

	   mov	   r1,#0x10
delay2:	   
	   nop
	   subs	   r1, r1, #1
	   bne     delay2
	  
   	   mov	   r1,#0x210
   	   
n512_page:
	   ldr    r7,[r5],#4	//;start of rom
       str    r7,[r4],#4	//;ram
       subs	   r1, r1, #4				    //; delay loop
       bne	   n512_page
       
       sub     r5,r5,#0x10
       sub     r4,r4,#0x10

	   cmp     r5,r6		//;end address of data
       blo     n512
       
       b	   wr_ok
       
       
flash_page:
// enable flash continuous mode
		
	   ldr    r0,=NFLASH_CTRL
       add    r0,r0,#0x0c  
       ldr    r1,[r0]
       and    r2,r1,#0x1800  
       cmp    r2,#0x0000        
       bne    nserial1
       ldr    r0,=SL2312_FLASH_CTRL_BASE+0x10 //FLASH_ACCESS_OFFSET
       bl	  nserial2
nserial1:		
        ldr    r0,=SL2312_FLASH_CTRL_BASE+0x20 //FLASH_ACCESS_OFFSET
nserial2:        
        ldr    r1,[r0]
        ldr    r5,=ACCESS_CONTINUE_MODE
        orr    r1,r1,r5
        str    r1,[r0]         

//  copy ROM code to SDRAM(current is at 0x10000000)
        ldr     r0,=SL2312_ROM_BASE   // Relocate FLASH/ROM to SDRAM
        ldr     r1,=SL2312_RAM_BASE   // ram base & length
        ldr     r2,=__ram_data_end
        add		r2,r2,#0x30000000
        
3:      ldr     r3,[r0],#4
        str     r3,[r1],#4
        cmp     r0,r2
        blo     3b
        ldr     r0,=4f

wr_ok:
        nop
	 	nop
4:	 	nop

// this code will remap the memory ROM and DRAM .ROM will place on 0x30000000 
//      DRAM will jump to 0x0       
    	
    	ldr	r3, =SL2312_GLOBAL_BASE		
	 	ldr	r4, [r3,#GLOBAL_MISC_CTRL]
	 	orr	r4, r4, #GLOBAL_REMAP_BIT 	 	// Set REMAP bit
	 	//orr	r4, r4, #0x02000000			// Set IOMUX mode 2
	 	orr	r4, r4, #0x03000000			// Set IOMUX mode 3
	 	and	r4, r4, #0xFFFFFFFF
     	str    r4, [r3,#GLOBAL_MISC_CTRL]

//        ldr r3,=SL2312_DRAM_CTRL_BASE + 0x1c       //Disable Prefetch function
//		mov	r4,#0x00000002      
//		str r4,[r3]
		
// disable flash continuous mode
       ldr    r0,=NFLASH_CTRL
       add    r0,r0,#0x0c  
       ldr    r1,[r0]
       and    r2,r1,#0x1800  
       cmp    r2,#0x0000        
       bne    nserial3
       ldr    r0,=SL2312_FLASH_CTRL_BASE+0x10 //FLASH_ACCESS_OFFSET
       bl	  nserial4
nserial3:		
        ldr    r0,=SL2312_FLASH_CTRL_BASE+0x20 //FLASH_ACCESS_OFFSET
nserial4:  
        ldr    r1,[r0]
        ldr    r5, =ACCESS_CONTINUE_DISABLE
        and    r1,r1,r5
        str    r1,[r0]         

#else	// ifndef MIDWAY

#ifdef DDR
        ldr    r0,=SL2312_DRAM_CTRL_BASE + DRAM_SDRMR       //DRAM mode control         
        ldr    r1,[r0]
        ldr    r5,=SDRMR_DISABLE_DLL                        //Disable DLL  
        orr    r1,r1,r5
        str    r1,[r0]
        
        ldr    r0,=SL2312_DRAM_CTRL_BASE + DRAM_SDRTR      //DRAM timing register         
        ldr    r1,=SDRAM_SDRTR_DEFAULT                     
        str    r1,[r0]
        
        ldr    r0,=SL2312_DRAM_CTRL_BASE + 0x14      		//DRAM DLL      
        ldr    r1,=DDR_DELAY                     
        str    r1,[r0]
        
#else //SDR
        ldr    r0,=SL2312_DRAM_CTRL_BASE + DRAM_SDTYPE      //DRAM type register         
        ldr    r1,=SDRAM_SDTYPE_DEFAULT                     //Set to SDRAM  
        str    r1,[r0]

        ldr    r0,=SL2312_DRAM_CTRL_BASE + DRAM_SDRMR      //DRAM type register         
        ldr    r1,=SDRAM_SDRMR_DEFAULT                    //Set Mode contorl for SDRAM  
        str    r1,[r0]

        ldr    r0,=SL2312_DRAM_CTRL_BASE + DRAM_SDRTR      //DRAM type register         
        ldr    r1,=SDRAM_SDRTR_DEFAULT                    //Set Timing contorl for SDRAM  
        str    r1,[r0]
#endif     
//         ldr    r0,=SL2312_DRAM_CTRL_BASE + DRAM_SDTYPE      //DRAM type register         
//         ldr    r1,=SDRAM_SDTYPE_128M                        //Set to 128M  
//         str    r1,[r0]
	 
        ldr    r0,=SL2312_DRAM_CTRL_BASE + DRAM_MCR         //DRAM mode control
        ldr    r1,[r0]
        ldr    r5,=MCR_DLLTM_ENABLE
        orr    r1,r1,r5                                     //Enable train mode
        str    r1,[r0]
        ldr    r2,=SL2312_RAM_BASE
        mov    r4,#0x20
1:    
        ldr    r3,[r2]                                      //Read data          
        subs   r4, r4, #1	                             //Decrement loop count
        bge    1b
	
		
	 	mov	r4,#0x00000200
2:
        mov    r5, r5
	 	subs	r4, r4, #1				    // delay loop
	 	bge	2b
	 
        ldr    r5,=MCR_DLLTM_MASK         
        and    r1,r1,r5                                     //Disable train mode
        str    r1,[r0]
         
        ldr		r3, =SL2312_GLOBAL_BASE		
	 	ldr		r4, =DRAM_SLEW_DEFAULT	 		// Set IDE slew rate SLOW
	 	ldr		r5, =IO_SLEW
	 	orr		r4,r4,r5
        str		r4, [r3,#GLOBAL_SLEW_RATE_CTRL]
         
        ldr		r3, =SL2312_GLOBAL_BASE		
	 	ldr		r4, =DRAM_DRIVE_DEFAULT			// Set Driving capability
	 	ldr 	r5, =IO_DRIVEN
	 	orr		r4,r4,r5
        str		r4, [r3,#GLOBAL_DRIVE_CTRL]


#if 0	 	
// enable flash continuous mode
        ldr    r0,=SL2312_FLASH_CTRL_BASE+FLASH_ACCESS_OFFSET
        ldr    r1,[r0]
        ldr    r5,=ACCESS_CONTINUE_MODE
        orr    r1,r1,r5
        str    r1,[r0]         
//  copy ROM code to SDRAM(current is at 0x10000000)
        ldr     r0,=SL2312_ROM_BASE   // Relocate FLASH/ROM to SDRAM
        ldr     r1,=SL2312_RAM_BASE   // ram base & length
        ldr     r2,=__ram_data_end
3:      ldr     r3,[r0],#4
        str     r3,[r1],#4
        cmp     r0,r2
        blo     3b
        ldr     r0,=4f
        nop
        mov     pc,r0
	 	nop
4:	 	nop

// disable flash continuous mode
        ldr    r0,=SL2312_FLASH_CTRL_BASE+FLASH_ACCESS_OFFSET
        ldr    r1,[r0]
        ldr    r5, =ACCESS_CONTINUE_DISABLE
        and    r1,r1,r5
        str    r1,[r0]         
#endif

//	
// this code will remap the memory ROM and SDRAM .ROM will place on 0x70000000 
//      SDRAM will jump to 0x0       
    	
// a8:		b	a8
	    ldr     lr,=remap
	    
    	ldr	r3, =SL2312_GLOBAL_BASE		
	 	ldr	r4, [r3,#GLOBAL_MISC_CTRL]
	 	orr	r4, r4, #GLOBAL_REMAP_BIT 	 	// Set REMAP bit
	 	orr	r4, r4, #0x00000018			// Enable LPC, FLASH
//	 	orr	r4, r4, #0x00000038			// Enable External LPC clock, FLASH
	 	and	r4, r4, #0xFFFFFFFF
     	str    r4, [r3,#GLOBAL_MISC_CTRL]

        mov     pc,lr;
remap:        
	 	mov	r4,#0x0002000
5:
     	mov    r5, r5
	 	subs	r4, r4, #1				    // delay loop
	 	bge	5b

// a9:		b	a9

	 	ldr	r3, =SL2312_GLOBAL_BASE		
	 	ldr	r4, [r3,#GLOBAL_RESET]
	 	orr	r4, r4, #0x00008000	 		// Set Watch Dog reset bit
        str    r4, [r3,#GLOBAL_RESET]
	 
	 	ldr	r3, =SL2312_GLOBAL_BASE+GLOBAL_ARBI_CTRL
	 	ldr	r4, =GLOBAL_ARBI_V			// Arbitration control IDE,PCI,EMAC high priority
     	str    r4, [r3]

#endif  //MIDWAY
#if	defined(GEMINI_ASIC) || defined(LEPUS_ASIC)
		ldr	r3, =SL2312_GPIO_BASE+0x08
	 	ldr	r4, =SL2312_UART_DIR			// Arbitration control IDE,PCI,EMAC high priority
     	str    r4, [r3]
		ldr	r3, =SL2312_GPIO_BASE+0x0C
	 	ldr	r4, =SL2312_UART_PIN			// Arbitration control IDE,PCI,EMAC high priority
     	str    r4, [r3]
    
     	
#else //GEMINI_ASIC || LEPUS_ASIC
	 	ldr	r3, =SL2312_GPIO_BASE+0x0C
	 	ldr	r4, =SL2312_UART_PIN			// Arbitration control IDE,PCI,EMAC high priority
     	str    r4, [r3]
#endif  //GEMINI_ASIC || LEPUS_ASIC
#endif //ICE_LOAD
#endif // _HAL_INIT_PLATFORM_
      	.endm

#endif // _HAL_SETUP_SL2312

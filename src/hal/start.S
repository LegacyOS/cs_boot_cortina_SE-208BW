;/****************************************************************************
; * Copyright  Storlink Corp 2005.  All rights reserved.                
; *--------------------------------------------------------------------------
; * Name		: sys_start.S
; * Description	: 
; *		Start-up entry for BOOT-2 (stage 2 of the boot loader)
; *
; * History
; *
; *	Date		Writer		Description
; *	-----------	-----------	-------------------------------------------------
; *	04/18/2005	Gary Chen	Create
; *
; ****************************************************************************/

#include <board_config.h>
#include <sl2312.h>
#include <hal_setup_sl2312.h>

        .file   "sys_start.S"

// #========================================================================
// #	Macro definition
// #========================================================================
#ifdef __thumb__
// Switch to thumb mode
#define THUMB_MODE(_r_, _l_)                     \
        ldr     _r_,=_l_ ## f+1                 ;\
        bx      _r_                             ;\
        .pool                                   ;\
        .code   16                              ;\
        .thumb_func                             ;\
 _l_:

// Call thumb function from ARM mode, return to ARM
// mode afterwards
#define THUMB_CALL(_r_, _l_, _f_)                \
        ldr     _r_,=_f_+1                      ;\
        mov     lr,pc                           ;\
        bx      _r_                             ;\
        .pool                                   ;\
        .code   16                              ;\
        .thumb_func                             ;\
        ldr     _r_,=_l_  ## f                  ;\
        bx      _r_                             ;\
        .pool                                   ;\
        .code   32                              ;\
 _l_:

// Switch to ARM mode
#define ARM_MODE(_r_, _l_)                       \
        ldr     _r_,=_l_ ## f                   ;\
        bx      _r_                             ;\
        .pool                                   ;\
        .code   32                              ;\
 _l_:

// Function definition, start executing body in ARM mode
#define FUNC_START_ARM(_name_, _r_)              \
        .code   16                              ;\
        .thumb_func                             ;\
        .globl _name_                           ;\
_name_:                                         ;\
        ldr     _r_,=_name_ ## _ARM             ;\
        bx      _r_                             ;\
        .code   32                              ;\
_name_ ## _ARM:

#else

// Switch to thumb mode
#define THUMB_MODE(_r_, _l_)

// Call ARM function
#define THUMB_CALL(_r_, _l_, _f_) \
        bl      _f_

// Switch to ARM mode
#define ARM_MODE(_r_, _l_)

// Function definition, start executing body in ARM mode
#define FUNC_START_ARM(_name_, _r_) \
        .globl _name_; \
_name_: 

#endif

#define DEFINE_PTR(name)			\
.##name: .word  name

#define WRITE_REG32(reg, value)		\
		ldr	    r0,=reg;			\
		ldr		r1,=value;			\
		str		r1,[r0]

// #========================================================================
// #	Vectors
// #========================================================================
        .code   32
        .section ".init","ax"

        .global __exception_handlers
__exception_handlers:
        b       reset_vector					// 0x00 
		ldr     pc,.undefined_instruction       // 0x04
		ldr     pc,.software_interrupt          // 0x08 start && software int
		ldr     pc,.abort_prefetch              // 0x0C
        ldr     pc,.abort_data                  // 0x10
		b       reset_vector                    // unused
		ldr     pc,.IRQ                         // 0x18
        ldr     pc,.FIQ                         // 0x1C
        
        .global vectors
vectors:
DEFINE_PTR(reset_vector);					/* 0x20 */
DEFINE_PTR(undefined_instruction);			/* 0x24 */
DEFINE_PTR(software_interrupt);				/* 0x28 */
DEFINE_PTR(abort_prefetch);					/* 0x2C */
DEFINE_PTR(abort_data);						/* 0x30 */
        .word   0							/* 0x34 */
DEFINE_PTR(IRQ);							/* 0x38 */
DEFINE_PTR(FIQ);							/* 0x3c */
 
/* Offset 0x40, the starting location of User's RAM Area */
                       
// #========================================================================
// #	Start-up Program
// #	reset_vector
// #========================================================================
        .text   
        .global reset_vector
        .type   reset_vector,function 
reset_vector:
// #========================================================================
// #	Set to supervisor mode and disable interrupts
// #    initialize CPSR (machine state register)
// #========================================================================
        mov     r0,#(ARM_CPSR_IRQ_DISABLE|ARM_CPSR_FIQ_DISABLE|ARM_CPSR_SUPERVISOR_MODE)
        msr     cpsr,r0

		/* Disable all interrupts in the interrupt controller */
		WRITE_REG32 (SL2312_INTERRUPT_BASE + SL2312_IRQ_MASK, 0);

		// setup SL2312 platform
		HAL_SETUP_SL2312
		
// #========================================================================
// #	warm_reset
// #========================================================================
		//  set interrupt vectors  
warm_reset:
        mov     r0,#0
        ldr     r1,=__exception_handlers
        // Wait with this if stubs are included (see further down).
        ldr     r2,[r1,#0x04]   // undefined instruction
        str     r2,[r0,#0x04]
        ldr     r2,[r1,#0x24]   
        str     r2,[r0,#0x24]
        ldr     r2,[r1,#0x08]   // software interrupt
        str     r2,[r0,#0x08]
        ldr     r2,[r1,#0x18]   //  IRQ
        str     r2,[r0,#0x18]
        ldr     r2,[r1,#0x38]
        str     r2,[r0,#0x38]
        ldr     r2,[r1,#0x1C]   /* FIQ */
        str     r2,[r0,#0x1C]
        ldr     r2,[r1,#0x3C]
        str     r2,[r0,#0x3C]
        ldr     r2,[r1,#0x0C]   /* abort (prefetch) */
        str     r2,[r0,#0x0C]
        ldr     r2,[r1,#0x2C]   
        str     r2,[r0,#0x2C]
        ldr     r2,[r1,#0x10]   /* abort (data) */
        str     r2,[r0,#0x10]
        ldr     r2,[r1,#0x30]
        str     r2,[r0,#0x30]


// #========================================================================
// #	Set up reset vector
// #========================================================================
        mov     r0,#0
        ldr     r1,=__exception_handlers
        ldr     r2,[r1,#0x00]    /*  reset vector intstruction */
        str     r2,[r0,#0x00]
        ldr     r2,=warm_reset
        str     r2,[r0,#0x20]
       
// #========================================================================
// #	 Copy from ROM Data to RAM area
// #========================================================================
// t1:		b		t1
        ldr     r3,.__rom_data_start
        ldr     r4,.__ram_data_start
        ldr     r5,.__ram_data_end
        cmp     r4,r5           /*  jump if no data to move */
        beq     2f
        sub     r3,r3,#4        /*  loop adjustments */
        sub     r4,r4,#4
1:      ldr     r0,[r3,#4]!     /*  copy info */
        str     r0,[r4,#4]!
        cmp     r4,r5
        bne     1b
2:


// #========================================================================
// #	 initialize interrupt/exception environments
// #========================================================================
        ldr     sp,.__startup_stack
        mov     r0,#(ARM_CPSR_IRQ_DISABLE|ARM_CPSR_FIQ_DISABLE|ARM_CPSR_IRQ_MODE)
        msr     cpsr,r0
        ldr     sp,.__exception_stack
        mov     r0,#(ARM_CPSR_IRQ_DISABLE|ARM_CPSR_FIQ_DISABLE|ARM_CPSR_UNDEF_MODE)
        msr     cpsr,r0
        ldr     sp,.__exception_stack

        /*  initialize CPSR (machine state register) */
        mov     r0,#(ARM_CPSR_IRQ_DISABLE|ARM_CPSR_FIQ_DISABLE|ARM_CPSR_SUPERVISOR_MODE)
        msr     cpsr,r0

        /*  Note: some functions in LIBGCC1 will cause a "restore from SPSR"!! */
        msr     spsr,r0

// #========================================================================
// #	 initialize stack
// #========================================================================
        ldr     sp,.__startup_stack

// #========================================================================
// #	 clear BSS
// #========================================================================
        ldr     r1,.__bss_start
        ldr     r2,.__bss_end
        mov     r0,#0
        cmp     r1,r2
        beq     2f
1:      str     r0,[r1],#4
        cmp     r1,r2
        bne     1b
2:

// #========================================================================
// #	Call main entry of C program
// #========================================================================
        bl      sl_main

_here:
        b       _here

// #========================================================================
// #	Function: 
// #========================================================================
        .global reset_platform
        .type   reset_platform,function
reset_platform:
        mov     r0,#0
        mov     pc,r0           /* Jump to reset vector  */      


// #========================================================================
// #	entry of exception
// #========================================================================
        .code   32
        .global undefined_instruction
        .global software_interrupt, abort_prefetch
        .global abort_data, FIQ
undefined_instruction:
software_interrupt:
abort_prefetch:
abort_data:
FIQ:
		b		hal_reset
		b		reset_vector

/*
 *************************************************************************
 *
 * Interrupt handling
 *
 *************************************************************************
 */

@
@ IRQ stack frame.
@
#define S_FRAME_SIZE	72

#define S_OLD_R0	68
#define S_PSR		64
#define S_PC		60
#define S_LR		56
#define S_SP		52

#define S_IP		48
#define S_FP		44
#define S_R10		40
#define S_R9		36
#define S_R8		32
#define S_R7		28
#define S_R6		24
#define S_R5		20
#define S_R4		16
#define S_R3		12
#define S_R2		8
#define S_R1		4
#define S_R0		0

		.macro	irq_save_user_regs
		sub		sp, sp, #S_FRAME_SIZE
		stmia	sp, {r0 - r12}			// Calling r0-r12
		nop
		add		r8, sp, #S_PC
		stmdb	r8, {sp, lr}^			// Calling SP, LR
		nop
		str		lr, [r8, #0]			// Save calling PC
		mrs		r6, spsr
		str		r6, [r8, #4]			// Save CPSR
		str		r0, [r8, #8]			// Save OLD_R0
		mov		r0, sp
		.endm

		.macro	irq_restore_user_regs
		ldmia	sp, {r0 - lr}^			// Calling r0 - lr
		nop
		nop
		mov		r0, r0
		ldr		lr, [sp, #S_PC]			// Get PC
		add		sp, sp, #S_FRAME_SIZE
		subs	pc, lr, #4				// return & move spsr_svc into cpsr
		.endm

		.macro get_irq_stack			// setup IRQ stack
		ldr     sp,.__exception_stack
		.endm

        .code   32
		.global	IRQ
		.align	5
IRQ:
		get_irq_stack
		irq_save_user_regs
		bl 	hal_irq_handler
		irq_restore_user_regs
	


// #========================================================================
// #	Pointers to various objects.
// #========================================================================
DEFINE_PTR(__startup_stack)
DEFINE_PTR(__exception_stack)
DEFINE_PTR(__bss_start)
DEFINE_PTR(__bss_end)
DEFINE_PTR(_end)
DEFINE_PTR(__rom_data_start)
DEFINE_PTR(__ram_data_start)
DEFINE_PTR(__ram_data_end)
DEFINE_PTR(__exception_handlers)
        .data

// #========================================================================
// #	Data area
// #========================================================================
        .data

boot_active:
        .balign 4
        .byte	'P'
        .byte	'O'
        .byte	'S'
        .byte	'T'

// #========================================================================
// #	BSS area
// # 		- Temporary interrupt stack
// #========================================================================
        .section ".bss"

        .balign 4
        .byte	'.'
        .byte	'B'
        .byte	'S'
        .byte	'S'

		.align 14
		.global ttb_base
ttb_base:
        .rept   (0x4000)
        .long   0
        .endr
        
        .balign 16
__exception_stack_base: 
        .rept   256
        .long   0
        .endr
__exception_stack:

        .balign 16
__startup_stack_base:
        .rept 4096
        .byte 0
        .endr
        .balign 16
__startup_stack:


// #========================================================================
// #	end
// #========================================================================

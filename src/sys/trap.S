#include <board_config.h>
//		.section	"__ex_table", "ax"
		.code 32
	.global _exception_handlers
_exception_handlers:
	b	.reset_vector		     // 0x00
	b	.undefined_instruction       // 0x04
        b     	.software_interrupt          // 0x08 start && software int
        b	.abort_prefetch              // 0x0C
        b	.abort_data                  // 0x10
        .word   0                            // unused
        b	.IRQ                         // 0x18
        b	.FIQ                         // 0x1C

.reset_vector:
//	read cpuid
//	if(cpu1)
	adr	r0, _slave 
	ldr	pc, [r0] 
.undefined_instruction:
	movs 	pc, r14 
.software_interrupt:
	stmfd r13!, {r1-r3, r14} 
	adr	r0, _soft_int	 
	mov	r14, pc	
	ldr	pc, [r0] 
	ldmfd r13!, {r1-r3, r15}^ 
.abort_prefetch:
	subs 	pc, r14, #8 			// #4 or #8 ??
.abort_data:
	subs 	pc, r14, #8
.IRQ:
	stmfd r13!, {r1-r3, r14} 
	adr	r0, _irq 
	mov	r14, pc	
	ldr	pc, [r0] 
	ldmfd r13!, {r1-r3, r15}^ 
	
.FIQ:
	stmfd r13!, {r1-r3, r14} 
	adr	r0, _fiq 
	mov	r14, pc	
	ldr	pc, [r0] 
	ldmfd r13!, {r1-r3, r15}^ 

_slave: 	.word	CPU2_BOOT_OFFSET	
_soft_int:	.word	0x0
_irq:		.word	0
_fiq:		.word	0
_error:		.word	0

	.global _exception_handlers_end
_exception_handlers_end:
	nop


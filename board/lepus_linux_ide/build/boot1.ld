
STARTUP(boot1_start.o)
GROUP(libboot.a libhal.a)
ENTRY(reset_vector)

MEMORY
{
    ram : ORIGIN = 0, LENGTH = 0x400000
    rom : ORIGIN = 0x00000000, LENGTH = 0x800000
}

SECTIONS
{
    .rom_vectors	0x00000000	:	{ . = . ; KEEP (*(.init)) } >  rom
    .text			ALIGN (1)	:	{ *(.text*) *(.init) *(.glue_7) *(.glue_7t) } >  rom
    .fini			ALIGN (4)	:	{ . = . ; *(.fini) }   >  rom
    .rodata			ALIGN (4)	:	{ . = . ; *(.rodata*) }   >  rom
    .fixup			ALIGN (4)	:	{ . = . ; *(.fixup) }   >  rom
    .data			0x2000 		:	{ __ram_data_start = ABSOLUTE (.); *(.data*)  . = ALIGN (4); } >  ram  __rom_data_start = LOADADDR (.data); __ram_data_end = .; PROVIDE (__ram_data_end = .); _edata = .; PROVIDE (edata = .); PROVIDE (__rom_data_end = LOADADDR (.data) + SIZEOF(.data)); 
    .bss			ALIGN (4)	:	{ __bss_start = ABSOLUTE (.);  *(.bss*) . = ALIGN (4); *(COMMON) __bss_end = ABSOLUTE (.); }   >  ram
     __heap1	=	ALIGN (32);
    . = ALIGN(4); _end = .; PROVIDE (end = .); 
}



STARTUP(boot2_start.o)
GROUP(./../lib/libsys.a ./../lib/libastel.a ./../lib/libhal.a ./../lib/libbsp.a ./../obj/usb/libusb.a ./../lib/libraid.a ./../lib/libide.a ./../lib/libcfg.a ./../lib/libnet.a ./../lib/libui.a ./../lib/libXmodem.a libgcc.a) 
ENTRY(reset_vector)

MEMORY
{
    ram : ORIGIN = 0, LENGTH = 0x800000
    rom : ORIGIN = 0x7E00000, LENGTH = 0x200000
}

SECTIONS
{
    .debug_aranges	0 			:	{ *(.debug_aranges) } .debug_pubnames 0 : { *(.debug_pubnames) } .debug_info     0 : { *(.debug_info) } .debug_abbrev   0 : { *(.debug_abbrev) } .debug_line     0 : { *(.debug_line) } .debug_frame    0 : { *(.debug_frame) } .debug_str      0 : { *(.debug_str) } .debug_loc      0 : { *(.debug_loc) } .debug_macinfo  0 : { *(.debug_macinfo) } 
    .rom_vectors	0x7E00000	:	{ . = . ; KEEP (*(.init)) }
    .text			ALIGN (1)	:	{ *(.text*) *(.init) *(.glue_7) *(.glue_7t) }
    .rodata			ALIGN (4)	:	{ . = . ; *(.rodata*) }   
    .data			ALIGN (4)	:	{ __ram_data_start = ABSOLUTE (.); *(.data*)  . = ALIGN (4); }   __rom_data_start = LOADADDR (.data); __ram_data_end = .; PROVIDE (__ram_data_end = .); PROVIDE (__rom_data_end = LOADADDR (.data) + SIZEOF(.data)); 
    .got			ALIGN (4)	:	{ . = . ; *(.got) }  
    .got.plt		ALIGN (4)	:	{ . = . ; *(.got.plt) }  
    .bss			ALIGN (4)	:	{ __bss_start = ABSOLUTE (.);  *(.bss*) . = ALIGN (4); *(COMMON) __bss_end = ABSOLUTE (.); }
     __heap1	=	ALIGN (32);
    . = ALIGN(4); _end = .; PROVIDE (end = .); 
}

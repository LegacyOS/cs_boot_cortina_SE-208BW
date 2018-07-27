#-----------------------------------------------------------------------------
MODULE 		:= 	sys
#-----------------------------------------------------------------------------

include $(PACKAGE_ENV)/environment.mak
#CFLAGS 			:= $(BASIC_CFLAGS) -DNO_ERRNO_H
CFLAGS 				:= $(BASIC_CFLAGS) -DNO_ERRNO_H
LDFLAGS 			:= $(BASIC_LFLAGS)
INCLUDE_PATH 		:= -I. $(foreach dir,$(INCPATH),-I$(dir))
LIBRARY 			:= 	lib$(MODULE).a
SYS_START_OBJ_FILE  :=  sys_start.o
#-----------------------------------------------------------------------------
FILES 			:= 	sys_main.c			\
					sys_fis.c			\
					sys_vctl.c			\
					sys_cfg.c			\
					sys_string.c		\
					sys_malloc.c		\
					sys_timer.c			\
					sys_crc16.c			\
					sys_printf.c		\
					trap.S			
#-----------------------------------------------------------------------------

include $(PACKAGE_ENV)/compiler.mak
build: $(BOARD_FOLDER)/obj/$(MODULE)/$(LIBRARY).stamp


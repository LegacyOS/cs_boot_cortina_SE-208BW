#-----------------------------------------------------------------------------
MODULE 		:= 	boot
#-----------------------------------------------------------------------------

include $(PACKAGE_ENV)/environment.mak
#CFLAGS 				:= $(BASIC_CFLAGS) -DNO_ERRNO_H
CFLAGS 					:= $(BASIC_CFLAGS) -DNO_ERRNO_H
LDFLAGS 				:= $(BASIC_LFLAGS)
INCLUDE_PATH 			:= -I. $(foreach dir,$(INCPATH),-I$(dir))
LIBRARY 				:= 	lib$(MODULE).a
BOOT_START_OBJ_FILE 	:=  boot_start.o
#-----------------------------------------------------------------------------
FILES 			:=	boot_main.c
#-----------------------------------------------------------------------------

include $(PACKAGE_ENV)/compiler.mak
build: $(BOARD_FOLDER)/obj/$(MODULE)/$(LIBRARY).stamp

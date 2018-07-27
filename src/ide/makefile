#-----------------------------------------------------------------------------
MODULE 		:= 	ide
#-----------------------------------------------------------------------------

include $(PACKAGE_ENV)/environment.mak
#CFLAGS 			:= $(BASIC_CFLAGS) -DNO_ERRNO_H
CFLAGS 				:= $(BASIC_CFLAGS) -DNO_ERRNO_H
LDFLAGS 			:= $(BASIC_LFLAGS)
INCLUDE_PATH 		:= -I. $(foreach dir,$(INCPATH),-I$(dir))
LIBRARY 			:= 	lib$(MODULE).a
#-----------------------------------------------------------------------------
FILES 			:= 	ide.c		\
					ext2fs.c	\
					aoe.c		\
					libata-scsi.c 
#-----------------------------------------------------------------------------

include $(PACKAGE_ENV)/compiler.mak
build: $(BOARD_FOLDER)/obj/$(MODULE)/$(LIBRARY).stamp


#-----------------------------------------------------------------------------
MODULE 		:= 	pci
#-----------------------------------------------------------------------------

include $(PACKAGE_ENV)/environment.mak
#CFLAGS 			:= $(BASIC_CFLAGS) -DNO_ERRNO_H
CFLAGS 				:= $(BASIC_CFLAGS) -DNO_ERRNO_H
LDFLAGS 			:= $(BASIC_LFLAGS)
INCLUDE_PATH 		:= -I. $(foreach dir,$(INCPATH),-I$(dir))
LIBRARY 			:= 	lib$(MODULE).a
#-----------------------------------------------------------------------------
FILES 			:= 	pci.c

#-----------------------------------------------------------------------------

include $(PACKAGE_ENV)/compiler.mak
build: $(BOARD_FOLDER)/obj/$(MODULE)/$(LIBRARY).stamp


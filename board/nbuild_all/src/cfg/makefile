#-----------------------------------------------------------------------------
MODULE 		:=	cfg
#-----------------------------------------------------------------------------

include $(PACKAGE_ENV)/environment.mak
CFLAGS 			:= $(BASIC_CFLAGS)
LDFLAGS 		:= $(BASIC_LFLAGS)
INCLUDE_PATH 	:= -I. $(foreach dir,$(INCPATH),-I$(dir))
LIBRARY 		:= 	lib$(MODULE).a

#-----------------------------------------------------------------------------
FILES 			:= 	board_version.c
#-----------------------------------------------------------------------------

include $(PACKAGE_ENV)/compiler.mak
build: $(BOARD_FOLDER)/obj/$(MODULE)/$(LIBRARY).stamp

version:
		rm -f $(OBJPATH)/board_version.*



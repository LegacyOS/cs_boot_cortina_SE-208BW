#-----------------------------------------------------------------------------
MODULE 		:= raid
#-----------------------------------------------------------------------------

include $(PACKAGE_ENV)/environment.mak
#CFLAGS 			:= $(BASIC_CFLAGS)
### compile issue we can't optimize these code !!  :(
CFLAGS				:= $(CPU_FLAG)  -Wno-implicit -Wno-uninitialized -Wno-undef -Wno-unknown-pragmas -Wno-unused -Wpointer-arith -Wstrict-prototypes -Winline -Wundef -g -O2 -ffunction-sections -fdata-sections -fno-exceptions
LDFLAGS 		:= $(BASIC_LFLAGS)
INCLUDE_PATH 	:= -I. $(foreach dir,$(INCPATH),-I$(dir))
LIBRARY 		:= 	lib$(MODULE).a

#-----------------------------------------------------------------------------
FILES 			:= 	raid.c   \
                                csumpartial.S
#-----------------------------------------------------------------------------

include $(PACKAGE_ENV)/compiler.mak
build: $(BOARD_FOLDER)/obj/$(MODULE)/$(LIBRARY).stamp



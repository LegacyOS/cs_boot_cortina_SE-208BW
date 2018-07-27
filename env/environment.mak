#=============================================================================
# 				Environment of Makefile
#=============================================================================
UTILITY 		:= $(PACKAGE_ROOT)/tools
BINPATH 		:= $(BOARD_FOLDER)/bin
LIBPATH 		:= $(BOARD_FOLDER)/lib
BOARD_SRC		:= $(BOARD_FOLDER)/src
SRCPATH 		:= $(PACKAGE_ROOT)/src
OBJPATH 		:= $(BOARD_FOLDER)/obj/$(MODULE)
INCPATH 		:= $(PACKAGE_ROOT)/inc $(SRCPATH) $(BOARD_FOLDER)/inc
FILEPATH 		:= $(SRCPATH)/$(MODULE)
CC 				:= $(CROSS_COMPILE)gcc
LD 				:= $(CROSS_COMPILE)ld
OBJCOPY 		:= $(CROSS_COMPILE)objcopy
AR 				:= $(CROSS_COMPILE)ar
MAP				:= $(CROSS_COMPILE)readelf
COMPRESS 		:= gzip -f -n
CP 				:= cp
CHMOD 			:= chmod
MAKE 			:= make
#CPU_FLAG		:= -mcpu=arm9
#CPU_FLAG		:= -DCONFIG_ARM -D__ARM__ -march=armv4
#BASIC_CFLAGS	:= $(CPU_FLAG) -mno-short-load-words -Wno-implicit -Wno-uninitialized -Wno-undef -Wno-unknown-pragmas -Wno-unused -Wpointer-arith -Wstrict-prototypes -Winline -Wundef -Woverloaded-virtual -g -O2 -ffunction-sections -fdata-sections -fno-rtti -fno-exceptions -fvtable-gc -finit-priority
#not chroot
#BASIC_CFLAGS	:= $(CPU_FLAG) -mno-short-load-words -Wno-implicit -Wno-uninitialized -Wno-undef -Wno-unknown-pragmas -Wno-unused -Wpointer-arith -Wstrict-prototypes -Winline -Wundef -g -O2 -ffunction-sections -fdata-sections -fno-exceptions -finit-priority
#BASIC_LFLAGS	:= $(CPU_FLAG) -mno-short-load-words -g -nostdlib -Wl,--gc-sections -Wl,-static
#chroot
BASIC_CFLAGS	:= $(CPU_FLAG)  -Wno-implicit -Wno-uninitialized -Wno-undef -Wno-unknown-pragmas -Wno-unused -Wpointer-arith -Wstrict-prototypes -Winline -Wundef -g -O2 -ffunction-sections -fdata-sections -fno-exceptions
BASIC_LFLAGS	:= $(CPU_FLAG)  -g -nostdlib -Wl,--gc-sections -Wl,-static

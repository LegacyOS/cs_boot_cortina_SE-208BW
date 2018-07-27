#-----------------------------------------------------------------------------
MODULE 		:= 	bsp
#-----------------------------------------------------------------------------

include $(PACKAGE_ENV)/environment.mak
CFLAGS 			:= $(BASIC_CFLAGS)
LDFLAGS 		:= $(BASIC_LFLAGS)
INCLUDE_PATH 	:= -I. $(foreach dir,$(INCPATH),-I$(dir))
LIBRARY 		:= 	lib$(MODULE).a

#-----------------------------------------------------------------------------
FILES			:=	uart/uart_drv.c         \
					uart/lpc.c				\
					flash/flash_drv.c       \
					flash/flash_amd.c		\
					flash/flash_nand.c      \
					flash/flash_m25p80.c      \
					flash/flash_at45db321.c      \
					emac/emac_sl2312.c      \
					emac/emac_phy.c         \
					emac/gmac_sl2312.c      \
					emac/gmac_phy.c			\
					emac/sl_lepus_gmac.c	\
					emac/lepus_gmac_phy.c	\
					emac/SPI.c				\
					cir/cir.c				  	
#-----------------------------------------------------------------------------

include $(PACKAGE_ENV)/compiler.mak
build: $(BOARD_FOLDER)/obj/$(MODULE)/$(LIBRARY).stamp



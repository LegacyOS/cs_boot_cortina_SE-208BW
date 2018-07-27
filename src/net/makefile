#-----------------------------------------------------------------------------
MODULE 		:= 	net
#-----------------------------------------------------------------------------

include $(PACKAGE_ENV)/environment.mak
#CFLAGS 			:= $(BASIC_CFLAGS) -DNO_ERRNO_H
CFLAGS 				:= $(BASIC_CFLAGS) -DNO_ERRNO_H
LDFLAGS 			:= $(BASIC_LFLAGS)
INCLUDE_PATH 		:= -I. $(foreach dir,$(INCPATH),-I$(dir))
LIBRARY 			:= 	lib$(MODULE).a
#-----------------------------------------------------------------------------
FILES 			:= 	net_main.c		\
					net_buf.c		\
					enet.c			\
					arp.c			\
					ip.c			\
					icmp.c			\
					udp.c			\
					tcp.c			\
					http.c			\
					tftp_client.c
#-----------------------------------------------------------------------------

include $(PACKAGE_ENV)/compiler.mak
build: $(BOARD_FOLDER)/obj/$(MODULE)/$(LIBRARY).stamp



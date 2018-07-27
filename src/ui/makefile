#-----------------------------------------------------------------------------
MODULE 		:= 	ui
#-----------------------------------------------------------------------------

include $(PACKAGE_ENV)/environment.mak
CFLAGS 			:= $(BASIC_CFLAGS)
LDFLAGS 		:= $(BASIC_LFLAGS)
INCLUDE_PATH 	:= -I. $(foreach dir,$(INCPATH),-I$(dir))
LIBRARY 		:= 	lib$(MODULE).a

#-----------------------------------------------------------------------------
FILES 			:= 	ui_menu.c		\
					ui_cli_main.c	\
					ui_cli_tbl.c	\
					ui_upgrade.c	\
					ui_lib.c		\
					ui_config.c		\
					ui_ping.c		\
					ui_tftp.c		\
					ui_go.c			\
					ui_load.c		\
					ui_dump.c		\
					ui_test.c
#-----------------------------------------------------------------------------

include $(PACKAGE_ENV)/compiler.mak
build: $(BOARD_FOLDER)/obj/$(MODULE)/$(LIBRARY).stamp



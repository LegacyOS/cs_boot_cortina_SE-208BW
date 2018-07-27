#-----------------------------------------------------------------------------
MODULE 		:=	hal
#-----------------------------------------------------------------------------

include $(PACKAGE_ENV)/environment.mak
CFLAGS 			:=	$(BASIC_CFLAGS)
LDFLAGS 		:=	$(BASIC_LFLAGS)
INCLUDE_PATH 	:=	-I. $(foreach dir,$(INCPATH),-I$(dir))
LIBRARY 		:= 	lib$(MODULE).a

#-----------------------------------------------------------------------------
FILES 			:= 	hal_sl2312.c	\
					hal_cache.c
#-----------------------------------------------------------------------------

include $(PACKAGE_ENV)/compiler.mak
build: $(BOARD_FOLDER)/obj/$(MODULE)/$(LIBRARY).stamp

$(LIBPATH)/$(BOOT1_START_O) : start.S
	@mkdir -p $(dir $@)
	$(CC) $< -c $(INCLUDE_PATH) -I$(dir $<) $(CFLAGS) -Wp,-MD,$(@:.o.d=.tmp) -o $(dir $@)$(notdir $(@:.o.d=.o)) -D _HAL_INIT_PLATFORM_

$(LIBPATH)/$(BOOT2_START_O) : start.S
	@mkdir -p $(dir $@)
	$(CC) $< -c $(INCLUDE_PATH) -I$(dir $<) $(CFLAGS) -Wp,-MD,$(@:.o.d=.tmp) -o $(dir $@)$(notdir $(@:.o.d=.o))




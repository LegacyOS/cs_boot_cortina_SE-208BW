#-----------------------------------------------------------------------------
#
# Boot Stage-I
#
export BOOT1_START_O	:=	boot1_start.o
BOOT1_LD 				:=   $(BOARD_FOLDER)/build/boot1.ld
BOOT1_BIN_FILE 			:= 	$(BOARD_BIN_FOLDER)/boot1.bin
BOOT1_ELF_FILE 			:= 	$(BOARD_BIN_FOLDER)/boot1.elf
BOOT1_MAP_FILE 			:= 	$(BOARD_BIN_FOLDER)/boot1.map

# Boot Stage-II
#
#include $(INCPATH)/board_config.h


export BOOT2_START_O	:=	boot2_start.o


#ifneq (RAM_SIZE,64)
#BOOT2_LD 				:=  $(BOARD_FOLDER)/build/boot2_128M.ld
#endif
#ifneq (RAM_SIZE,128)
#BOOT2_LD 				:=  $(BOARD_FOLDER)/build/boot2_64M.ld
#endif
ifdef RAM_128
BOOT2_LD 				:=  $(BOARD_FOLDER)/build/boot2_128M.ld
else
BOOT2_LD 				:=  $(BOARD_FOLDER)/build/boot2_64M.ld
endif

BOOT2_BIN_FILE 			:= 	$(BOARD_BIN_FOLDER)/boot2.bin
BOOT2_TEMP_FILE 		:= 	$(BOARD_BIN_FOLDER)/boot2
BOOT2_ELF_FILE 			:= 	$(BOARD_BIN_FOLDER)/boot2.elf
BOOT2_MAP_FILE 			:= 	$(BOARD_BIN_FOLDER)/boot2.map
BOOT2_COMPRESS_FILE		:= 	$(BOARD_BIN_FOLDER)/boot2.gz
BOOT2_UPGRADE_FILE 		:= 	$(BOARD_BIN_FOLDER)/boot2.img

#-----------------------------------------------------------------------------
SOURCES := $(foreach dir,$(LIBFILES), $(LIBPATH)/$(dir))

$(BOOT2_ELF_FILE): force
	@mkdir -p $(dir $@)
	$(MAKE) -r -C $(SRCPATH)/hal -f makefile $(BOARD_FOLDER)/obj/hal/libhal.a.stamp
	$(MAKE) -r -C $(SRCPATH)/bsp -f makefile $(BOARD_FOLDER)/obj/bsp/libbsp.a.stamp
	$(MAKE) -r -C $(SRCPATH)/ide -f makefile $(BOARD_FOLDER)/obj/ide/libide.a.stamp
	$(MAKE) -r -C $(SRCPATH)/raid -f makefile $(BOARD_FOLDER)/obj/raid/libraid.a.stamp
	$(MAKE) -r -C $(SRCPATH)/net -f makefile $(BOARD_FOLDER)/obj/net/libnet.a.stamp
	$(MAKE) -r -C $(SRCPATH)/sys -f makefile $(BOARD_FOLDER)/obj/sys/libsys.a.stamp
#	$(MAKE) -r -C $(SRCPATH)/astel -f makefile $(BOARD_FOLDER)/obj/astel/libastel.a.stamp
	$(MAKE) -r -C $(SRCPATH)/ui -f makefile $(BOARD_FOLDER)/obj/ui/libui.a.stamp
	$(MAKE) -r -C $(SRCPATH)/Xmodem -f makefile $(BOARD_FOLDER)/obj/Xmodem/libXmodem.a.stamp
	$(MAKE) -r -C $(SRCPATH)/pci -f makefile $(BOARD_FOLDER)/obj/pci/libpci.a.stamp
	$(MAKE) -r -C $(BOARD_SRC)/cfg -f makefile version
	$(MAKE) -r -C $(BOARD_SRC)/cfg -f makefile $(BOARD_FOLDER)/obj/cfg/libcfg.a.stamp
	$(MAKE) -r -C $(SRCPATH)/hal $(LIBPATH)/$(BOOT2_START_O)
	$(CC) $(LDFLAGS) -o $@ -L$(LIBPATH) -T$(BOOT2_LD)
	@echo "... finish to build "$@""

$(BOOT2_BIN_FILE): $(BOOT2_ELF_FILE)
	@mkdir -p $(dir $@)
	$(OBJCOPY) --strip-debug -O binary $< $@
	$(MAP) -s -S $< > $(BOOT2_MAP_FILE)
	@echo "... finish to build "$@""

force:	;

$(BOOT1_ELF_FILE): force
	@mkdir -p $(dir $@)
	$(MAKE) -r -C $(SRCPATH)/hal $(BOARD_FOLDER)/obj/hal/libhal.a.stamp
	$(MAKE) -r -C $(SRCPATH)/boot $(BOARD_FOLDER)/obj/boot/libboot.a.stamp
	$(MAKE) -r -C $(SRCPATH)/hal $(LIBPATH)/$(BOOT1_START_O)
	$(CC) $(LDFLAGS) -o $@ -L$(LIBPATH) -T$(BOOT1_LD)
	@echo "... finish to build "$@""
	
$(BOOT1_BIN_FILE): $(BOOT1_ELF_FILE)
	@mkdir -p $(dir $@)
	$(OBJCOPY) --strip-debug -O binary $< $@
	$(MAP) -s -S $< > $(BOOT1_MAP_FILE)
	@echo "... finish to build "$@""
	
$(TARGET_BIN_FILE): $(BOOT1_BIN_FILE) $(BOOT2_BIN_FILE)
	@mkdir -p $(dir $@)
	$(MERGE_CMD) $@ $(BOOT_SIZE) $(BOOT1_BIN_FILE) $(BOOT1_BIN_LOCATION) $(BOOT2_BIN_FILE) $(BOOT2_BIN_LOCATION) 
	@echo "... finish to build "$@""
	
clean:
	mv $(LIBPATH)/libastel.a ./
	$(MAKE) -r -C $(SRCPATH)/hal $@
	$(MAKE) -r -C $(SRCPATH)/bsp $@
	$(MAKE) -r -C $(SRCPATH)/ide $@
	$(MAKE) -r -C $(SRCPATH)/raid $@
	$(MAKE) -r -C $(SRCPATH)/sys $@
#	$(MAKE) -r -C $(SRCPATH)/astel $@
	$(MAKE) -r -C $(SRCPATH)/net $@
	$(MAKE) -r -C $(SRCPATH)/ui $@
	$(MAKE) -r -C $(SRCPATH)/Xmodem $@
	$(MAKE) -r -C $(SRCPATH)/pci $@
	$(MAKE) -r -C $(SRCPATH)/boot $@
	$(MAKE) -r -C $(BOARD_SRC)/cfg $@
	rm -f $(LIBPATH)/*.a
	mv ./libastel.a $(LIBPATH)
	rm -f $(LIBPATH)/*.o
	@echo "... end of clean"

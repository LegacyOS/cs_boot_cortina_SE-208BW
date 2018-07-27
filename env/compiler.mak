.PHONY: default build clean

# include any dependency rules generated previously
ifneq ($(wildcard $(BOARD_FOLDER)/obj/$(MODULE)/*.deps),)
include $(wildcard $(BOARD_FOLDER)/obj/$(MODULE)/*.deps)
endif

#include $(BOARD_FOLDER)/obj/$(MODULE)/lib$(MODULE).a.deps

SOURCES := $(foreach dir,$(FILES), $(OBJPATH)/$(dir))
OBJECTS := $(SOURCES:.cxx=.o.d)
OBJECTS := $(OBJECTS:.c=.o.d)
OBJECTS := $(OBJECTS:.S=.o.d)


$(OBJPATH)/%.o.d : %.c
	@mkdir -p $(dir $@)
	$(CC) $< -c $(INCLUDE_PATH) -I$(dir $<) $(CFLAGS) -Wp,-MD,$(@:.o.d=.tmp) -o $(dir $@)$(notdir $(@:.o.d=.o))
	@sed -e '/^ *\\/d' -e "s#.*: #$@: #" $(@:.o.d=.tmp) > $@
	@rm $(@:.o.d=.tmp)

$(OBJPATH)/%.o.d : %.cxx
	@mkdir -p $(dir $@)
	$(CC) $< -c $(INCLUDE_PATH) -I$(dir $<) $(CFLAGS) -Wp,-MD,$(@:.o.d=.tmp) -o $(dir $@)$(notdir $(@:.o.d=.o))
	@sed -e '/^ *\\/d' -e "s#.*: #$@: #" $(@:.o.d=.tmp) > $@
	@rm $(@:.o.d=.tmp)

$(OBJPATH)/%.o.d : %.S
	@mkdir -p $(dir $@)
	$(CC) $< -c $(INCLUDE_PATH) -I$(dir $<) $(CFLAGS) -Wp,-MD,$(@:.o.d=.tmp) -o $(dir $@)$(notdir $(@:.o.d=.o))
	@sed -e '/^ *\\/d' -e "s#.*: #$@: #" $(@:.o.d=.tmp) > $@
	@rm $(@:.o.d=.tmp)

$(BOARD_FOLDER)/obj/$(MODULE)/$(LIBRARY).stamp: $(OBJECTS)
	@mkdir -p $(LIBPATH)
	$(AR) rcs $(LIBPATH)/$(LIBRARY) $(foreach obj,$?,$(dir $(obj))$(notdir $(obj:.o.d=.o)))
	cat $^ > $(@:.stamp=.deps)
	@touch $@

# rule to clean the build tree
clean:
	@find $(OBJPATH) -type f -print | grep -v makefile | xargs rm -f
	@rm -f $(OBJPATH)/*.stamp $(OBJPATH)/*.deps
	@rm -f $(LIBPATH)/$(LIBRARY)


$(warning hello)
this_makefile = $(lastword $(MAKEFILE_LIST))

dirname = $(foreach d,$(dir $1),$(d:%/=%))

start_dir = $(shell pwd)

$(warning start_dir = $(start_dir))

include $(this_makefile:%/top.mk=%/mkdir.mk)

src_root = $(call dirname,$(call dirname,$(this_makefile)))
bld_root = $(src_root)/bld

top: all

.DEFAULT: | $(bld_root)/.
	$(info target = $@)
	$(info $|)
	$(MAKE) -C $(bld_root) \
		-I $(src_root)/make \
		-f $(src_root)/make/rules.mk \
		VPATH=$(src_root) \
		$(start_dir)/$@ 

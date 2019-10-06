.PHONY: all
all:

.SECONDEXPANSION:

src_root := .
bld_root = bld

$(warning src_root: $(realpath $(src_root)))
$(warning bld_root: $(realpath $(bld_root)))
$(if $(realpath $(bld_root)),,$(shell mkdir $(bld_root)))
# $(warning subst: $(subst $(realpath $(src_root)),,$(realpath $(bld_root))))

$(if $(subst $(realpath $(src_root)),,$(realpath $(bld_root))),\
    $(info paths ok),\
    $(error paths bad: $(realpath $(src_root)) $(realpath $(bld_root))))

bin = $(bld_root)/bin

stem = $1

this_rules.mk = $(lastword $(filter %rules.mk,$(MAKEFILE_LIST)))
# . = $(patsubst %/,%,$(dir $(this_rules.mk)))

. = $(this_rules.mk:%/rules.mk=%)

src = $(patsubst %/,%,$(dir $(this_rules.mk)))
bld = $(bld_root)/$(src)

include make/features/*.mk

deps =

-include ./rules.mk

stem = $*

include make/patterns/*.mk

# -include $(deps)


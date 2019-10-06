dbg = $(if $(DBG),$(warning $1))

dbg_obj = $(if $(DBG),$(call dbg_var,$(filter $1.%,$(.VARIABLES))))

dbg_var = $(foreach i,$1,$(warning $i = $($i)))

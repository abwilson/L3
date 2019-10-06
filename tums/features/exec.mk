target = $($(stem).target)

define exec_impl
    $(call dbg,Defining exec "$1".)
    $(eval $1.target ?= $(bld)/$1)
    execs += $(target)
    $(target): LDFLAGS = $($1.lddirs:%=-L%) $($1.libs:%=-l%)
    $(target): $($1.src:%.cpp=$(bld_root)/%.o)
    -include $(wildcard $($1.src:%.cpp=$(bld_root)/%.d)) /dev/null
    all: $(target)
endef

exec = $(eval $(exec_impl))



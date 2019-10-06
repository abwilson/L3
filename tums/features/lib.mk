define lib_impl
    $(info Defining lib "$1")
    libs += $1
    $(eval $1.target ?= $(bld_root)/lib/lib$1.a)
    all: $(target)
    $(target): LDFLAGS = $($1.libs:%=-l%)
    $(target): $($1.src:$(src)/%.cpp=$(bld)/%.o)
     -include $($1.src:$(src)/%.cpp=$(bld)/%.d)
endef

lib = $(eval $(lib_impl))


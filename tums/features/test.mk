define test_impl
    $(call dbg,Defining test "$1".)
    $(call dbg_obj,$1)
    $(eval test: $(1:%=$(bld)/%.ok))
    $(eval $1.src ?= $(wildcard $(src)/*Test.cpp))
    $(eval $1.libs += gtest_main gtest gmock)
    $(exec_impl)
    $(eval tests += $(target))
endef

test = $(eval $(test_impl))


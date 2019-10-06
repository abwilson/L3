#$(warning In $(src))
#$(warning bld $(bld))

$(src).tests = $(wildcard $(src)/test_*.cpp)

$(foreach t,$($(src).tests:$(src)/%.cpp=%),\
    $(eval $t.src = $(src)/$t.cpp) \
    $(eval $(call test,$t)))

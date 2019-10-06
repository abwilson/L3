CPPFLAGS =
CXXFLAGS = --std=c++17 -MD -g -O3

# cpp_read_deps = $(eval -include $(@:.o=.d))

cpp_files = $(wildcard $(src)/*.cpp)

# cpp_read_deps = \
#       $(shell test -f $(@:.o=.d) && make -BrRf make/show_deps.mk -f $(@:.o=.d) $@ 2> /dev/null)

    # $(info auto deps $@) \
    # $(info $(shell test -f $(@:.o=.d) && make -Brf make/show_deps.mk -f $(@:.o=.d) $@ 2> /dev/null))


# cpp_read_deps = test/../src/AddGlue.h

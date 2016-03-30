srcs.cpp	:= $(wildcard *.cpp)
srcs.c		:= $(wildcard *.c)
objs		:= $(srcs.cpp:%.cpp=%.o) $(srcs.c:%.c=%.o)
deps		:= $(srcs.cpp:%.cpp=%.d) $(srcs.c:%.c=%.d)
execs		:= $(srcs.cpp:%.cpp=%)   $(srcs.c:%.cpp=%)

valgrind_home = /opt/valgrind-svn
valgrind = $(valgrind_home)/bin/valgrind

benchmark_params = RING_SIZE

l3_macros = L3_ITERATIONS L3_QSIZE $(benchmark_params)

macros = $(foreach m,$(l3_macros),$($m:%=-D$m=$($m)))

opt_flag = -O3

#defines = $(foreach p,$(benchmark_params),$(if $($p),-D$p=$($p)))

# -pthread

CPPFLAGS = -I../include -I $(valgrind_home)/include $(macros)
CXXFLAGS =  -g $(opt_flag) --std=c++11 -MMD -MP -MF $(<:%.cpp=%.d) -MT $@
CXX = clang++


rapidjson = rapidjson/include

all: $(execs)

test: $(execs:%=%.run)

$(execs): %: %.o
	$(LINK.cc) $^ -o $@

clean:
	- rm -f $(execs) $(objs) $(deps)

.PHONY: force_run

run: $(execs:%=%.run)
%.run: % force_run
	./$<

time: $(execs:%=%.time)
%.time: % force_run
	bash -c "time ./$<"

%.valgrind: %
	$(valgrind) --tool=callgrind --instr-atstart=no ./$<

-include $(deps)

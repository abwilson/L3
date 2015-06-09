srcs.cpp	:= $(wildcard *.cpp)
srcs.c		:= $(wildcard *.c)
objs		:= $(srcs.cpp:%.cpp=%.o) $(srcs.c:%.c=%.o)
execs		:= $(srcs.cpp:%.cpp=%)   $(srcs.c:%.cpp=%)


valgrind_home = /opt/valgrind-svn
valgrind = $(valgrind_home)/bin/valgrind


CPPFLAGS = -I $(valgrind_home)/include
CXXFLAGS = -O3 --std=c++11
CXX = clang++ 


rapidjson = rapidjson/include

all: $(execs)

$(execs):

clean:
	- rm -f $(execs) $(objs)

%.run: %
	./$<

%.time: %
	bash -c "time ./$<"

%.valgrind: %
	$(valgrind) --tool=callgrind --instr-atstart=no ./$<

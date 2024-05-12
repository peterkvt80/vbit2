CXX=g++

CXXFLAGS = -g -O2 -Wall -MMD -MP -std=gnu++11 -fstack-protector-all -Wextra -I.

LIBS = -lpthread -fstack-protector -s

ifeq ($(OS),Windows_NT)
	CXXFLAGS += -DWIN32
	LIBS += -lwsock32 -static
else
	ifeq ($(shell test -e /etc/os-release && echo -n yes),yes)
		ifeq ($(shell if [ `grep -c raspbian /etc/os-release` -gt 0 ]; then echo true ; else echo false ; fi), true)
			CXXFLAGS += -DRASPBIAN
		endif
	endif
endif

srcs = $(wildcard *.cpp)
objs = $(srcs:.cpp=.o)
deps = $(srcs:.cpp=.d)

vbit2: $(objs)
	$(CXX) -o $@ $^ $(LIBS)

%.o: %.c $(deps)
	$(CXX) -c $< -o $@

#Cleanup
.PHONY: clean

clean:
	rm -f $(objs) $(deps) vbit2

-include $(deps)

all: vbit2

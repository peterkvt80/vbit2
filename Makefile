#Set the compiler for C++11 ("gcc" for C or "g++" for C++)
CC=g++

#Set any compiler flags you want to use (e.g. "-I."), or leave blank
CXXFLAGS = -g -O2 -Wall -std=gnu++11 -fstack-protector-all -Wextra -I.

LIBS = -lpthread -fstack-protector

ifeq ($(OS),Windows_NT)
CXXFLAGS += -DWIN32
LIBS += -lwsock32
endif

#Set any dependent files (e.g. header files) so that if they are edited they cause a re-compile (e.g. "main.h my_sub_functions.h some_definitions_file.h"), or leave blank
DEPS = vbit2.h service.h configure.h pagelist.h ttxpage.h packet.h tables.h mag.h ttxpagestream.h ttxline.h carousel.h filemonitor.h command.h TCPClient.h newfor.h hamm-tables.h packetsource.h packetmag.h packet830.h packetsubtitle.h ttxcodes.h specialpages.h

OBJ = vbit2.o service.o configure.o pagelist.o ttxpage.o packet.o tables.o mag.o ttxpagestream.o ttxline.o carousel.o filemonitor.o command.o TCPClient.o newfor.o packetsource.o packetmag.o packet830.o packetsubtitle.o specialpages.o

#Below here doesn't need to change
#Compile each object file
%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

#Combine them into the output file
vbit2: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

#Cleanup
.PHONY: clean

clean:
	rm -f *.o *~ core *~

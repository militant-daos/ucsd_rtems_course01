#INCLUDE_DIRS =
INCLUDE_DIRS = -I../../common
LIB_DIRS =
CXX=g++

CXXDEFS=
CXXFLAGS= --std=c++11 -ggdb -Wall -Werror -Wpedantic -O0 $(INCLUDE_DIRS) $(CXXDEFS)
LIBS=

HFILES= common.h threading.h rt_time.h string_utils.h error_codes.h
CPPFILES= posix_clock.cpp

SRCS= ${HFILES} ${CPPFILES}
OBJS= ${CPPFILES:.cpp=.o}

all:	posix_clock

clean:
	-rm -f *.o *.d
	-rm -f posix_clock

distclean:
	-rm -f *.o *.d
	-rm -f posix_clock

posix_clock: posix_clock.o
	$(CXX) $(LDFLAGS) -o $@ $@.o -lpthread -lstdc++ -lrt

depend:

.c.o:
	$(CXX) $(CXXFLAGS) -c $<

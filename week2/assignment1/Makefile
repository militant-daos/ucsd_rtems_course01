#INCLUDE_DIRS =
INCLUDE_DIRS = -I../../common
LIB_DIRS =
CXX=g++

CXXDEFS=
CXXFLAGS= --std=c++11 -ggdb -Wall -Werror -Wpedantic -O0 $(INCLUDE_DIRS) $(CXXDEFS)
LIBS=

HFILES= common.h threading.h
CPPFILES= pthread.cpp

SRCS= ${HFILES} ${CPPFILES}
OBJS= ${CPPFILES:.cpp=.o}

all:	pthread

clean:
	-rm -f *.o *.d
	-rm -f perfmon pthread

distclean:
	-rm -f *.o *.d
	-rm -f pthread

pthread: pthread.o
	$(CXX) $(LDFLAGS) -o $@ $@.o -lpthread

depend:

.c.o:
	$(CXX) $(CXXFLAGS) -c $<

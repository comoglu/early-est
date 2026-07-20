
# Build environment can be configured the following
# environment variables:
#   CC : Specify the C compiler to use
#   CFLAGS : Specify compiler options to use

BINDIR=${MYBIN}

# 20210908 AJL  LIBRARY_PATH=/usr/local/lib

# Options specific for GCC
ifndef CC
CC = gcc
endif
#
ifndef CCFLAGS
#
CCFLAGS_BASIC =  -Wall -fPIC -I..
#
# optimized
CCFLAGS = -O3 $(CCFLAGS_BASIC)
#
# profile
#CCFLAGS=-O3 -pg $(CCFLAGS_BASIC)
#
# debug - gdb, valgrind, ...
#CCFLAGS = $(CCFLAGS_BASIC) -g
# valgrind --leak-check=yes  exe_name <args>
# valgrind --leak-check=full --show-reachable=yes exe_name exe_name <args>
endif


LIB_A = libtdproc.a

all: $(LIB_A) install

$(LIB_A): $(LIB_OBJS)
	rm -f $(LIB_A)
	ar -csq $(LIB_A) $(LIB_OBJS)

clean:
	rm -f *.o
	rm -f *.a

# specific rule
FILTER_SOURCES = timedomain_filter_1.c timedomain_filter_2.c timedomain_filter_3.c timedomain_filter_6.c
timedomain_filter.o: timedomain_filter.c $(FILTER_SOURCES)

# Implicit rule for building object files
%.o: %.c
	$(CC) -c $< $(CCFLAGS)

# 20210908 AJL - modified to simply leave in this directory, to avoid LD_LIBRARY_PATH problems on different systems
#install:
#	@echo
#	@echo "Installing library using: cp $(LIB_A) ../applications"
#	cp $(LIB_A) ../applications
#	@echo
install:
	@echo
	@echo "Built library: $(LIB_A)"
	@echo

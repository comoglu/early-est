# Makefile

#  IMPORTANT: change here to set the directory for binary executable files
ifdef MYBIN
BINDIR=${MYBIN}
else
# with the following, binary executables will be placed in bin subdirectory of your home directory
#BINDIR=~/bin/
# if in doubt, use the following - binary executables will be placed in the current directory
BINDIR=.
endif

# IMPORTANT: librabbitmq stuff
# REQUIRES: rabbitmq-c-* to compile!
# see top-level Makefile for instructions


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


all : ${BINDIR}/amqp_sendstring_test ${BINDIR}/amqp_listen_test


OBJS=amqp_sendstring_test.o amqp_sendstring_func.o amqp_listen_test.o


%.o : %.c
	$(CC) -c $(CCFLAGS) $(AMQP_INCLUDE_PATH) $< -o $@


clean :
	rm -f *.o
	rm -f *.a
	rm -f ${BINDIR}/amqp_sendstring_test
	rm -f ${BINDIR}/amqp_listen_test


# --------------------------------------------------------------------------
# amqp_sendstring_test
#
OBJ_TEST1= amqp_sendstring_test.o amqp_sendstring_func.o
${BINDIR}/amqp_sendstring_test : ${OBJ_TEST1}
	$(CC) ${OBJ_TEST1} ${CCFLAGS} -o ${BINDIR}/amqp_sendstring_test $(AMQP_LIBRARY_PATH) -lrabbitmq
# --------------------------------------------------------------------------

# --------------------------------------------------------------------------
# amqp_sendstring_test amqp_listen_test
#
OBJ_TEST2= amqp_listen_test.o
${BINDIR}/amqp_listen_test : ${OBJ_TEST2}
	$(CC) ${OBJ_TEST2} ${CCFLAGS} -o ${BINDIR}/amqp_listen_test $(AMQP_LIBRARY_PATH) -lrabbitmq
# --------------------------------------------------------------------------

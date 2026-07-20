
# Build environment can be configured the following
# environment variables:
#   CC : Specify the C compiler to use
#   CFLAGS : Specify compiler options to use

#  IMPORTANT: change here to set the directory for binary executable files
ifdef MYBIN
BINDIR=${MYBIN}
else
# with the following, binary executables will be placed in bin subdirectory of your home directory
#BINDIR=~/bin/
# if in doubt, use the following - binary executables will be placed in the current directory
BINDIR=.
endif


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


OBJS_SDT = student_t.o incbet.o incbi.o ndtri.o polevl.o
OBJS = statistics.o ${OBJS_SDT}

all: $(OBJS) ${BINDIR}/test_student_t ${BINDIR}/student_t_table

clean:
	rm -f *.o
	rm -f *.a

# Implicit rule for building object files
%.o: %.c
	$(CC) -c $< $(CCFLAGS)


# --------------------------------------------------------------------------
# response_test
#
OBJ_TEST1= test_student_t.o $(OBJS_SDT)
${BINDIR}/test_student_t : ${OBJ_TEST1}
	$(CC) ${OBJ_TEST1} ${CCFLAGS} -o ${BINDIR}/test_student_t -lm
# --------------------------------------------------------------------------

# --------------------------------------------------------------------------
# response_test
#
OBJ_TEST2= student_t_table.o $(OBJS_SDT)
${BINDIR}/student_t_table : ${OBJ_TEST2}
	$(CC) ${OBJ_TEST2} ${CCFLAGS} -o ${BINDIR}/student_t_table -lm
# --------------------------------------------------------------------------

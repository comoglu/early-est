
# Build environment can be configured the following
# environment variables:
#   CC : Specify the C compiler to use
#   CFLAGS : Specify compiler options to use

#  IMPORTANT: change here to set the directory for binary executable files
ifdef MYBIN
export BINDIR=${MYBIN}
else
# with the following, binary executables will be placed in bin subdirectory of your home directory
#BINDIR=~/bin/
# if in doubt, use the following - binary executables will be placed in the current directory
export BINDIR=.
endif

# Options specific for GCC
ifndef CC
export CC = gcc
endif
#
ifndef CCFLAGS
#
export CCFLAGS_BASIC =  -Wall -fPIC -I..
#
# optimized
export CCFLAGS = -O3 $(CCFLAGS_BASIC)
#
# profile
#CCFLAGS=-O3 -pg $(CCFLAGS_BASIC)
#
# debug - gdb, valgrind, ...
#CCFLAGS = $(CCFLAGS_BASIC) -g
# valgrind --leak-check=yes  exe_name <args>
# valgrind --leak-check=full --show-reachable=yes exe_name exe_name <args>
endif

#LDFLAGS = -L..
#LDLIBS = -lm

OBJ=read_input.o write_output.o fmamp_invert.o  fmamp_utils.o octtree/octtree.o vector/vector.o \
    matrix_statistics/matrix_statistics.o alomax_matrix/alomax_matrix.o alomax_matrix/alomax_matrix_svd.o ../alomax_matrix/polarization.o ../alomax_matrix/eigv.o \
    ran1/ran1.o settings/settings.o settings/strmap.o statistics/statistics.o


all: $(BINDIR)/fmamp

$(BINDIR)/fmamp: fmamp.o modules $(OBJ)
	$(CC) $(CCFLAGS) -o $(BINDIR)/fmamp fmamp.o $(OBJ) -lm


MODULES = \
	octtree vector matrix_statistics alomax_matrix ran1 settings statistics

modules:
	@for x in $(MODULES); \
	do \
		(echo ------; cd $$x; echo Making $@ in:; pwd; \
		make -f Makefile); \
	done

clean:
	@for x in $(MODULES); \
	do \
		(cd $$x; echo Cleaning in:; pwd; \
		make -f Makefile clean); \
	done
	rm -f $(BINDIR)/fmamp
	rm -f *.o
	rm -f *.a


# Implicit rule for building object files
%.o: %.c
	$(CC) -c $< $(CCFLAGS)

install:
	@echo
	@echo "No install target, copy the executable(s) yourself"
	@echo

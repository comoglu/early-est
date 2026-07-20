

# 20110523 AJL
#
#  IMPORTANT: change here to set the directory for binary executable files
ifdef MYBIN
BINDIR=${MYBIN}
else
# with the following, binary executables will be placed in bin subdirectory of your home directory
#BINDIR=~/bin/
# if in doubt, use the following - binary executables will be placed in the current directory
BINDIR=.
endif
#
#FCMP = gfortran -g -Wall
#FCMP = gfortran -O3
FCMP = /usr/local/bin/gfortran -O3


all:	$(BINDIR)/hash_driver_early_est $(BINDIR)/hash_driver1 $(BINDIR)/hash_driver2 $(BINDIR)/hash_driver3 $(BINDIR)/hash_driver4 $(BINDIR)/hash_driver5

early_est:	$(BINDIR)/hash_driver_early_est


clean:
	rm -f *.o $(BINDIR)/hash_driver? $(BINDIR)/hash_driver_early_est


OBJS1=	fmech_subs.o \
        pol_subs.o \
        station_subs.o \
        uncert_subs.o \
	util_subs.o

OBJS2=	fmech_subs.o \
        pol_subs.o \
        station_subs.o \
        uncert_subs.o \
	util_subs.o \
        vel_subs.o

OBJS3=	fmamp_subs.o \
        pol_subs.o \
        station_subs.o \
        uncert_subs.o \
	util_subs.o \
        vel_subs.o

OBJS4=	fmech_subs.o \
        pol_subs.o \
        station_subs_5char.o \
        uncert_subs.o \
	util_subs.o \
        vel_subs.o

$(BINDIR)/hash_driver_early_est: Makefile hash_driver_early_est.o $(OBJS1)
	$(FCMP)  hash_driver_early_est.o $(OBJS1) -o $(BINDIR)/hash_driver_early_est

$(BINDIR)/hash_driver1: Makefile hash_driver1.o $(OBJS1)
	$(FCMP)  hash_driver1.o $(OBJS1) -o $(BINDIR)/hash_driver1

$(BINDIR)/hash_driver2: Makefile hash_driver2.o $(OBJS2)
	$(FCMP)  hash_driver2.o $(OBJS2) -o $(BINDIR)/hash_driver2

$(BINDIR)/hash_driver3: Makefile hash_driver3.o $(OBJS3)
	$(FCMP)  hash_driver3.o $(OBJS3) -o $(BINDIR)/hash_driver3

$(BINDIR)/hash_driver4: Makefile hash_driver4.o $(OBJS4)
	$(FCMP)  hash_driver4.o $(OBJS4) -o $(BINDIR)/hash_driver4

$(BINDIR)/hash_driver5: Makefile hash_driver5.o $(OBJS2)
	$(FCMP)  hash_driver5.o $(OBJS2) -o $(BINDIR)/hash_driver5

pol_subs.o : Makefile pol_subs.f
	$(FCMP) pol_subs.f -c

fmech_subs.o : Makefile fmech_subs.f
	$(FCMP) fmech_subs.f -c

fmamp_subs.o : Makefile fmamp_subs.f
	$(FCMP) fmamp_subs.f -c

station_subs.o : Makefile station_subs.f
	$(FCMP) station_subs.f -c

station_subs_5char.o : Makefile station_subs_5char.f
	$(FCMP) station_subs_5char.f -c

uncert_subs.o : Makefile uncert_subs.f
	$(FCMP) uncert_subs.f -c

util_subs.o : Makefile util_subs.f
	$(FCMP) util_subs.f -c

vel_subs.o : Makefile vel_subs.f
	$(FCMP) vel_subs.f -c

# Implicit rule for building object files
%.o: %.f
	$(FCMP) -c $<





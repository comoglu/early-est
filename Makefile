
# =========================================================================================================================
# options for applications
# =========================================================================================================================

# =========================================================================================================================
# conditional compiling of different velocity models / travel-time grids / phase types, etc.
#
# global scale AK135 model, time grid and phases generated with iaspei-tau FORTRAN (ttimes_ak135_0-800_10*)
TTIME_DEFS=-D TTIMES_AK135_GLOBAL
export TTIMES_NAME=
#
# global scale AK135 model, time grid and phases generated with edu.sc.seis.TauP Java (ttimes_ak135_0-800_10_TauP*)
#TTIME_DEFS=-D TTIMES_AK135_GLOBAL_TAUP
#export TTIMES_NAME=_AK135_GLOBAL_TAUP
#
# regional scale AK135 model, time grid and phases (ttimes_ak135_regional*)
#TTIME_DEFS=-D TTIMES_AK135_REGIONAL -D TTIMES_REGIONAL
#export TTIMES_NAME=_AK135_REGIONAL
#
# regional scale marsite1 model, time grid and phases (ttimes_marsite1_regional*)
# Estelle CROS e-mail 20130419
#TTIME_DEFS=-D TTIMES_MARSITE1_REGIONAL -D TTIMES_REGIONAL
#export TTIMES_NAME=_MARSITE1_REGIONAL
# AJL adapted with Vp/Vs=1.73 from: 2008__Becel_et__Moho_crustal_architecture_and_deep_deformation_under_the_North_Marmara_Trough_from_the_SEISMARMARA_Leg_1_offshore–onshore_reflection–refraction_survey__Tectonopyhsics
#TTIME_DEFS=-D TTIMES_MARSITE2_REGIONAL -D TTIMES_REGIONAL
#export TTIMES_NAME=_MARSITE2_REGIONAL
#
# regional scale 1D Velocity Model Used by the SCSN (Hadley and Kanamori, 1977; K Hutton et. al, BSSA, 2010)
# HK Model with Vp/Vs=1.73
# Depth to Top of Layer (km), P-Velocity (km/sec)
# 0.0 5.5
# 5.5 6.3
# 16.0 6.7
# 32.0 7.8
#TTIME_DEFS=-D TTIMES_SOCALSN_HK_2010 -D TTIMES_REGIONAL
#export TTIMES_NAME=_SOCALSN_HK_2010
#
# Longmenshan (2008/05/12 Wenchuan, China)
# Zheng Y, Ma H S, Lü J, et al. Source mechanism of strong aftershocks (Ms≥5.6) of the 2008/05/12 Wenchuan earthquake and the implication for seismotectonics. Sci China Ser D-Earth Sci, 2009, 52(6): 739-753, doi: 10.1007/s11430-009-0074-3
#TTIME_DEFS=-D TTIMES_LONGMENSHAN_REGIONAL -D TTIMES_REGIONAL
#export TTIMES_NAME=_LONGMENSHAN_REGIONAL
#
# regional scale TexNet model, time grid and phases (ttimes_marsite1_regional*)
#TTIME_DEFS=-D TTIMES_R4_S3_T3_IASP91_REGIONAL -D TTIMES_REGIONAL
#export TTIMES_NAME=_R4_S3_T3_IASP91_REGIONAL
#
#
export EXE_NAME=$(TTIMES_NAME)
# =========================================================================================================================

# =========================================================================================================================
# AJL_TEST : development / test functions - WARNING: not fully implemented, debugged, tested or memory checked!
# You can comment out the following line to disable all development / test functions
#
DEV_DEFS= $(DEV_DEFS_0) $(DEV_DEFS_1) $(DEV_DEFS_2) $(DEV_DEFS_3) $(DEV_DEFS_4) $(DEV_DEFS_5) $(DEV_DEFS_6) $(DEV_DEFS_7) $(DEV_DEFS_8) $(DEV_DEFS_9)
#
#DEV_DEFS_0= -D COMPILE_OLD_VERSION # DO NOT USE!
#
#export EXE_NAME=$(TTIMES_NAME)_ALPHA   # X 20150415
#DEV_DEFS_0= -D ALPHA_VERSION   # X 20150415
#
# 20150401 AJL - Uncomment the following line to﻿use all picks for location, irrespective of HF S/N.
DEV_DEFS_1= -D USE_USE_ALL_PICKS_FOR_LOCATION
#
# =========================================================================================================================



# NOTE: following no longer needed, superseded by Leaflet mapping
# Google Maps JavaScript API key  ===============================================================================================================
# To use the Maps JavaScript API, you must get an API key which you can then add to your mobile app, website, or web server.
# The API key is used to track API requests associated with your project for usage and billing.
# https://developers.google.com/maps/documentation/javascript/get-api-key#get-an-api-key
#
# key for http://your.website/*
#GOOGLE_API_KEY=XXX
#
# key for http://alomax.free.fr/*
#DEFS_GOOGLE_API_KEY= -D GOOGLE_API_KEY=\"XXXX\"
#
# =========================================================================================================================



# rabbitmq-c DEFS ===============================================================================================================
# IMPORTANT: librabbitmq stuff (20171226 - AJL added)
# REQUIRES: rabbitmq-c-0.8.0 to compile!
# 1) Make sure librabbitmq is on your environment LD_LIBRARY_PATH:
# export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:<path to rabbitmg install>/rabbitmq-c-0.8.0/build/librabbitmq
# 2) Define AMQP_INCLUDE_PATH and AMQP_LIBRARY_PATH
# export AMQP_INCLUDE_PATH=-I<path to rabbitmg install>/rabbitmq-c-0.8.0/librabbitmq
# export AMQP_LIBRARY_PATH=-L<path to rabbitmg install>/rabbitmq-c-0.8.0/build/librabbitmq
#
# Uncomment the following line to ﻿send messages though rabbitmq message broker
###AMQP_DEFS= -D USE_RABBITMQ_MESSAGING
#
ifdef AMQP_DEFS
ifndef AMQP_INCLUDE_PATH
export AMQP_INCLUDE_PATH= -I$(HOME)/opt/rabbitmq-c-0.8.0/librabbitmq
endif
ifndef AMQP_LIBRARY_PATH
export AMQP_LIBRARY_PATH= -L$(HOME)/opt/rabbitmq-c-0.8.0/build/librabbitmq
endif
export LDFLAGS_RMQ= $(AMQP_LIBRARY_PATH)
export LDLIBS_RMQ= -lrabbitmq
RMQ_OBJ= ../rabbitmq/amqp_sendstring_func.o
RMQ_MODULES= rabbitmq
endif
# END - AMQP_DEFS ===============================================================================================================

# json-c DEFS ===============================================================================================================
# IMPORTANT: json-c stuff (20211117 - AJL added)
# REQUIRES: json-c to compile!
# See here to install: https://github.com/json-c/json-c
# 1) Make sure json-c is on your environment LD_LIBRARY_PATH:
# export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/usr/local/lib
# 2) Define JSONC_INCLUDE_PATH
#
# Uncomment the following line to enable use of JSON output format
###JSON_DEFS= -D USE_JSON_OUTPUT
#
ifdef JSON_DEFS
ifndef JSONC_INCLUDE_PATH
export JSONC_INCLUDE_PATH= -I/usr/local/include
endif
export LDLIBS_JSON= -ljson-c
JSON_OBJ= ../json/json_lib.o
JSON_MODULES= json
endif

# libcurl DEFS ===============================================================================================================
# 20260720 - added: enables TLS/HTTPS support for the internet gain/station web-service
# queries (-sta-query/-pz-query), via libcurl, fixing failures against FDSN services that
# now require HTTPS (e.g. IRIS/EarthScope) -- the built-in fetch code otherwise only ever
# speaks plain HTTP on port 80. See PATCHES.md for the full diagnosis.
# REQUIRES: libcurl, e.g.:
#   apt install libcurl4-openssl-dev   (Debian/Ubuntu)
#   brew install curl                  (macOS)
#
# Comment out the following line if libcurl is not available -- everything else builds and
# runs as before, just without HTTPS support for these web-service queries.
LIBCURL_DEFS= -D USE_LIBCURL
#
ifdef LIBCURL_DEFS
export LDLIBS_CURL= -lcurl
endif
# END - libcurl DEFS ===============================================================================================================


# Uncomment the following line to ﻿enable OpenMP parallel programming (http://www.openmp.org)
# DO NOT USE!
#FLAG_OPENMP= -fopenmp -D USE_OPENMP


# Options specific for GCC
export CC = gcc
#export CC = /usr/local/bin/gcc-7
#
# GNU
GNU_SOURCE=-D _GNU_SOURCE
# IMPORTANT: try the following if the compiler you are using does not support GNU extensions!
#GNU_SOURCE=
#
# 20260720 - libslink headers: point this at wherever libslink is installed/built on your
# system (only needed if the compiler can't already find <libslink/libslink.h> via the
# standard include path). Was previously hardcoded to one deployment's home directory.
ifndef LIBSLINK_INCLUDE_PATH
export LIBSLINK_INCLUDE_PATH=
endif
#
CCFLAGS_BASIC =  -std=c99 -Wall -Wno-format-truncation -fcommon $(LIBSLINK_INCLUDE_PATH) $(GNU_SOURCE) `xml2-config --cflags` $(TTIME_DEFS) $(AMQP_DEFS) ${JSON_DEFS} $(JSONC_INCLUDE_PATH) $(LIBCURL_DEFS) $(FLAG_OPENMP) $(DEV_DEFS) $(DEFS_GOOGLE_API_KEY)
# 20160513 - -mcmodel=large flag may be needed on Linux per suggestion by mehmety@boun.edu.tr
#CCFLAGS_BASIC = -mcmodel=large -std=c99 -Wall $(GNU_SOURCE) `xml2-config --cflags` $(TTIME_DEFS) $(DEV_DEFS)

#
# all warnings
#export CCFLAGS_BASIC = -Wextra $(CCFLAGS_BASIC)
#
#
# basic
#export CCFLAGS = $(CCFLAGS_BASIC)
#
# optimized
export CCFLAGS = -O3 $(CCFLAGS_BASIC)
#
# profile
#export CCFLAGS= -pg -p $(CCFLAGS_BASIC)
#
# debug - gdb, valgrind, ...
#export CCFLAGS = -g $(CCFLAGS_BASIC)
#export CCFLAGS = -g -O3 $(CCFLAGS_BASIC) -g -O3
# gdb:
# gdb --args exe_name
# gdb-apple --args exe_name
# lldb -- exe_name
# lldb -o run -- <exe_name> <exe_args>
#
# valgrind:
# 20220201 AJL - Install:
# brew tap LouisBrunner/valgrind
# brew install --HEAD LouisBrunner/valgrind/valgrind
#
# !!! Note: 20121213 AJL - valgrind only works if remove -c ./plot_warning_report_seedlink_runtime.bash from seedlink_monitor command line!
#           20141126 AJL - Above may not be necessary with Valgrind-3.10.0
# valgrind --leak-check=yes --dsymutil=yes exe_name <args>
# w/o -O3 : valgrind --leak-check=yes --dsymutil=yes --track-origins=yes exe_name <args>
# valgrind --leak-check=full --show-reachable=yes --dsymutil=yes exe_name <args>
# valgrind --tool=callgrind --dsymutil=yes exe_name <args>  # callgrind_annotate [options] --tree=both --inclusive=yes callgrind.out.<pid>



export LIB_OBJS =  $(RMQ_OBJ) $(JSON_OBJ) \
../timedomain_processing/timedomain_processing.o ../timedomain_processing/timedomain_processing_data.o  ../timedomain_processing/timedomain_processing_report.o \
../timedomain_processing/timedomain_processes.o ../timedomain_processing/timedomain_filter.o ../timedomain_processing/timedomain_memory.o \
../timedomain_processing/ttimes.o ../timedomain_processing/location.o ../timedomain_processing/time_utils.o \
../timedomain_processing/loc2xml.o ../vector/vector.o ../statistics/statistics.o \
../alomax_matrix/alomax_matrix.o ../alomax_matrix/alomax_matrix_svd.o ../alomax_matrix/polarization.o ../alomax_matrix/eigv.o ../matrix_statistics/matrix_statistics.o \
../picker/PickData.o ../picker/FilterPicker5_Memory.o  ../picker/FilterPicker5.o \
../net/net_lib.o ../response/response_lib.o ../math/complex.o ../ran1/ran1.o ../octtree/octtree.o ../feregion/feregion.o \
../settings/settings.o ../settings/strmap.o ../miniseed_utils/miniseed_utils.o


MODULES = $(RMQ_MODULES) \
	math \
	vector \
	statistics \
	alomax_matrix \
	matrix_statistics \
	picker \
	net \
	response \
	ran1 \
	octtree \
	feregion \
	settings \
	miniseed_utils \
	$(JSON_MODULES) \
	timedomain_processing \
	fmamp \
	applications

all:
	@for x in $(MODULES); \
	do \
		(echo ------; cd $$x; echo Making $@ in:; pwd; \
		make -f Makefile); \
	done
	@echo
	gcc --version
	@echo
	@echo "Notes:"
	@echo "   Build requires Libxml2 (XML C parser and toolkit) available from http://xmlsoft.org/ and from http://www.macports.org/"
	@echo "   Velocity model / travel-time grid: $(TTIME_DEFS)"
	@echo

clean:
	@for x in $(MODULES); \
	do \
		(cd $$x; echo Cleaning in:; pwd; \
		make -f Makefile clean); \
	done

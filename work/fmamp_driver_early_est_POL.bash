#!/bin/bash

HYPOS_PATH=$1
echo "HYPOS_PATH ${HYPOS_PATH}"

rm -f ${HYPOS_PATH}/events/hypo.*.fmamp.mech.*


# prepare fmamp properties file
cat << END > fmamp_driver_early_est.prop
#=========================================
# section name (do not remove)
[Program]
#--- The directory in which to write output files. type=string default=./out
outpath=${HYPOS_PATH}/events/
#--- Variant name modifier for output files (e.g. _amplitude). type=string default=<none>
variant_name=_polarity
#--- Format of input files. type=string default=EARLY_EST_CSV  (Early-est hypos.csv format)
input_format=EARLY_EST_CSV
#--- Name of hypos input file (Early-est hypos.csv format). type=string default=./hypos.csv
hypofile=${HYPOS_PATH}/hypos.csv
#--- Name of picks input file (Early-est picks.csv format). type=string default=./picks.csv
pickfile=${HYPOS_PATH}/picks.csv

[Inversion]
#--- Minimum number of first-motion/amplitude observations to enable inversion. type=int default=10
min_number_observations=8
#--- Critical number of first-motion/amplitude observations for weighting inversion. type=int default=10
# 20140905 AJL  critical_number_observations=15
critical_number_observations=25
#--- Maximum number of oct-tree nodes to evaluate. type=int default=10000
# 20140905 AJL  
max_num_nodes=20000
#max_num_nodes=50000
#--- Minimum node size in degrees of strike/dip/rake. type=int default=1.0
min_node_size_deg=1.0
#--- Node size for initial, regular grid in degrees of strike/dip/rake. type=int default=10.0
initial_grid_step=10.0
#--- Use first motion polarities for inversion, 0=NO, 1=Yes. type=int default=1
use_first_motion_polarities=1
#--- Use amplitudes for inversion, 0=NO, 1=Yes. type=int default=0
use_amplitudes=0

END


# run fmamp
fmamp -p fmamp_driver_early_est.prop


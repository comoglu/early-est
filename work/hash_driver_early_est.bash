#!/bin/bash

HYPOS_PATH=$1
echo "HYPOS_PATH ${HYPOS_PATH}"

# 20121019 AJL - maxout changed 300->150, max az gap 180->150, grid angle 2->2
# 20140808 AJL - minimum number of polarities changed 10->8 (for comparison with fmamp)

# run HASH
cat << END > hash_driver_early_est.inp
'station_polarity_reverse.csv'		! station polarity reversal file (NOTE: note implemented yet in hash_driver_early_est.c)
'${HYPOS_PATH}/hypos.csv'			! name of hypos input file (Early-est hypos.csv format)
'${HYPOS_PATH}/picks.csv'			! name of picks input file (Early-est picks.csv format)
'${HYPOS_PATH}/preferred.out'			! output file name for focal mechanisms
'${HYPOS_PATH}/acceptable.out'		! output file name for acceptable planes
'${HYPOS_PATH}/events/'						! output file path for Early-est/GMT plot files
8                          ! minimum number of polarities (e.g., 8)
150                         ! maximum azimuthal gap (e.g., 90)
90                          ! maximum takeoff angle gap (e.g., 60)
2                           ! grid angle for focal mech search, in degrees         (min    5.0000000     )
1                           ! number of trials (e.g., 30)
150                         ! maxout for focal mech. output (e.g., 500)
0.1                         ! fraction of picks presumed bad (e.g., 0.10)
9999999                     ! maximum allowed source-station distance,                    in km (e.g., 120)
45                          ! angle for computing mechanisms probability,                 in degrees (e.g., 45)
0.25                        ! probability threshold for multiples (e.g., 0.1)

END

rm -f ${HYPOS_PATH}/events/hypo.*.hash.mech.*
#time 
hash_driver_early_est < hash_driver_early_est.inp 
#gdb hash_driver_early_est # < hash_driver_early_est.inp 


#! /usr/bin/python

# This file is part of the Anthony Lomax Python Library.
# 
# Copyright (C) 2011 Anthony Lomax <anthony@alomax.net www.alomax.net>
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

__author__ = 'anthony'
__date__ = '$Jul 06, 2016$'


import pandas as pd
import rupturePredict as rp
from math import isnan

if __name__ == '__main__':
    import sys
    
    hypolist_file = '/Users/anthony/work/early-est/tsunamiLearn/data/1994-201606_ee_merge_finite_fault.csv'
    P_underwater =  None
    Ndip =  None
    
    hypos_file = open(hypolist_file)
    df_hypos = pd.read_csv(hypos_file)
    
    print 'event_id ff_duration_obs ff_duration_pred ff_rupture_area_obs ff_rupture_area_obs  At_obs At_pred  It_obs It_pred'

    rupturePredict = rp.RupturePredict()
    for event_id in df_hypos['event_id']:
        df_event = df_hypos[df_hypos['event_id'] == event_id]
        print str(event_id),
        for process_type, pkl_file_root in [ \
                                            ('ff_duration', '/Users/anthony/work/early-est/tsunamiLearn/out/regressors/ff_duration_ElasticNet_a_0.1_l1r_0.1'), \
                                            ('ff_rupture_area', '/Users/anthony/work/early-est/tsunamiLearn/out/regressors/ff_rupture_area_ElasticNet_a_0.1_l1r_0.1'), \
                                            ('At', '/Users/anthony/work/early-est/tsunamiLearn/out/regressors/At_ElasticNet_a_0.001_l1r_0.95'), \
                                            ('It', '/Users/anthony/work/early-est/tsunamiLearn/out/regressors/It_ElasticNet_a_0.01_l1r_0.1'), \
                                            ]:
            ok1 = True
            if (df_event.iloc[0]['ff_duration'] < 0.0):
                ok1 = False
            ok2 = True
            if (isnan(df_event.iloc[0]['At'])):
                ok2 = False
            if (not ok1 and not ok2):
                continue
            if ((process_type == 'ff_duration' or process_type == 'ff_rupture_area') and not ok1 or (process_type == 'At') and not ok2):
                print 'NaN  NaN',
                continue
            prediction = rupturePredict.predict(process_type, hypolist_file, event_id, pkl_file_root, P_underwater, Ndip)
            print str(df_event.iloc[0][process_type])  + ' ' + str(prediction),
        print ''
    

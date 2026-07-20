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

# 20210916 AJL - converted to Python 3(3.8)

__author__ = 'anthony'
__date__ = '$Jul 06, 2016$'

import sys

try:
    import pandas as pd
    import joblib
except ImportError as importerr:
    print('Warning: ' + importerr.__class__.__name__ + ": " + str(importerr) + '\n\t' \
        + 'Advanced Python packages are required for Early-est Rupture Prediction:' + '\n\t' \
        + 'Pandas http://pandas.pydata.org/pandas- docs/stable/install.html' + '\n\t' \
        + 'scikit-learn http://scikit-learn.org/stable/install.html' + '\n\t' \
        + 'Python and these packages can be easily installed with www.anaconda.com/download')
    #sys.exit()


#col_names_use = ['depth(km)', 'T50Ex', 'Td(sec)', 'TdT50Ex', 'Mwp', 'T0(sec)', 'TdT0', 'Mwpd', 'P_underwater', 'Ndip']


class RupturePredict(object):


    def predict(self, process_type, hypolist_file, event_id, pkl_file_root, P_underwater, Ndip):
        
        try:
            
            # open hypo list file, find event
            if (hypolist_file.endswith('.csv')):
                hypos_file = open(hypolist_file)
                df_hypos = pd.read_csv(hypos_file)
            else:
                # assume Early-est .csv / .csv.hdr pair
                hdr_file = open(hypolist_file + '.csv.hdr')
                columns = hdr_file.read().split()
                # 20210916 AJL - Python3 / Pandas upgrade: names: Duplicates in this list are not allowed.
                #print("BEFORE:", columns)
                for ndx in range(len(columns)):
                    nduplicate = 0
                    for ndx2 in range(ndx + 1, len(columns)):
                        if columns[ndx] == columns[ndx2]:
                            nduplicate += 1
                            columns[ndx2] = columns[ndx2] + str(nduplicate)
                #print("AFTER:", columns)
                            
                # END - # 20210916 AJL
                hypos_file = open(hypolist_file + '.csv')
                df_hypos = pd.read_csv(hypos_file, names = columns, delim_whitespace = True)
            if (P_underwater != None) :
                df_hypos['P_underwater'] = P_underwater
            if (Ndip != None) :
                df_hypos['Ndip'] = Ndip
            df_hypos['TdT0'] = df_hypos['Td(sec)'] * df_hypos['T0(sec)']
            
            # get saved regressor etc.
            try:
            	clf = joblib.load(pkl_file_root +  '.regressor.pkl')
            	scaler = joblib.load(pkl_file_root +  '.scaler.pkl')
            	col_names_use = joblib.load(pkl_file_root +  '.col_names_use.pkl')
            except Exception as exception:
            	print("error: loading RupturePredict pickle files:", pkl_file_root + '.*'  + str(exception))
            	return(-1)
    
            event_id = int(event_id)
            df_hypos = df_hypos[df_hypos['event_id'] == event_id]
            df_hypo_data = df_hypos[col_names_use]
            
            # apply prediction
            # sort so column order matches that of pkl'd regressor
            df_hypo_data = df_hypo_data.reindex(sorted(df_hypo_data), axis=1)
            df_hypo_data = scaler.transform(df_hypo_data)
            if False:
                print('clf.predict(df_hypo_data)')
                print(df_hypo_data)
                print('===================')
            prediction = clf.predict(df_hypo_data)
        
        except Exception as exception:
            print(exception.__class__.__name__ + ": " + str(exception))
            prediction = [-1]

        return(prediction[0])
        



if __name__ == '__main__':
    import sys
    
    if (len(sys.argv) < 5):
        print('Usage: ' + sys.argv[0] + ' process_type[ff_rupture_area|ff_duration] pkl_file_root hypolist_file  event_id [P_underwater [Ndip]]')
        print('   hypolist_file is *.csv file containing all needed features [P_underwater & Ndip optional]')
        print('      or root path/name of Early-est *.csv/*.csv.hdr pair [P_underwater & Ndip required]')
        sys.exit()
    
    #print 'DEBUG: sys.argv:', 
    #for n in range(len(sys.argv)):
    #    print(sys.argv[n])
    process_type = sys.argv[1]
    pkl_file_root = sys.argv[2]
    hypolist_file = sys.argv[3]
    event_id = sys.argv[4]
    P_underwater =  None
    if (len(sys.argv) > 5):
        P_underwater =  sys.argv[5]
    Ndip =  None
    if (len(sys.argv) > 6):
        Ndip =  sys.argv[6]
    
    rp = RupturePredict()
    prediction = rp.predict(process_type, hypolist_file, event_id, pkl_file_root, P_underwater, Ndip)
    
    print('Prediction: ' + process_type + ': ' + str(prediction))
    

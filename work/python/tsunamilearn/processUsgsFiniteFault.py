#!/usr/bin/env python
# encoding: utf-8
'''
processUsgsFiniteFault -- shortdesc

processUsgsFiniteFault is a description

It defines classes_and_methods

@author:     user_name

@copyright:  2016 organization_name. All rights reserved.

@license:    license

@contact:    user_email
@deffield    updated: Updated
'''

import sys
import os

from optparse import OptionParser
from datetime import datetime

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt


__all__ = []
__version__ = 0.1
__date__ = '2016-06-16'
__updated__ = '2016-06-16'

DEBUG = 0
TESTRUN = 0
PROFILE = 0

global verbose
verbose = 0


class ProcessUsgsFiniteFault(object):
        
    
    def readQueryList(self, ffdb_inpath):
        
        query_file = ffdb_inpath + '/query.csv'
        df_query = pd.read_csv(query_file, index_col = False)
        
        # time,latitude,longitude,depth,mag,magType,nst,gap,dmin,rms,net,id,updated,place,type,\
        #   horizontalError,depthError,magError,magNst,status,locationSource,magSource

        if (verbose > 1):
            print(df_query[['time', 'latitude', 'longitude', 'depth', 'mag', 'magType', 'id', 'place']])
            
        return df_query


    def mergeIntoEEcsv(self, df_finite_fault, mergefile, tolerance):
        
        #df_merginto = pd.read_csv(mergefile, delim_whitespace=True, index_col = False)  # direct EE output
        df_merginto = pd.read_csv(mergefile, dtype={'event_id': str})        # ouput merge file from tsunamiLearn.py
        new_columns = df_merginto.columns.values.tolist()
        new_columns.append('ff_duration')
        new_columns.append('ff_rupture_area')
        dfmerged = pd.DataFrame(columns = new_columns)
        
        for index_m, row_m in df_merginto.iterrows():
            datestrs = row_m['otime(UTC)'].rsplit('.', 1)
            date_m = datetime.strptime(datestrs[0], '%Y.%m.%d-%H:%M:%S')
            row_m['ff_duration'] = -1
            row_m['ff_rupture_area'] = -1
            for index_ff, row_ff in df_finite_fault.iterrows():
                datestrs = row_ff['time'].rsplit('.', 1)
                date_ff = datetime.strptime(datestrs[0], '%Y.%m.%d-%H:%M:%S')
                dt = date_ff - date_m
                if (abs(dt.total_seconds()) < tolerance):
                    row_m['ff_duration'] = row_ff['duration']
                    row_m['ff_rupture_area'] = row_ff['rupture_area']
            dfmerged = dfmerged.append(row_m)
                    
        return dfmerged


    def processMomentRateFunction(self, ffdb_inpath, event_code, integral_cutoff, df_query):
        
        mr_file = ffdb_inpath + '/mr/' + event_code + '.mr'
        try:
            df_mr = pd.read_csv(mr_file, delim_whitespace=True, index_col = False, names = ['time', 'moment_rate'])
        except IOError as e:
            print(str(e))
            return -1.0, -1.0, -1.0, -1.0
        
        if (verbose > 1):
            print(df_mr)
            
        # integrate moment rate and find cutoff
        dt = df_mr['time'][1] - df_mr['time'][0]
        integral = df_mr['moment_rate'].sum() * dt
        if (verbose > 1):
            print(df_query[['time', 'mag', 'magType', 'id', 'place']],)
            print('\nintegral: ' + str(integral) + ', Mw: ' + str((2.0/3.0) * np.log10(integral) - 10.7))
        cutoff = integral * integral_cutoff
        peak_mr = df_mr['moment_rate'].max()         # 20170327 AJL - added determination of peak moment rate time delay from start
        mr_cutoff = 0.5 * peak_mr          # 20170327 AJL - added determination of peak moment rate time delay from start
        duration = 0.0
        integral = 0.0
        # 20170327 AJL - added determination of peak moment rate time delay from start
        peak_mr = -1.0
        mr_time_peak = -1.0
        mr_time_begin = -1.0
        mr_time_end = -1.0
        #
        mr_time = 0.0
        for mrval in df_mr['moment_rate']:
            integral += mrval * dt
            if (integral < cutoff):
                duration = mr_time
            if (mrval > peak_mr):
                peak_mr = mrval
                mr_time_peak = mr_time
            if (mr_time_begin < 0.0 and mrval >= mr_cutoff):
                mr_time_begin = mr_time
            if (mrval >= mr_cutoff):
                mr_time_end = mr_time
            mr_time += dt
        if (verbose > 1):
            print('duration: ' + str(duration))
            print('mr_time_peak: ' + str(mr_time_peak))
            print('mr_time_begin: ' + str(mr_time_begin))
            print('mr_time_end: ' + str(mr_time_end))

        return duration, mr_time_peak, mr_time_begin, mr_time_end
        

    def processFiniteFault(self, ffdb_inpath, event_code, duration, df_query):
        
        fsp_file = ffdb_inpath + '/fsp/' + event_code + '.fsp'
        
        nx = 0
        nz = 0
        dx = 0.0
        dz = 0.0
        try:
            for line in open(fsp_file):
                if (line.find("Invs")) > -1:
                    if line.find("Nx") > -1:
                        tokens = line[line.find("Nx"):].split()
                        nx = int(tokens[2])
                    if line.find("Nz") > -1:
                        tokens = line[line.find("Nz"):].split()
                        nz = int(tokens[2])
                    if line.find("Dx") > -1:
                        tokens = line[line.find("Dx"):].split()
                        dx = float(tokens[2])
                    if line.find("Dz") > -1:
                        tokens = line[line.find("Dz"):].split()
                        dz = float(tokens[2])
        except IOError as e:
            print(str(e))
            return -1.0
        
        try:
            df_fsp = pd.read_csv(fsp_file, delim_whitespace=True, index_col = False, comment='%', \
                                 names = ['LAT', 'LON', 'X', 'Y', 'Z', 'SLIP', 'RAKE', 'TRUP', 'RISE'], \
                                 )
        except IOError as e:
            print(str(e))
            return -1.0
        
        if (verbose > 1):
            print(df_fsp)
            
        # integrate slip and find cutoff
        dA = dx * dz
        df_tmp = df_fsp[df_fsp['TRUP'] <= duration]
        rupture_area = len(df_tmp) * dA
        if (verbose > 1):
            print(df_query[['time', 'mag', 'magType', 'id', 'place']],)
            print('\nrupture_area: ' + str(rupture_area) + ', fault_area: ' + str(nx * nz * dA))

        return rupture_area
        



def main(argv=None):
    '''Command line options.'''

    program_name = os.path.basename(sys.argv[0])
    program_version = "v0.1"
    program_build_date = "%s" % __updated__

    program_version_string = '%%prog %s (%s)' % (program_version, program_build_date)
    #program_usage = '''usage: spam two eggs''' # optional - will be autogenerated by optparse
    program_longdesc = '''''' # optional - give further explanation about what the program does
    program_license = "Copyright 2016 user_name (organization_name)                                            \
                Licensed under the Apache License 2.0\nhttp://www.apache.org/licenses/LICENSE-2.0"

    if argv is None:
        argv = sys.argv[1:]
    
    #try:
    # setup option parser
    parser = OptionParser(version=program_version_string, epilog=program_longdesc, description=program_license)
    parser.add_option("-i", "--in", dest="infile", help="set input USGS finite fault file db [default: %default]", metavar="FILE")
    parser.add_option("-o", "--out", dest="outpath", help="set output path [default: %default]", metavar="FILE")
    parser.add_option("-m", "--merge", dest="mergefile", help="set mergefile [default: %default]", metavar="FILE")
    parser.add_option("-v", "--verbose", dest="verbose", action="count", help="set verbosity level [default: %default]")
    parser.add_option("-p", "--plot", dest="plot", help="plot results [default: %default]")

    # set defaults
    parser.set_defaults(outpath="./out", infile="./in.csv", mergefile = None)

    # process options
    (opts, args) = parser.parse_args(argv)

    global verbose
    verbose = 0
    plot_it = False
    if opts.verbose > 0:
        verbose = opts.verbose
        if (verbose > 0):
            print("verbosity level = %d" % opts.verbose)
    if opts.infile:
        if (verbose > 0):
            print("infile = %s" % opts.infile)
        ffdb_inpath = opts.infile
    if opts.mergefile:
        if (verbose > 0):
            print("mergefile = %s" % opts.mergefile)
        mergefile = opts.mergefile
    if opts.outpath:
        if (verbose > 0):
            print("outpath = %s" % opts.outpath)
        csv_outpath = opts.outpath
    if opts.plot:
        if (verbose > 0):
            print("plot = %s" % opts.plot)
        plot_it = True


    # apply
    processUsgsFiniteFault = ProcessUsgsFiniteFault()
    
    # read query file to df
    df_query = processUsgsFiniteFault.readQueryList(ffdb_inpath)
    
    # process duration
    # initialize output df's
    df_finite_fault = pd.DataFrame(columns = ['time', 'latitude', 'longitude', 'depth', 'mag', 'magType', 'id', 'duration', 'rupture_area', 'mr_time_peak', 'mr_time_begin', 'mr_time_end'])
    # loop over events
    integral_cutoff = 0.95
    for event_id in df_query['id']:
        event_code = event_id[2:]
        if (verbose > 1):
            print('processing: ' + event_id + ' (' + event_code + ')')
        duration, mr_time_peak, mr_time_begin, mr_time_end = processUsgsFiniteFault.processMomentRateFunction(ffdb_inpath, event_code, integral_cutoff, df_query.loc[df_query['id'] == event_id])
        newrow = df_query[df_query['id'] == event_id]
        newrow = newrow[['time', 'latitude', 'longitude', 'depth', 'mag', 'magType', 'id']]
        newrow['duration'] = duration
        newrow['mr_time_peak'] = mr_time_peak
        newrow['mr_time_begin'] = mr_time_begin
        newrow['mr_time_end'] = mr_time_end
        df_finite_fault = pd.concat([df_finite_fault, newrow])
        if (verbose > 1):
            print('\n')
    if (verbose > 0):
        if (verbose > 1):
            with pd.option_context('display.max_rows', 999, 'display.max_columns', 999):
                print(df_finite_fault[['time', 'mag', 'magType', 'id', 'duration', 'mr_time_peak', 'mr_time_begin', 'mr_time_end']],)
                print('\n')
        df_tmp =  df_finite_fault[df_finite_fault['duration'] > 100.0]
        print(df_tmp[['time', 'mag', 'magType', 'id', 'duration']],)
        print('\n')
        if plot_it:
            df_finite_fault.plot(kind='scatter', x='mag', y='duration')
            plt.show()
            df_finite_fault.plot(kind='scatter', x='mag', y='mr_time_peak')
            plt.show()

    # process finite faulting
    # loop over events
    integral_cutoff = 0.95
    for event_id in df_finite_fault['id']:
        event_code = event_id[2:]
        if (verbose > 1):
            print('processing: ' + event_id + ' (' + event_code + ')')
        df_tmp = df_finite_fault.loc[df_query['id'] == event_id]
        duration = df_tmp.iloc[0]['duration']
        rupture_area = processUsgsFiniteFault.processFiniteFault(ffdb_inpath, event_code, duration, df_query.loc[df_query['id'] == event_id])
        df_finite_fault.loc[df_finite_fault['id'] == event_id, 'rupture_area'] = rupture_area
        if (verbose > 1):
            print('\n')
    if (verbose > 0):
        with pd.option_context('display.max_rows', 999, 'display.max_columns', 999):
            print(df_finite_fault[['time', 'mag', 'magType', 'id', 'duration', 'rupture_area']],)
            print('\n')
        df_tmp =  df_finite_fault[df_finite_fault['duration'] > 100.0]
        with pd.option_context('display.max_rows', 999, 'display.max_columns', 999):
            print(df_tmp[['time', 'mag', 'magType', 'id', 'duration', 'rupture_area']],)
            print('\n')
        if plot_it:
            df_finite_fault.plot(kind='scatter', x='rupture_area', y='duration')
            plt.show()

    # convert date/time to EE format
    s_tmp = df_finite_fault['time'].str.replace('-', '.')
    s_tmp = s_tmp.str.replace('Z', '')
    df_finite_fault['time'] = s_tmp.str.replace('T', '-')
    
    # write output csv file
    out_file = csv_outpath + '/' + ffdb_inpath.replace('/', '_') + '_finite_fault.csv'
    df_finite_fault.to_csv(out_file)
    
    # merge into EE csv file
    tolerance = 30  #seconds
    if (mergefile != None):
        df_merge = processUsgsFiniteFault.mergeIntoEEcsv(df_finite_fault, mergefile, tolerance)
        # write output csv file
        out_file = csv_outpath + '/' + ffdb_inpath.replace('/', '_') + '_ee_merge_finite_fault.csv'
        df_merge.to_csv(out_file)


if __name__ == "__main__":
    if DEBUG:
        sys.argv.append("-h")
    if TESTRUN:
        import doctest
        doctest.testmod()
    if PROFILE:
        import cProfile
        import pstats
        profile_filename = 'processUsgsFiniteFault_profile.txt'
        cProfile.run('main()', profile_filename)
        statsfile = open("profile_stats.txt", "wb")
        p = pstats.Stats(profile_filename, stream=statsfile)
        stats = p.strip_dirs().sort_stats('cumulative')
        stats.print_stats()
        statsfile.close()
        sys.exit(0)
    sys.exit(main())
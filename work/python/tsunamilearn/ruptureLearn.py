#!/usr/bin/env python
# encoding: utf-8
'''
ruptureLearn -- shortdesc

ruptureLearn is a description

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

from operator import itemgetter

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

from sklearn import linear_model
from sklearn import svm
from sklearn import neighbors
from sklearn import ensemble

from sklearn.dummy import DummyRegressor

import sklearn.metrics as metrics

from sklearn.model_selection import train_test_split
from sklearn import preprocessing
import joblib


__all__ = []
__version__ = 0.1
__date__ = '2016-06-09'
__updated__ = '2016-06-09'

DEBUG = 0
TESTRUN = 0
PROFILE = 0

global verbose
verbose = 0

global COL_BACKGROUND
#COL_BACKGROUND = (0.2, 0.4, 0.2)
COL_BACKGROUND = 'black'

# event event_id assoc_ndx ph_assoc ph_used dmin(deg) gap1(deg) gap2(deg) atten sigma_otime(sec) otime(UTC) 
#   lat(deg) lon(deg) errH(km) depth(km) errZ(km) Q T50Ex n Td(sec) n TdT50Ex WL_col mb n Mwp n T0(sec) n Mwpd n 
#   region n_sta_tot n_sta_active assoc_latency 
#   P_underwater P_underwater_std is_over_slab n_slab slab_name slab_depth plate_bdy_list
global col_names_read
col_names_read = ['atten', 'otime(UTC)', 'lat(deg)', 'lon(deg)', 'depth(km)', 'errZ(km)', 'Q', \
                'T50Ex', 'n', 'Td(sec)', 'n', 'TdT50Ex', 'mb', 'n', 'Mwp', 'n', 'T0(sec)', 'n', 'Mwpd', 'n', \
                'region', \
                'P_underwater', 'P_underwater_std', 'slab_depth', 'plate_bdy_list', 'Ndip', \
                'It', 'At', 'ff_duration', 'ff_rupture_area']

global col_names_use
#col_names_use = ['lat(deg)', 'lon(deg)', 'depth(km)', 'T50Ex', 'Td(sec)', 'TdT50Ex', 'mb', 'Mwp', 'T0(sec)', 'Mwpd', 'P_underwater', 'slab_depth', 'min_sub_dist', 'Ndip']
#col_names_use = ['lat(deg)', 'lon(deg)', 'depth(km)', 'T50Ex', 'Td(sec)', 'TdT50Ex', 'mb', 'Mwp', 'T0(sec)', 'TdT0', 'Mwpd', 'P_underwater', 'min_sub_dist', 'Ndip']
#col_names_use = ['T50Ex', 'Td(sec)', 'TdT50Ex', 'Mwp', 'T0(sec)', 'Mwpd', 'P_underwater', 'min_sub_dist', 'Ndip']
#col_names_use = ['depth(km)', 'TdT50Ex', 'Mwp', 'P_underwater', 'min_sub_dist']
#col_names_use = ['TdT50Ex', 'Mwp', 'P_underwater', 'min_sub_dist', 'Ndip']
#col_names_use = ['TdT50Ex', 'Mwp', 'P_underwater', 'min_sub_dist']
#col_names_use = ['TdT50Ex', 'Mwp', 'P_underwater', 'Ndip']
#col_names_use = ['TdT50Ex', 'Mwp', 'P_underwater']
#col_names_use = ['TdT50Ex', 'P_underwater', 'min_sub_dist', 'Ndip']
#col_names_use = ['TdT50Ex', 'P_underwater']
#col_names_use = ['Mwp', 'P_underwater']
#col_names_use = ['TdT50Ex']
#col_names_use = ['TdT50Ex', 'Mwp']
#col_names_use = ['TdT0', 'Mwpd', 'P_underwater']
#col_names_use = ['TdT0', 'Mwpd', 'min_sub_dist']
#col_names_use = ['TdT0', 'Mwpd', 'min_sub_dist', 'Ndip']
#col_names_use = ['Td(sec)', 'T0(sec)', 'Mwpd', 'P_underwater', 'min_sub_dist', 'Ndip']
#col_names_use = ['Td(sec)', 'T0(sec)', 'Mwpd', 'P_underwater', 'min_sub_dist']
#col_names_use = ['Td(sec)', 'T0(sec)', 'TdT0', 'Mwpd', 'min_sub_dist', 'Ndip']
#col_names_use = ['Td(sec)', 'T0(sec)', 'Mwpd', 'min_sub_dist', 'Ndip']
#col_names_use = ['Td(sec)', 'T0(sec)', 'Mwpd']
#col_names_use = ['Td(sec)', 'T0(sec)']
#col_names_use = ['TdT50Ex', 'TdT0', 'Mwpd']
#
# best
col_names_use = ['depth(km)', 'T50Ex', 'Td(sec)', 'TdT50Ex', 'Mwp', 'T0(sec)', 'TdT0', 'Mwpd', 'P_underwater', 'Ndip']


global col_names_description
col_names_description = col_names_use + ['otime(UTC)', 'region']


class RuptureLearn(object):
    

    def plotScatterMatrix(self, df_data, df_target, target_type, plot_to_file, do_log10, outpath):
        
        from pandas.plotting import scatter_matrix
    
        plt.rcParams.update({'figure.facecolor': 'white'})
        plt.rcParams.update({'axes.facecolor': COL_BACKGROUND})
        plt.rcParams.update({'axes.edgecolor': 'white'})
    
        df_select = pd.DataFrame(df_data, columns=col_names_use)
        df_select = pd.concat([df_target, df_select], axis=1)
        if (verbose > 0):
            print(df_select)
        
        # remove negative magnitudes
        try:
            df_select['mb'] = np.where(df_select['mb'] < 0, np.nan, df_select['mb'])
        except:
            pass
        try:
            df_select['Mwp'] = np.where(df_select['Mwp'] < 0, np.nan, df_select['Mwp'])
        except:
            pass
        try:
            df_select['Mwpd'] = np.where(df_select['Mwpd'] < 0, np.nan, df_select['Mwpd'])
        except:
            pass
    
        if (do_log10):
            df_select=df_select.rename(columns = {target_type: 'log10_'+ target_type})
    
        #color_wheel = {0: COL_TRUE, 1: COL_PHANTOM, }
        #colors = df[target_type].map(lambda x: color_wheel.get(x))
        #scatter_matrix(df_select, s=100, alpha=0.8, color=df_color, figsize=(17, 17), diagonal='kde')
        scatter_matrix(df_select, s=20, alpha=0.8, color='white', figsize=(17, 17), diagonal='kde')
        plt.rcParams.update({'font.size': 12})
        if (plot_to_file):
            logtxt = ''
            if (do_log10):
                logtxt = 'log10_'
            col_txt = ''
            for col in col_names_use:
                col_txt += col + '.'
            plt.savefig(outpath + '/plt/regression_' + logtxt + target_type + '_scatter_' + col_txt + 'png', bbox_inches='tight')
        else:
            plt.show()
    
    
    def createDfHyposTarget(self, hypos_file, target_type, do_log10):
        
        df_hypos_read = pd.read_csv(hypos_file, usecols = col_names_read)
        
        df_hypos_read['TdT0'] = df_hypos_read['Td(sec)'] * df_hypos_read['T0(sec)']
        
        min_sub_dist_list = []
        for index in range(len(df_hypos_read)):
            boundaries = df_hypos_read.iloc[index]['plate_bdy_list']
            min_sub_dist = -1.0
            if (not pd.isnull(boundaries)):
                for boundary in boundaries.split(';'):
                    tokens = boundary.split('|')
                    if (len(tokens) == 3):
                        bdy_type = tokens[2]
                        dist = float(tokens[0])
                        if (bdy_type == 'SUB' and (min_sub_dist < 0.0 or dist < min_sub_dist)):
                            min_sub_dist = dist
            min_sub_dist_list.append(min_sub_dist)
        df_hypos_read['min_sub_dist'] = min_sub_dist_list
        
        #df_hypos = df_hypos_read
        df_hypos = df_hypos_read[df_hypos_read[target_type] > 0.0]
        
        #if (target_type == 'At'):
            #df_hypos = df_hypos[df_hypos_read[target_type] <= 2.0]
        
        if (do_log10):
            df_hypos[target_type] = np.log10(df_hypos[target_type])
        
        if (verbose > 1):
            print(df_hypos)
        pd.options.display.float_format = '{:,.1f}'.format
        print(df_hypos[['otime(UTC)', 'Mwpd', 'region', target_type]])
        print('')
        
        df_target = df_hypos.loc[ : , target_type]

            
        return df_hypos, df_target


    def doLearnPredcitRegress(self, df_data, df_target, target_type, rand_seed, scaler, results_dict, qualities, outpath):
        
        np.random.seed(rand_seed)
        
        df_data_use = df_data[col_names_use]
        # sort so column order is know for later prediction using pkl regressor
        df_data_use = df_data_use.reindex(sorted(df_data_use), axis=1)
        if scaler != None:
            df_data_use = scaler.fit_transform(df_data_use)

        df_train_x, df_test_x, df_train_y, df_test_y = train_test_split(df_data_use, df_target, test_size = 0.2, random_state = rand_seed)
        #df_test_description = pd.DataFrame(df_test, columns=col_names_description)

        clfs = []
        names = []
        #
        for alpha in [1.0, 0.1, 0.01, 0.001]:
            # Ridge
            clfs.append(linear_model.Ridge(alpha = alpha))
            names.append(clfs[len(clfs) - 1].__class__.__name__ + '_alpha_' + str(alpha))
            # Lasso
            clfs.append(linear_model.Lasso(alpha = alpha))
            names.append(clfs[len(clfs) - 1].__class__.__name__ + '_alpha_' + str(alpha))
            # ElasticNet
            for l1_ratio in [.1, .5, .7, .9, .95, .99, 1]:
                clfs.append(linear_model.ElasticNet(alpha = alpha, l1_ratio = l1_ratio, fit_intercept = True))
                names.append(clfs[len(clfs) - 1].__class__.__name__ + '_a_' + str(alpha) + '_l1r_' + str(l1_ratio))
            # Bayesian Ridge Regression
            paramBR = 0.001 * alpha
            clfs.append(linear_model.BayesianRidge(alpha_1=paramBR, alpha_2=paramBR, lambda_1=paramBR, lambda_2=paramBR))
            names.append(clfs[len(clfs) - 1].__class__.__name__ + '_paramBR_' + str(paramBR))
            
        # Neighbours Classifiers
        for n_neighbors in [1, 2, 3, 4, 5, 10]:
            clfs.append(neighbors.KNeighborsRegressor(n_neighbors, weights='uniform'))
            names.append(clfs[len(clfs) - 1].__class__.__name__ + '_uniform_' + str(n_neighbors))
            clfs.append(neighbors.KNeighborsRegressor(n_neighbors, weights='distance'))
            names.append(clfs[len(clfs) - 1].__class__.__name__ + '_distance_' + str(n_neighbors))
            
        # GradientBoostingRegressor
        clfs.append(ensemble.GradientBoostingRegressor())
        names.append(clfs[len(clfs) - 1].__class__.__name__)

        # GradientBoostingRegressor
        clfs.append(linear_model.LinearRegression())
        names.append(clfs[len(clfs) - 1].__class__.__name__)
      
        # Support Vector Regression
        clfs.append(svm.SVR())
        names.append(clfs[len(clfs) - 1].__class__.__name__)
        clfs.append(svm.NuSVR())
        names.append(clfs[len(clfs) - 1].__class__.__name__)
        clfs.append(svm.LinearSVR())
        names.append(clfs[len(clfs) - 1].__class__.__name__)

        # Dummy Regressor for reference
        clfs.append(DummyRegressor())
        names.append(clfs[len(clfs) - 1].__class__.__name__)

        table_output = []
        if (qualities == None):
            qualities = [-1.0] * len(clfs)
        nclf = 0
        for clf in clfs:
            clf.fit(df_train_x, df_train_y)
            # print results
            prediction = clf.predict(df_test_x)
            if False:
                print('clf.predict(df_test_x)')
                print(df_test_x)
                print('===================')
            #quality = clf.score(df_test_x, df_test_y)
            #quality = 1.0 - metrics.mean_squared_error(df_test_y, prediction) /  (np.dot(df_test_y, df_test_y) / len(df_test_y))
            #quality = 1.0 - metrics.mean_absolute_error(df_test_y, prediction) / np.linalg.norm(df_test_y, 1)
            ##quality = 1.0 - metrics.median_absolute_error(df_test_y, prediction) / np.linalg.norm(df_test_y, 1)
            quality = metrics.r2_score(df_test_y, prediction)
            #
            name_padded = (names[nclf] + (' ' * 35))[:35]
            table_output.append( {'nclf' : nclf, 'name' : name_padded, 'quality' : quality} )
            #
            results_df = pd.DataFrame(columns = ['prediction', 'target'])
            dict_tmp = {'prediction' : prediction, 'target' : df_test_y}
            results_tmp = pd.DataFrame(data = dict_tmp)
            try:
                results_df = results_dict[name_padded]
            except (KeyError, IndexError, ):
                results_df = pd.DataFrame(columns = ['prediction', 'target'])
            results_df = results_df.append(results_tmp)
            results_dict[name_padded] = results_df
            # check if best quality so far for this classifier
            if (quality > qualities[nclf]):
                qualities[nclf] = quality
                # save classifier to disk
                filename = outpath + '/regressors/' + target_type + '_' + names[nclf]
                joblib.dump(clf, filename +  '.regressor.pkl', compress=9)
                joblib.dump(scaler, filename +  '.scaler.pkl', compress=9)
                joblib.dump(col_names_use, filename +  '.col_names_use.pkl', compress=9)
            #
            nclf += 1
            
        sorted_table = sorted(table_output, key=itemgetter('quality'), reverse = True)

        # print test/target data
        if (verbose > 0):
            name_padded = ('name' + (' ' * 35))[:35]
            print('n\t' + name_padded + '\tquality')
            for table_entry in sorted_table:
                print(table_entry['nclf'], '\t', table_entry['name'], '\t', ("%.2f" %  table_entry['quality']))
            
        return sorted_table, results_dict, qualities
    
    def plotResults(self, results_dict, sorted_summary_list, target_type, scaler_name, do_log10, plot_to_file, outpath):
    
        ncol=4
        nrow=4
        
        plt.style.use('ggplot')
        plt.rcParams.update({"figure.figsize" : [17, 17]})
        plt.rcParams.update({'font.size': 10})

        fig, axes = plt.subplots(ncols=ncol, nrows=nrow)
        plt.subplots_adjust(left=0.05, right=0.99, top=0.95, bottom=0.05)
        
        col_txt = '.'
        for col in col_names_use:
            col_txt += col + '.'

        fig.suptitle(target_type + ': ' +  scaler_name + ': ' + col_txt, fontsize=12)
        
        i = 0
        for n in range(len(axes)):
            for m in range(len(axes[n])):
                axes[n, m].axis('equal')
                if (n < nrow - 1):   # plot best results
                    summary_list = sorted_summary_list[i]
                else:   # plot worst results
                    summary_list = sorted_summary_list[len(sorted_summary_list) - (ncol * nrow - i)]
                i += 1
                results_df = results_dict[summary_list['name']]
                if (do_log10):
                    results_df['target'] = np.power(10.0, results_df['target'])
                    results_df['prediction'] = np.power(10.0, results_df['prediction'])
                results_df.plot.scatter(ax = axes[n, m], x = 'target', y = 'prediction')
                axes[n, m].set_title(summary_list['name'].strip()  + ': q=' + ("%.2f" %  (summary_list['quality'])), fontsize =12)
                
        if (plot_to_file):
            logtxt = ''
            if (do_log10):
                logtxt = '_log10'
            plt.savefig(outpath + '/plt/' + target_type + '_' +  scaler_name + logtxt + col_txt + 'png', bbox_inches='tight')
        else:
            plt.show()
    

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
    parser.add_option("-i", "--in", dest="infile", help="set input EE-finite-fault csv file [default: %default]", metavar="FILE")
    parser.add_option("-t", "--target_type", dest="target_type", help="set target rupture measure [default: %default]")
    parser.add_option("-o", "--out", dest="outpath", help="set output path [default: %default]", metavar="FILE")
    parser.add_option("-v", "--verbose", dest="verbose", default=0, action="count", help="set verbosity level [default: %default]")

    # set defaults
    parser.set_defaults(outpath="./out", infile="./in.csv")
    parser.set_defaults(target_type="ff_rupture_area")

    # process options
    (opts, args) = parser.parse_args(argv)

    global verbose
    verbose = 0
    if opts.verbose > 0:
        verbose = opts.verbose
        if (verbose > 0):
            print("verbosity level = %d" % opts.verbose)
    if opts.infile:
        if (verbose > 0):
            print("infile = %s" % opts.infile)
        hypos_file = opts.infile
    if opts.target_type:
        if (verbose > 0):
            print("target_type = %s" % opts.target_type)
        target_type = opts.target_type
    if opts.outpath:
        if (verbose > 0):
            print("outpath = %s" % opts.outpath)
        outpath = opts.outpath
    try:
        os.makedirs(outpath + '/plt')
    except:
        pass
    try:
        os.makedirs(outpath + '/regressors')
    except:
        pass

    # log10
    do_log10 = False
    
    plot_to_file = True

    # apply
    ruptureLearn = RuptureLearn()
    df_hypos, df_target = ruptureLearn.createDfHyposTarget(hypos_file, target_type, do_log10)

    # scatter plot
    ruptureLearn.plotScatterMatrix(df_hypos, df_target, target_type, plot_to_file, do_log10, outpath)
    
    # set scaler
    scaler = None
    scaler_name = 'none'
    if True:
        scaler = preprocessing.MinMaxScaler(feature_range=(0, 1))
        scaler_name = 'MinMaxScaler'
    if False:
        scaler = preprocessing.StandardScaler()
        scaler_name = 'StandardScaler'
    if False:
        scaler = preprocessing.RobustScaler()
        scaler_name = 'RobustScaler'
        
    results_dict = {}
    qualities = None
    summary_list = None
    num_tests = 1000
    for n in range(num_tests):
        rand_seed = 1000 + n * n
        print('n=' + str(n) + '/' + str(num_tests) + ', rand_seed=' + str(rand_seed) + ' ======================================')
        sorted_table, results_dict, qualities = ruptureLearn.doLearnPredcitRegress(df_hypos, df_target, target_type, rand_seed, scaler, results_dict, qualities, outpath)
        for table_entry in sorted_table:
            index = int(table_entry['nclf'])
            try:
                sum_dict_entry = summary_list[index]
                summary_list[index]['quality'] = sum_dict_entry['quality'] + table_entry['quality']
            except:
                if (summary_list == None):
                    summary_list = [0]*len(sorted_table)
                summary_list[index] = {'name' : table_entry['name'], 'quality' : table_entry['quality']}
            
    sorted_summary_list = sorted(summary_list, key=lambda x: x['quality'], reverse = True)
    print('=======================================================')
    print('Summary results: ' + 'nTest:' + str(num_tests) + ' ' +  str(col_names_use)  + ' scaler:' + scaler_name + ' :')
    for n in range(len(sorted_summary_list)):
        sorted_summary_list[n]['quality'] = sorted_summary_list[n]['quality'] / num_tests
        print(n, '\t', sorted_summary_list[n]['name'], '\t', ("%.2f" %  (sorted_summary_list[n]['quality'])))

    # plot
    ruptureLearn.plotResults(results_dict, sorted_summary_list, target_type, scaler_name, do_log10, plot_to_file, outpath)


    #except Exception, e:
    #    indent = len(program_name) * " "
    #    sys.stderr.write(program_name + ": " + repr(e) + "\n")
    #    sys.stderr.write(indent + "  for help use --help")
    #    return 2


if __name__ == "__main__":
    if DEBUG:
        sys.argv.append("-h")
    if TESTRUN:
        import doctest
        doctest.testmod()
    if PROFILE:
        import cProfile
        import pstats
        profile_filename = 'ruptureLearn_profile.txt'
        cProfile.run('main()', profile_filename)
        statsfile = open("profile_stats.txt", "wb")
        p = pstats.Stats(profile_filename, stream=statsfile)
        stats = p.strip_dirs().sort_stats('cumulative')
        stats.print_stats()
        statsfile.close()
        sys.exit(0)
    sys.exit(main())
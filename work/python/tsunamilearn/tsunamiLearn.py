#!/usr/bin/env python
# encoding: utf-8
'''
tsunamiLearn -- shortdesc

tsunamiLearn is a description

It defines classes_and_methods

@author:     user_name

@copyright:  2016 organization_name. All rights reserved.

@license:    license

@contact:    user_email
@deffield    updated: Updated 20220527 ALomax to Python 3 and recent sklearn
'''

import sys
import os

from optparse import OptionParser

from operator import itemgetter

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

from sklearn import svm
from sklearn import neighbors
from sklearn import tree
from sklearn.model_selection import train_test_split
from sklearn.ensemble import AdaBoostClassifier
from sklearn.tree import DecisionTreeClassifier
from sklearn.ensemble import RandomForestClassifier
from sklearn.ensemble import ExtraTreesClassifier
from sklearn.dummy import DummyClassifier
from sklearn import preprocessing
###from sklearn.externals import joblib
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
                'P_underwater', 'P_underwater_std', 'slab_depth', 'plate_bdy_list', 'Ndip']

global col_names_use
#col_names_use = ['lat(deg)', 'lon(deg)', 'depth(km)', 'errZ(km)', 'T50Ex', 'Td(sec)', 'TdT50Ex', 'mb', 'Mwp', 'T0(sec)', 'Mwpd', 'P_underwater', 'P_underwater_std', 'slab_depth', 'min_sub_dist']
#col_names_use = ['depth(km)', 'TdT50Ex', 'Mwp', 'P_underwater', 'min_sub_dist']
#col_names_use = ['TdT50Ex', 'Mwp', 'P_underwater', 'min_sub_dist', 'Ndip']
#col_names_use = ['TdT50Ex', 'Mwp', 'P_underwater', 'Ndip']
#col_names_use = ['TdT50Ex', 'Mwp', 'P_underwater']
#col_names_use = ['TdT50Ex', 'P_underwater', 'min_sub_dist', 'Ndip']
#col_names_use = ['TdT50Ex', 'P_underwater']
#col_names_use = ['Mwp', 'P_underwater']
#col_names_use = ['TdT50Ex']
#col_names_use = ['TdT50Ex', 'Mwp']
#col_names_use = ['TdT0', 'Mwpd', 'P_underwater']
#col_names_use = ['TdT0', 'Mwpd', 'P_underwater', 'min_sub_dist']
#col_names_use = ['TdT0', 'Mwpd', 'P_underwater', 'min_sub_dist', 'Ndip']
#col_names_use = ['Mwpd']
#
col_names_use = ['TdT50Ex', 'Mwp', 'P_underwater', 'min_sub_dist']



global col_names_description
col_names_description = col_names_use + ['otime(UTC)', 'region']


class TsunamiLearn(object):
    

    def plotScatterMatrix(self, df_data, target_type, plot_to_file):
        
        from pandas.plotting import scatter_matrix
    
        plt.rcParams.update({'figure.facecolor': 'white'})
        plt.rcParams.update({'axes.facecolor': COL_BACKGROUND})
        plt.rcParams.update({'axes.edgecolor': 'white'})
    
        df_select = pd.DataFrame(df_data, columns=[target_type] + col_names_use)
        if (verbose > 0):
            print(df_select)
        
        # remove negative magnitudes
        try:
            df_select['mb'] = np.where(df_select['mb'] <= 0, np.nan, df_select['mb'])
        except:
            pass
        try:
            df_select['Mwp'] = np.where(df_select['Mwp'] <= 0, np.nan, df_select['Mwp'])
        except:
            pass
        try:
            df_select['Mwpd'] = np.where(df_select['Mwpd'] <= 0, np.nan, df_select['Mwpd'])
        except:
            pass

        #color_wheel = {0: COL_TRUE, 1: COL_PHANTOM, }
        #colors = df[target_type].map(lambda x: color_wheel.get(x))
        #scatter_matrix(df_select, s=100, alpha=0.8, color=df_color, figsize=(17, 17), diagonal='kde')
        scatter_matrix(df_select, s=20, alpha=0.8, color='white', figsize=(17, 17), diagonal='kde')
        plt.rcParams.update({'font.size': 12})
        if (plot_to_file):
            col_txt = ''
            for col in col_names_use:
                col_txt += col + '.'
            plt.savefig('plt/decision_' + target_type + '_scatter_' + col_txt + 'png', bbox_inches='tight')
        else:
            plt.show()
    
    
    def createDfHypos(self, hypos_file, target_type):
        
        df_hypos = pd.read_csv(hypos_file, index_col = False, usecols = col_names_read + [target_type])
        
        df_hypos['TdT0'] = df_hypos['Td(sec)'] * df_hypos['T0(sec)']
        
        min_sub_dist_list = []
        for index in range(len(df_hypos)):
            boundaries = df_hypos.iloc[index]['plate_bdy_list']
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
        df_hypos['min_sub_dist'] = min_sub_dist_list
        
        if (verbose > 0):
            print(df_hypos)
        print(df_hypos[['region', 'Ndip']])

        df_select = df_hypos

        # filter rows
        if (target_type == 'At'):
            # remove rows with missing At values
            #df_select = df_select[np.isfinite(df_select[target_type])]
            # set NaN to small value
            df_select[target_type] = np.where(np.isnan(df_select[target_type]), 0.01, df_select[target_type])

        if (verbose > 0):
            print(df_select)
        
        # remove negative magnitudes
        try:
            df_select['mb'] = np.where(df_select['mb'] < 0, 0.0, df_select['mb'])
        except:
            pass
        try:
            df_select['Mwp'] = np.where(df_select['Mwp'] < 0, 0.0, df_select['Mwp'])
        except:
            pass
        try:
            df_select['Mwpd'] = np.where(df_select['Mwpd'] < 0, 0.0, df_select['Mwpd'])
        except:
            pass
        try:
            df_select['slab_depth'] = np.where(np.isnan(df_select['slab_depth']), -1, df_select['slab_depth'])
        except:
            pass
        if (verbose > 0):
            print(df_select)


        return df_select



    def doLearnPredcitClassify(self, df_select, target_type, rand_seed):
        
        np.random.seed(rand_seed)

        df_train, df_test = train_test_split(df_select, test_size = 0.2, random_state = rand_seed)
        df_test_description = pd.DataFrame(df_test, columns=col_names_description)
        #
        #df_train = df_select
        #df_test = df_select
        
        df_train_x = pd.DataFrame(df_train, columns=col_names_use)
        df_test_x = pd.DataFrame(df_test, columns=col_names_use)
        df_test_ee = pd.DataFrame(df_test, columns=['TdT50Ex'])
    
        if True:
            min_max_scaler = preprocessing.MinMaxScaler(feature_range=(0, 1))
            df_train_x = min_max_scaler.fit_transform(df_train_x)
            df_test_x = min_max_scaler.fit_transform(df_test_x)
        if False:
            standard_scaler = preprocessing.StandardScaler()
            df_train_x = standard_scaler.fit_transform(df_train_x)
            df_test_x = standard_scaler.fit_transform(df_test_x)
        if False:
            robust_scaler = preprocessing.RobustScaler()
            df_train_x = robust_scaler.fit_transform(df_train_x)
            df_test_x = robust_scaler.fit_transform(df_test_x)

        train = dict(data=df_train_x, target=df_train)
        #print(train)
        test = dict(data=df_test_x, target=df_test)
        #print(test)
        

        clfs = []
        names = []
        #
        # SVM Classifiers and fit out data. We do not scale our
        # data since we want to plot the support vectors
        C = 1.0  # SVM regularization parameter
        clfs.append(svm.SVC(kernel='linear', C=C))
        names.append(clfs[len(clfs) - 1].__class__.__name__ + '_linear')
        clfs.append(svm.SVC(kernel='rbf', gamma=0.7, C=C))
        names.append(clfs[len(clfs) - 1].__class__.__name__ + '_rbf')
        clfs.append(svm.SVC(kernel='poly', degree=3, C=C))
        names.append(clfs[len(clfs) - 1].__class__.__name__ + '_poly')
        #
        # Neighbours Classifiers
        for n_neighbors in [1, 2, 3, 4, 5, 10]:
            clfs.append(neighbors.KNeighborsClassifier(n_neighbors, weights='uniform'))
            names.append(clfs[len(clfs) - 1].__class__.__name__ + '_uniform_' + str(n_neighbors))
            clfs.append(neighbors.KNeighborsClassifier(n_neighbors, weights='distance'))
            names.append(clfs[len(clfs) - 1].__class__.__name__ + '_distance_' + str(n_neighbors))
        #
        clfs.append(tree.DecisionTreeClassifier(class_weight=None, criterion='gini', max_depth=None,
            max_features=None, max_leaf_nodes=None, min_samples_leaf=1,
            min_samples_split=2, min_weight_fraction_leaf=0.0,
            random_state=None, splitter='best'))
        names.append(clfs[len(clfs) - 1].__class__.__name__)
        #
        clfs.append(AdaBoostClassifier(DecisionTreeClassifier(max_depth=1), algorithm='SAMME', n_estimators=50))
        names.append(clfs[len(clfs) - 1].__class__.__name__ + '_SAMME_50')
        clfs.append(AdaBoostClassifier(DecisionTreeClassifier(max_depth=1), algorithm='SAMME', n_estimators=100))
        names.append(clfs[len(clfs) - 1].__class__.__name__ + '_SAMME_100')
        clfs.append(AdaBoostClassifier(DecisionTreeClassifier(max_depth=1), algorithm='SAMME', n_estimators=200))
        names.append(clfs[len(clfs) - 1].__class__.__name__ + '_SAMME_200')
        clfs.append(AdaBoostClassifier(DecisionTreeClassifier(max_depth=1), algorithm='SAMME.R', n_estimators=50))
        names.append(clfs[len(clfs) - 1].__class__.__name__ + '_SAMME.R_50')
        #clfs.append(AdaBoostClassifier(DecisionTreeClassifier(max_depth=1), algorithm='SAMME.R', n_estimators=100))
        #names.append(clfs[len(clfs) - 1].__class__.__name__ + '_SAMME.R_100')
        #clfs.append(AdaBoostClassifier(DecisionTreeClassifier(max_depth=1), algorithm='SAMME.R', n_estimators=200))
        #names.append(clfs[len(clfs) - 1].__class__.__name__ + '_SAMME.R_200')
        #
        clfs.append(RandomForestClassifier(n_estimators=5, criterion='gini'))
        names.append(clfs[len(clfs) - 1].__class__.__name__ + '_gini_5')
        clfs.append(RandomForestClassifier(n_estimators=5, criterion='entropy'))
        names.append(clfs[len(clfs) - 1].__class__.__name__ + '_entropy_5')
        clfs.append(RandomForestClassifier(n_estimators=10, criterion='gini'))
        names.append(clfs[len(clfs) - 1].__class__.__name__ + '_gini_10')
        clfs.append(RandomForestClassifier(n_estimators=10, criterion='entropy'))
        names.append(clfs[len(clfs) - 1].__class__.__name__ + '_entropy_10')
        #
        #clfs.append(ExtraTreesClassifier(n_estimators=10, max_depth=None, min_samples_split=1, random_state=0))
        #names.append(clfs[len(clfs) - 1].__class__.__name__)
        clfs.append(ExtraTreesClassifier(n_estimators=5, criterion='gini'))
        names.append(clfs[len(clfs) - 1].__class__.__name__ + '_gini_5')
        clfs.append(ExtraTreesClassifier(n_estimators=5, criterion='entropy'))
        names.append(clfs[len(clfs) - 1].__class__.__name__ + '_entropy_5')
        clfs.append(ExtraTreesClassifier(n_estimators=10, criterion='gini'))
        names.append(clfs[len(clfs) - 1].__class__.__name__ + '_gini_10')
        clfs.append(ExtraTreesClassifier(n_estimators=10, criterion='entropy'))
        names.append(clfs[len(clfs) - 1].__class__.__name__ + '_entropy_10')
        #
        clfs.append(DummyClassifier(random_state = rand_seed))
        names.append(clfs[len(clfs) - 1].__class__.__name__)

        target = test['target'][target_type].values

        table_output = []
        nclf = 0
        for clf in clfs:
            clf.fit(train['data'], train['target'][target_type].values)
            # save classifier to disk
            filename = './classifiers/' + names[nclf] +  '.joblib.pkl'
            _ = joblib.dump(clf, filename, compress=9)
            # print results
            prediction = clf.predict(test['data'])
            n_true_pos = 0
            n_true_neg = 0
            n_false_neg = 0
            n_false_pos = 0
            n_target_yes = 0
            n_target_no = 0
            for n in range(len(target)):
                if (target[n] >= 1):
                    n_target_yes += 1
                    if (prediction[n] >= 1):
                        n_true_pos += 1
                    else:
                        n_false_neg += 1
                else:
                    n_target_no += 1
                    if (prediction[n] >= 1):
                        n_false_pos += 1
                    else:
                        n_true_neg += 1
            quality = 0.0
            if (False):
                quality = ((2.0 * n_true_pos + n_true_neg) - (2.0 * n_false_neg + n_false_pos)) / (2.0 * n_true_pos + n_true_neg + 2.0 * n_false_neg + n_false_pos)
            if (True):
                # results_01.txt:  quality = ((3.0 * n_true_pos + n_true_neg) - (3.0 * n_false_neg + n_false_pos)) / (3.0 * n_true_pos + n_true_neg + 3.0 * n_false_neg + n_false_pos)
                #quality = ((3.0 * n_true_pos + n_true_neg) - (3.0 * n_false_neg + n_false_pos)) / (3.0 * n_target_yes + n_target_no) # results_02.txt
                #quality = (3.0 * n_true_pos - (3.0 * n_false_neg + n_false_pos)) / (3.0 * n_target_yes) # results_02.txt
                # F1 score
                if (n_true_neg == len(target)):
                    quality = 1.0
                else:
                    quality = float(2 * n_true_pos) / float(2 * n_true_pos + n_false_pos + n_false_neg)
            else:
                quality = float(n_true_pos - (n_false_neg + n_false_pos)) / float(n_target_yes)

            #print nclf, '\t', names[nclf], '\t', n_true_pos, '\t', n_true_neg, '\t', n_false_neg, '\t', n_false_pos, '\t', quality, \
            #'\t', '"', (" ".join(str(x) for x in prediction)), '"'
            name_padded = (names[nclf] + (' ' * 35))[:35]
            table_output.append( {'nclf' : nclf, 'name' : name_padded, 'n_true_pos' : n_true_pos, 'n_true_neg' : n_true_neg, \
                                  'n_false_neg' : n_false_neg, 'n_false_pos' : n_false_pos, 'quality' : quality, 'prediction' : (" ".join(str(x) for x in prediction))} )
            #print nclf, '\t', quality, '\t', clf.__class__.__name__
            nclf += 1
            
        # standard EE TdT50Ex
        ee_predictions = []
        n_true_pos = 0
        n_true_neg = 0
        n_false_neg = 0
        n_false_pos = 0
        n_target_yes = 0
        n_target_no = 0
        for n in range(len(target)):
            ee_prediction = df_test_ee.iloc[n]['TdT50Ex'] >= 8.0
            if (target[n] >= 1):
                n_target_yes += 1
                if (ee_prediction):
                    n_true_pos += 1
                else:
                    n_false_neg += 1
            else:
                n_target_no += 1
                if (ee_prediction):
                    n_false_pos += 1
                else:
                    n_true_neg += 1
            if (ee_prediction):
                ee_predictions.append(1)
            else:
                ee_predictions.append(0)
        # results_01.txt:  quality = ((3.0 * n_true_pos + n_true_neg) - (3.0 * n_false_neg + n_false_pos)) / (3.0 * n_target_yes + n_true_neg + 3.0 * n_false_neg + n_false_pos)
        #quality = ((3.0 * n_true_pos + n_true_neg) - (3.0 * n_false_neg + n_false_pos)) / (3.0 * n_target_yes + n_target_no) # results_02.txt
        #quality = (3.0 * n_true_pos - (3.0 * n_false_neg + n_false_pos)) / (3.0 * n_target_yes) # results_02.txt
        # F1 score
        if (n_true_neg == len(target)):
            quality = 1.0
        else:
            quality = float(2 * n_true_pos) / float(2 * n_true_pos + n_false_pos + n_false_neg)
        #print nclf, '\t', names[nclf], '\t', n_true_pos, '\t', n_true_neg, '\t', n_false_neg, '\t', n_false_pos, '\t', quality, \
        #'\t', '"', (" ".join(str(x) for x in prediction)), '"'
        name_padded = ('>>> EarlyEst TdT50Ex =========' + (' ' * 35))[:35]
        table_output.append( {'nclf' : nclf, 'name' : name_padded, 'n_true_pos' : n_true_pos, 'n_true_neg' : n_true_neg, \
                                  'n_false_neg' : n_false_neg, 'n_false_pos' : n_false_pos, 'quality' : quality, 'prediction' : (" ".join(str(x) for x in ee_predictions))} )
        #print nclf, '\t', quality, '\t', clf.__class__.__name__
        nclf += 1

        # print test/target data
        print('n' + '\t'  + 'ndx' + '\t'+ 'target' + '\t',)
        for col_name in col_names_description:
            print(col_name + '\t' ,)
        print('')
        icount = 0
        for idx, row in df_test_description.iterrows():
            print(str(icount) + '\t' + str(idx) + '\t' + str(target[icount]) + '\t',)
            icount += 1
            for col_name in col_names_description:
                print(str(row[col_name]) + '\t',)
            print('')
        print('-----------------------------------------------------------')

        num_yes = sum(target >= 1)
        num_no = len(target) - num_yes
        name_padded = ('name' + (' ' * 35))[:35]
        print('n\t' + name_padded + '\tNco1/' + str(num_yes) + '\tNco0/' + str(num_no) + '\tNmiss\tNfalse\tqual\t' + (" ".join(str(x) for x in target)))
        sorted_table = sorted(table_output, key=itemgetter('quality'), reverse = True)
        for table_entry in sorted_table:
            print(table_entry['nclf'], '\t', table_entry['name'], '\t', table_entry['n_true_pos'], '\t', table_entry['n_true_neg'], '\t', table_entry['n_false_neg'], \
            '\t', table_entry['n_false_pos'], '\t', ("%.2f" %  table_entry['quality']), \
            '\t', table_entry['prediction'])
            
        return sorted_table

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
    parser.add_option("-i", "--in", dest="infile", help="set input hypos csv file [default: %default]", metavar="FILE")
    parser.add_option("-t", "--target-type", dest="target_type", help="set target tsunami measure type [default: %default]")
    parser.add_option("-o", "--out", dest="outpath", help="set output path [default: %default]", metavar="FILE")
    parser.add_option("-v", "--verbose", dest="verbose", default=0, action="count", help="set verbosity level [default: %default]")

    # set defaults
    parser.set_defaults(outpath="./out", infile="./in.csv")

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

    plot_to_file = True

    # apply
    tsunamiLearn = TsunamiLearn()
    df_hypos = tsunamiLearn.createDfHypos(hypos_file, target_type)
    tsunamiLearn.plotScatterMatrix(df_hypos, target_type, plot_to_file)
    
    # convert continuous target to label
    if (target_type == 'At'):
        df_hypos[target_type] = np.log10(df_hypos[target_type])
        df_hypos[target_type] = np.where(df_hypos[target_type] > 0, 1, 0)
        df_hypos[target_type] = df_hypos[target_type].astype(int)
        #df_hypos = df_hypos.round({target_type : 0})
        if (verbose > 0):
            print(df_hypos[target_type])
    elif (target_type == 'It'):
        df_hypos[target_type] = np.where(df_hypos[target_type] > 1, 1, 0)
    #print df_hypos[target_type]
        
    
    summary_list = None
    num_tests = 1000
    for n in range(num_tests):
        rand_seed = 1000 + n * n
        print('n=' + str(n) + '/' + str(num_tests) + ', rand_seed=' + str(rand_seed) + ' ======================================')
        sorted_table = tsunamiLearn.doLearnPredcitClassify(df_hypos, target_type, rand_seed)
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
    print('Summary results: ' + 'nTest:' + str(num_tests) + ' ' +  str(col_names_use) + ' :')
    for n in range(len(sorted_summary_list)):
        print(n, '\t', sorted_summary_list[n]['name'], '\t', ("%.2f" %  (sorted_summary_list[n]['quality'] / num_tests)))


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
        profile_filename = 'tsunamiLearn_profile.txt'
        cProfile.run('main()', profile_filename)
        statsfile = open("profile_stats.txt", "wb")
        p = pstats.Stats(profile_filename, stream=statsfile)
        stats = p.strip_dirs().sort_stats('cumulative')
        stats.print_stats()
        statsfile.close()
        sys.exit(0)
    sys.exit(main())
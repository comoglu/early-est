/***************************************************************************
 * early_est.h
 *
 *
 * Written by Anthony Lomax
 *   ALomax Scientific www.alomax.net
 *
 * created: 2014.02.18
 ***************************************************************************/



// hypocenter =====================================================================================

#define WARNING_LEVEL_STRING_LEN 64

#ifndef MAX_NUM_PICKS
#define MAX_NUM_PICKS 10000
#endif

#define STANDARD_STRLEN 4096


typedef struct {
    double centralValue;
    double upperBound;
    double lowerBound;
    int numLevel;
}
statistic_level;

typedef struct {
    // location
    double ot_std_dev;
    int year, month, day, hour, min; // origin time
    double dec_sec;
    double lat;
    double lon;
    double errh;
    double depth;
    double errz;
    double depth_step;
    int nassoc_P; // number of picks that contributed with weight > 0 to location
    int nassoc; // number of picks of any type associated to this hypocenter
    double dist_min; // minimum distance of associated phase counted as nassoc_P
    double dist_max; // maximum distance of associated phase counted as nassoc_P
    double gap_primary; // maximum azimuth gap
    double gap_secondary; // secondary azimuth gap - largest azimuth gap filled by a single station
    double ampAttenPower; // attenuation decay power from linear regression fit of P amplitudes to distance
    double vector_azimuth; // azimuth of sum of vectors from epicenter to stations
    double vector_distance; // 1/n_stations * distance of sum of vectors from epicenter to stations
    // warning statistics
    statistic_level t50ExLevelStatistics;
    statistic_level taucLevelStatistics;
    statistic_level tdT50ExLevelStatistics;
    char warningLevelString[WARNING_LEVEL_STRING_LEN];
    statistic_level mwpLevelStatistics;
    statistic_level mbLevelStatistics;
    statistic_level t0LevelStatistics;
    statistic_level mwpdLevelStatistics;
    long unique_id;
    int hyp_assoc_index;
    int loc_seq_num;
    char quality_code[8];   // 20150907 AJL - added
}
Hypocenter_early_est;


#define POLARITY_POS 		1
#define POLARITY_UNKNOWN 	0
#define POLARITY_NEG 		-1

typedef struct {
    long event_unique_id;
    char station[16];
    char location[16];
    char channel[16];
    char network[16];
    char locflag;
    int year, month, day, hour, min; // pick time
    double dec_sec;
    char pick_error_type[16];
    double pick_error;
    int fmpolarity; // broad-band polarity measure
    double fmquality; // broad-band polarity measure
    char fmtype[32];
    // event association
    int is_associated;
    double epicentral_distance; // degrees
    double epicentral_azimuth; // degrees CW from N
    double residual;
    char phase[16];
    double take_off_angle_inc; // degrees (0/down->180/up)
    double amplitude_error_ratio; // ratio between a_ref rms amplitude and that predicted for event (see location.c -> calcLinearRegressionPowerRelation)
    double dist_weight;
    double loc_weight;
    double station_quality_weight;
    double amplitude; // a_ref rms amplitude
    double t50; // t50 rms amplitude
    double mb; // mb magnitude
    double mwp; // Mwp magnitude
    double sn_hf; // HF s/n ratio at pick
    double sn_brb; // BRB s/n ratio at pick
    double sn_brb_int; // integrated BRB s/n ratio at pick
}
Pick_early_est;


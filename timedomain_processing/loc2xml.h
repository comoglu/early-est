/*
 * File:   loc2xml.h
 * Author: anthony
 *
 * Created on January 24, 2011, 11:37 AM
 */


// loc
int writeLocXML(char *xmlWriterFileroot, time_t time_stamp, char* agencyId, char* author,
        HypocenterDesc** hypo_list, int num_hypocenters, TimedomainProcessingData** data_list, int num_de_data,
        HypocenterDesc** orphaned_hypo_list, int num_orphaned_hypocenters, // 20221007 AJL - added to for reporting of cancelled events in xml
        int iWriteAssociatedOnly, int iWriteArrivals, int iWriteUnAssociatedPicks, int printIgnoredData, int writeJSONcopy);

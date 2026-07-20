#!/bin/bash
export PATH="/home/comonvgc/early-est-1.2.9/work/gmt_bin:$PATH"

SECONDS=0

# ==============================================
# user settings

# operating system type
OPERATING_SYSTEM=LINUX
#OPERATING_SYSTEM=LINUX
# local directory to accumulate report interval output files: xml, csv, web, plot etc.
# 20190401 AJL  SAVE_DIR=./seedlink_plots
SAVE_DIR=`mktemp -d ${TMPDIR:-/tmp}/$0.XXXXXX`
#echo "SAVE_DIR ${SAVE_DIR}"

# local directory to create web content
LOCAL_DIR_WARNING=/home/comonvgc/early-est.comoglu.com
#echo "LOCAL_DIR_WARNING ${LOCAL_DIR_WARNING}"

# remote web server to put web content
# uncomment the following line to enable ftp transfer to remote web server
#$$$DO_FTP_TO_REMOTE=YES
# uncomment the following line to disable ftp transfer to remote web server
#$$$
DO_FTP_TO_REMOTE=NO
# remote web server login is specified by env variable FTP_USER_PW_HOST, e.g.
# export FTP_USER_PW_HOST="ftp://account:password@account.dom"
# ~/work/mseed_processing/plot_warning_report_GMTX.gmt
# remote web server directory is specified by env variable FTP_DIR_WARNING, e.g.
# export FTP_DIR_WARNING="projects/early-est"
# ftp timeout
FTP_TIMEOUT_SEC=10
FTP_LINUX_TIMEOUT_SEC=120
#
# choose here if you want ftp upload of all files only if no other ftp process is running
# otherwise, ftp upload will be attempted only for critical, real-time files 
# WARNING: only use this if no other ftp processes will be running on this system!
RUN_FTP_ONLY_IF_PREV_COMPLETED=YES
#
FTP_COMMAND=
if [ ${OPERATING_SYSTEM} == MACOSX ] ; then
	# Mac OS X
	FTP_COMMAND="ftp -i -q ${FTP_TIMEOUT_SEC} ${FTP_USER_PW_HOST}"
else
	# assume Linux
	FTP_COMMAND="timeout ${FTP_LINUX_TIMEOUT_SEC} ftp -i ${FTP_HOST}"
fi


# choose here if you want javascript and html compression *.js -> *.jgz,  *.html -> *.hgz 
# server must support serving compressed files, .htaccess must contain:
# AddType application/x-javascript .jgz 
# AddEncoding gzip .jgz
# AddType text/html .hgz
# AddEncoding gzip .hgz
#
USE_COMPRESSION_FOR_JAVASCRIPT_HTML=NO
#USE_COMPRESSION_FOR_JAVASCRIPT_HTML=YES

# choose here if you want to use gs (ghostscript) or convert (ImageMagic command) for ps to jpg conversion
#USE_CONVERT_FOR_PS2JPG=NO
USE_CONVERT_FOR_PS2JPG=NO

# choose here if you want this script to terminate if a previous instance of this script is still running
RUN_ONLY_IF_PREV_COMPLETED=YES

# enable following block only for development of processEvents.py !!!
if [ YES = NO ]; then
	DEV="_DEV"
	echo "WARNING: DEV! Using processEvents${DEV}.py!"
fi

# 20151026 AJL  PLOT_MAP_MECHANISM_TYPE=hash	# sets mechanism type to plot on map, hash or fmamp, see also plot_event_info.gmt, plot_warning_map.gmt, processEvents.py
PLOT_MAP_MECHANISM_TYPE=fmamp_polarity	# sets mechanism type to plot on map, hash or fmamp, see also plot_event_info.gmt, plot_warning_map.gmt, processEvents.py

# check if sending alerts
if [ -n "${DO_EARLY_EST_ALERTS}" ] ; then
	if [ ${DO_EARLY_EST_ALERTS} == YES ] ; then
	   echo "NOTE: Early-est ALERTS active!"
	fi
fi




# ==============================================
# functions


archive_cleanup() {

	# ==============================================
	# archive, cleanup

	# make tgz archive and clean out directory
	let SLENGTH=${#INSTANCE_NAME}
	#echo "${INSTANCE_NAME}: SLENGTH = ${SLENGTH}"
	# check for minimum length path, to avoid removing wrong directories
	# event name must be at least length 19, of form 2010.01.26.15.37.50
	if [ ${SLENGTH} -ge 19 ] ; then
		#echo "SLENGTH >= 19 !"
		PWDIR=$(pwd)
		cd ${INSTANCE_PATH}
		# clean selected
		rm ${INSTANCE_NAME}/plot/*.grid.bin
		# archive
		tar -czf ${INSTANCE_NAME}.tgz ${INSTANCE_NAME}/
		# clean all
		rm -r ${INSTANCE_NAME}/
		#rm -r ${INSTANCE_NAME}/plot/*
		#rmdir ${INSTANCE_NAME}/plot/
		#rm -r ${INSTANCE_NAME}/*
		#rmdir ${INSTANCE_NAME}/
		cd ${PWDIR}
	fi
	
	# 20190412 AJL - bug fix
	# clean up old stuff
	rm -rf ${SAVE_DIR}

	#gwenview ${SAVE_DIR}/t50.jpg

	#echo "To make a movie:"
	#echo "nice convert -delay 20 -quantize RGB ${SAVE_DIR}/${EVENT_NAME_ROOT}*.pdf ${SAVE_DIR}/${EVENT_NAME_ROOT}.mpeg"

	#echo "kaffeine ${SAVE_DIR}/${EVENT_NAME_ROOT}.mpeg"

	#echo "plot_warning_report_seedlink_runtime.bash: ${INSTANCE_NAME}: end script"

}




# ==============================================
# initialization

ROOT_OUT_PATH=$1
#echo "ROOT_OUT_PATH ${ROOT_OUT_PATH}"

INSTANCE_PATH=$2
#echo "INSTANCE_PATH ${INSTANCE_PATH}"

INSTANCE_NAME=$3
#echo "INSTANCE_NAME ${INSTANCE_NAME}"

INSTANCE_PATH_NAME=${INSTANCE_PATH}/${INSTANCE_NAME}

#echo "INSTANCE_PATH_NAME: ${INSTANCE_PATH_NAME}"

echo "plot_warning_report_seedlink_runtime.bash: ${INSTANCE_NAME}: begin script"

# check if last report completed
# if not completed tmp file exists, do not run this report
# 20150506 AJL - added
NOT_COMPLETED_FILE="_temp_not_completed.tmp"
if [ ${RUN_ONLY_IF_PREV_COMPLETED} == YES ] ; then
	# remove not completed file if too old		# 20190401 AJL - added this block
	if [ -e ${NOT_COMPLETED_FILE} ] ; then
		MAX_AGE=180		#seconds
		if [ ${OPERATING_SYSTEM} == MACOSX ] ; then
			# Mac OS X
			if [ `stat -f %a ${NOT_COMPLETED_FILE}` -le $(( `date +%s` - ${MAX_AGE} )) ]; then
				rm ${NOT_COMPLETED_FILE}
			fi
		else
			# assume Linux
			if [ $(stat -c '%Y' "${NOT_COMPLETED_FILE}") -le $(( `date +%s` - ${MAX_AGE} )) ]; then
				rm ${NOT_COMPLETED_FILE}
			fi
		fi
	fi
	if [ -e ${NOT_COMPLETED_FILE} ] ; then
		echo -n "WARNING: plot_warning_report_seedlink_runtime.bash: ${INSTANCE_NAME}: end script: previous not completed: "
		cat ${NOT_COMPLETED_FILE}
		archive_cleanup
		exit -1
	fi
	cat << END > ${NOT_COMPLETED_FILE}
${INSTANCE_NAME}
END
fi

# clean up old stuff
rm -rf ${SAVE_DIR}/events
rm -rf ${SAVE_DIR}/*

mkdir ${LOCAL_DIR_WARNING}/events
mkdir ${SAVE_DIR}/events

# limit size of local output files
tail -n 10000  seedlink_monitor.picks > _temp_.picks
rm -f seedlink_monitor.picks
mv _temp_.picks seedlink_monitor.picks

# seedlink_monitor process ID
MONITOR_PID=-1
if [ ${OPERATING_SYSTEM} == MACOSX ] ; then
	# Mac OS X
	ps -o etime,comm | grep seedlink_monitor > _temp_.pid
else
	# assume Linux
	ps -ao etime=,comm | grep seedlink_mon > _temp_.pid
fi
{
read MONITOR_ELAPSED_TIME DUMMY
} < _temp_.pid
MONITOR_ELAPSED_TIME=${MONITOR_ELAPSED_TIME/:*:*/h}
MONITOR_ELAPSED_TIME=${MONITOR_ELAPSED_TIME/:*/m}
MONITOR_ELAPSED_TIME=${MONITOR_ELAPSED_TIME/-/d}

#echo "DEBUG: plot_warning_report_seedlink_runtime.bash: ${INSTANCE_NAME} dt=${SECONDS}: TP 1"

# event processing - HASH / fmamp focal mechanism calculation =======================
# this must be run before plot_warning_report_GMTX.gmt to have focal mechanisms plotted on main monitor graphic
COMMAND="python python/processEvents${DEV}.py MECHANISM ${SAVE_DIR} ${ROOT_OUT_PATH} ${INSTANCE_PATH} ${INSTANCE_PATH_NAME} ${INSTANCE_NAME} ${PLOT_MAP_MECHANISM_TYPE}"
echo ${COMMAND}
echo "Start: ${COMMAND}" > ./processEvents_MECHANISM.log
${COMMAND} >> ./processEvents_MECHANISM.log
echo "Finished: ${COMMAND}" >> ./processEvents_MECHANISM.log

#echo "DEBUG: plot_warning_report_seedlink_runtime.bash: ${INSTANCE_NAME} dt=${SECONDS}: TP 2"


# ==============================================
# main report processing 

# monitor graphic, maps, etc.
#JX=0.2
JX=0.19
COMMAND="./plot_warning_report_GMT4.gmt ${SAVE_DIR} ${ROOT_OUT_PATH} ${INSTANCE_PATH_NAME} ${INSTANCE_NAME} ${JX} ${PLOT_MAP_MECHANISM_TYPE} ${MONITOR_ELAPSED_TIME}"
echo ${COMMAND}
#${COMMAND} > ./plot_warning_report.log
${COMMAND}
#echo "DEBUG: plot_warning_report_seedlink_runtime.bash: ${INSTANCE_NAME} dt=${SECONDS}: TP 2.1"

ROOT_NAME=${SAVE_DIR}/${INSTANCE_NAME}_Monitor
cp -p ${ROOT_NAME}.pdf ${SAVE_DIR}/t50.pdf
mv ${ROOT_NAME}.pdf ${INSTANCE_PATH_NAME}
#cp -p ${ROOT_NAME}_map.pdf ${SAVE_DIR}/last_map.pdf
#mv ${ROOT_NAME}_map.pdf ${INSTANCE_PATH_NAME}

if [ ${USE_CONVERT_FOR_PS2JPG} == NO ] ; then
	COMMAND="gs -dQUIET -dNOPAUSE -dBATCH -sDEVICE=jpeg -r120 -dJPEGQ=85 -sOutputFile=${SAVE_DIR}/t50.jpg ${SAVE_DIR}/t50.pdf"
else
	COMMAND="convert -density 150 -quality 85  ${ROOT_NAME}.ps ${SAVE_DIR}/t50.jpg"
fi
echo ${COMMAND}
${COMMAND} >> ./plot_warning_report.log
#${COMMAND}
#cp -p ${SAVE_DIR}/t50.jpg ${INSTANCE_PATH}/${EVENT_NAME_ROOT}/${INSTANCE_NAME}_Monitor.jpg
#rm ${ROOT_NAME}.ps
#echo "DEBUG: plot_warning_report_seedlink_runtime.bash: ${INSTANCE_NAME} dt=${SECONDS}: TP 2.2"

cp -p ${INSTANCE_PATH_NAME}/pickmessage.html ${SAVE_DIR}
cp -p ${INSTANCE_PATH_NAME}/sta.health.html ${SAVE_DIR}
cp -p ${INSTANCE_PATH_NAME}/station.map.html ${SAVE_DIR}
cp -p ${INSTANCE_PATH_NAME}/station.map.js ${SAVE_DIR}
cp -p ${INSTANCE_PATH_NAME}/hypomessage.html ${SAVE_DIR}
cp -p ${INSTANCE_PATH_NAME}/hypos.csv ${SAVE_DIR}
cp -p ${INSTANCE_PATH_NAME}/hypos.csv ${SAVE_DIR}/hypos.txt
cp -p ${INSTANCE_PATH_NAME}/picks.csv ${SAVE_DIR}
cp -p ${INSTANCE_PATH_NAME}/picks.csv ${SAVE_DIR}/picks.txt
cp -p ${INSTANCE_PATH_NAME}/pickdata_nlloc.txt ${SAVE_DIR}
cp -p ${ROOT_OUT_PATH}/hypolist.html ${SAVE_DIR}
cp -p ${ROOT_OUT_PATH}/hypolist.csv ${SAVE_DIR}
#cp -p ${ROOT_OUT_PATH}/hypolist.csv ${SAVE_DIR}/hypolist.txt
cp -p ${INSTANCE_PATH_NAME}/monitor.xml ${SAVE_DIR}
cp -p ${INSTANCE_PATH_NAME}/alarm_notification.txt ${SAVE_DIR}
#

cp -p ${SAVE_DIR}/t50.pdf ${LOCAL_DIR_WARNING}
cp -p ${SAVE_DIR}/t50.jpg ${LOCAL_DIR_WARNING}
cp -p ${SAVE_DIR}/pickmessage.html ${LOCAL_DIR_WARNING}
cp -p ${SAVE_DIR}/hypomessage.html ${LOCAL_DIR_WARNING}
cp -p ${SAVE_DIR}/hypolist.html ${LOCAL_DIR_WARNING}
#
#echo "DEBUG: plot_warning_report_seedlink_runtime.bash: ${INSTANCE_NAME} dt=${SECONDS}: TP 3"
HTML_EXT=html
XML_EXT=xml
if [ ${USE_COMPRESSION_FOR_JAVASCRIPT_HTML} == YES ] ; then
	HTML_EXT=hgz
	XML_EXT=xgz
	#
	gzip -c ${LOCAL_DIR_WARNING}/pickmessage.html > ${LOCAL_DIR_WARNING}/pickmessage.${HTML_EXT}
	sed 's/pickmessage.html/pickmessage.hgz/' ${LOCAL_DIR_WARNING}/warning_list.html > ${LOCAL_DIR_WARNING}/warning_list.html.${HTML_EXT}
	#
	gzip -c ${LOCAL_DIR_WARNING}/hypolist.html > ${LOCAL_DIR_WARNING}/hypolist.${HTML_EXT}
	sed 's/hypolist.html/hypolist.hgz/' ${LOCAL_DIR_WARNING}/warning.html > ${LOCAL_DIR_WARNING}/warning.html.tmp
	sed 's/sta.health.html/sta.health.hgz/' ${LOCAL_DIR_WARNING}/warning.html.tmp > ${LOCAL_DIR_WARNING}/warning.html.tmp2
	rm ${LOCAL_DIR_WARNING}/warning.html.tmp
	sed 's/monitor.xml/monitor.xgz/' ${LOCAL_DIR_WARNING}/warning.html.tmp2 > ${LOCAL_DIR_WARNING}/warning.html.${HTML_EXT}
	rm ${LOCAL_DIR_WARNING}/warning.html.tmp2
else
	cp ${LOCAL_DIR_WARNING}/warning_list.html ${LOCAL_DIR_WARNING}/warning_list.html.${HTML_EXT}
	cp ${LOCAL_DIR_WARNING}/warning.html ${LOCAL_DIR_WARNING}/warning.html.${HTML_EXT}
fi
#echo "DEBUG: plot_warning_report_seedlink_runtime.bash: ${INSTANCE_NAME} dt=${SECONDS}: TP 4"
#
cp -p ${SAVE_DIR}/sta.health.html ${LOCAL_DIR_WARNING}
cp -p ${SAVE_DIR}/station.map.js ${LOCAL_DIR_WARNING}
cp -p ${SAVE_DIR}/station.map.html ${LOCAL_DIR_WARNING}
cp -p ${SAVE_DIR}/hypos.csv ${LOCAL_DIR_WARNING}
cp -p ${SAVE_DIR}/hypos.txt ${LOCAL_DIR_WARNING}
cp -p ${SAVE_DIR}/picks.csv ${LOCAL_DIR_WARNING}
cp -p ${SAVE_DIR}/picks.txt ${LOCAL_DIR_WARNING}
cp -p ${SAVE_DIR}/pickdata_nlloc.txt ${LOCAL_DIR_WARNING}
cp -p ${SAVE_DIR}/hypolist.csv ${LOCAL_DIR_WARNING}
#cp -p ${SAVE_DIR}/hypolist.txt ${LOCAL_DIR_WARNING}
cp -p ${SAVE_DIR}/monitor.xml ${LOCAL_DIR_WARNING}

# make sure alarm sounds
SOUND_ALERT=NO
read ALARM_LEVEL < ${SAVE_DIR}/alarm_notification.txt
if [ ${ALARM_LEVEL} == 3 ] ; then	# hypocenter alert message sent
	SOUND_ALERT=YES
elif [ ${ALARM_LEVEL} == 2 ] ; then	# new or existing hypocenter with first valid magnitude
	SOUND_ALERT=YES
elif [ ${ALARM_LEVEL} == 1 ] ; then	# new hypocenter w/o valid magnitude
	SOUND_ALERT=NO
fi
#SOUND_ALERT=YES
#echo "DEBUG: ===========>>>>>>>>> ALARM_LEVEL: ${ALARM_LEVEL}   SOUND_ALERT: ${SOUND_ALERT}"
if [ ${SOUND_ALERT} == YES ] ; then
	cp ${LOCAL_DIR_WARNING}/warning_image_only_alert_ALERT.html ${LOCAL_DIR_WARNING}/warning_image_only_alert.html
	# DEBUG! TODO: comment out the next line!
	#./run_action__alert_sent.bash ${INSTANCE_PATH_NAME}/hypos_pretty.csv "TEST "
else
	cp ${LOCAL_DIR_WARNING}/warning_image_only_alert_NO_ALERT.html ${LOCAL_DIR_WARNING}/warning_image_only_alert.html
fi
touch ${LOCAL_DIR_WARNING}/warning_image_only_alert.html

#echo "DEBUG: plot_warning_report_seedlink_runtime.bash: ${INSTANCE_NAME} dt=${SECONDS}: TP 5"

if [ ${DO_FTP_TO_REMOTE} == YES ] ; then

# check only once if ftp already running, if check again later can cause collisions between ftp in instances of this script
	FTP_OK=YES
	if [ ${RUN_FTP_ONLY_IF_PREV_COMPLETED} == YES ] ; then
		ps cax | grep ftp
		if [ $? -eq 0 ]; then
			FTP_OK=NO
		fi
	fi
	if [ ${FTP_OK} == YES ] ; then
		echo "> Begin FTP transfers [${INSTANCE_NAME} dt=${SECONDS}]"  >./plot_warning_report_ftp.log
	else
		echo "> Begin FTP transfers [${INSTANCE_NAME} dt=${SECONDS}]"  >>./plot_warning_report_ftp.log
	fi

# do critical real-time ftp transfer
echo "> Starting MAIN1 FTP transfer. [${INSTANCE_NAME} dt=${SECONDS}]"  >>./plot_warning_report_ftp.log
PWDIR=$(pwd)
echo "plot_warning_report_seedlink_runtime.bash: ${INSTANCE_NAME}: begin FTP MAIN1"
echo "LOCAL_DIR_WARNING=${LOCAL_DIR_WARNING}"
echo "FTP_DIR_WARNING=${FTP_DIR_WARNING}"
echo "FTP_COMMAND=${FTP_COMMAND}" >>./plot_warning_report_ftp.log
${FTP_COMMAND} << END >>./plot_warning_report_ftp.log
!# status
put ${LOCAL_DIR_WARNING}/t50.jpg ${FTP_DIR_WARNING}/t50.jpg_PART
rename ${FTP_DIR_WARNING}/t50.jpg_PART ${FTP_DIR_WARNING}/t50.jpg
put ${LOCAL_DIR_WARNING}/warning.html.${HTML_EXT} ${FTP_DIR_WARNING}/warning.html_PART
rename ${FTP_DIR_WARNING}/warning.html_PART ${FTP_DIR_WARNING}/warning.html
!# make sure alarm sounds
put ${LOCAL_DIR_WARNING}/warning_image_only_alert.html ${FTP_DIR_WARNING}/warning_image_only_alert.html_PART
rename ${FTP_DIR_WARNING}/warning_image_only_alert.html_PART ${FTP_DIR_WARNING}/warning_image_only_alert.html
!#
put ${LOCAL_DIR_WARNING}/warning_list.html.${HTML_EXT} ${FTP_DIR_WARNING}/warning_list.html_PART
put ${LOCAL_DIR_WARNING}/pickmessage.${HTML_EXT} ${FTP_DIR_WARNING}/pickmessage.${HTML_EXT}_PART
rename ${FTP_DIR_WARNING}/warning_list.html_PART ${FTP_DIR_WARNING}/warning_list.html
rename ${FTP_DIR_WARNING}/pickmessage.${HTML_EXT}_PART ${FTP_DIR_WARNING}/pickmessage.${HTML_EXT}
put ${LOCAL_DIR_WARNING}/hypomessage.html ${FTP_DIR_WARNING}/hypomessage.html_PART
rename ${FTP_DIR_WARNING}/hypomessage.html_PART ${FTP_DIR_WARNING}/hypomessage.html
put ${LOCAL_DIR_WARNING}/hypolist.${HTML_EXT} ${FTP_DIR_WARNING}/hypolist.${HTML_EXT}_PART
rename ${FTP_DIR_WARNING}/hypolist.${HTML_EXT}_PART ${FTP_DIR_WARNING}/hypolist.${HTML_EXT}
bye
END
#echo "DEBUG: plot_warning_report_seedlink_runtime.bash: ${INSTANCE_NAME} dt=${SECONDS}: TP 6"
echo "plot_warning_report_seedlink_runtime.bash: ${INSTANCE_NAME}: end FTP MAIN1"
cd ${PWDIR}
echo "> Finished MAIN1 FTP transfer. [${INSTANCE_NAME} dt=${SECONDS}]"  >>./plot_warning_report_ftp.log

	
	if [ ${FTP_OK} == NO ] ; then
		echo "X Another FTP process may be running: will not perform MAIN2 FTP transfer. [${INSTANCE_NAME} dt=${SECONDS}]"  >>./plot_warning_report_ftp.log
	else
		echo "> Starting MAIN2 FTP transfer. [${INSTANCE_NAME} dt=${SECONDS}]"  >>./plot_warning_report_ftp.log

		#
		if [ ${USE_COMPRESSION_FOR_JAVASCRIPT_HTML} == YES ] ; then
			gzip -c ${LOCAL_DIR_WARNING}/sta.health.html > ${LOCAL_DIR_WARNING}/sta.health.${HTML_EXT}
		fi
		#
		JS_EXT=js
		if [ ${USE_COMPRESSION_FOR_JAVASCRIPT_HTML} == YES ] ; then
			JS_EXT=jgz
			gzip -c ${LOCAL_DIR_WARNING}/station.map.js > ${LOCAL_DIR_WARNING}/station.map.${JS_EXT}
			gzip -c ${LOCAL_DIR_WARNING}/monitor.xml > ${LOCAL_DIR_WARNING}/monitor.${XML_EXT}
			sed 's/station.map.js/station.map.jgz/' ${LOCAL_DIR_WARNING}/station.map.html > ${LOCAL_DIR_WARNING}/station.map.html.${JS_EXT}
		else
			cp ${LOCAL_DIR_WARNING}/station.map.html ${LOCAL_DIR_WARNING}/station.map.html.${JS_EXT}
		fi
#echo "DEBUG: plot_warning_report_seedlink_runtime.bash: ${INSTANCE_NAME} dt=${SECONDS}: TP 7"

PWDIR=$(pwd)
echo "plot_warning_report_seedlink_runtime.bash: ${INSTANCE_NAME}: begin FTP MAIN2"
echo "LOCAL_DIR_WARNING=${LOCAL_DIR_WARNING}"
echo "FTP_DIR_WARNING=${FTP_DIR_WARNING}"
echo "FTP_COMMAND=${FTP_COMMAND}" >>./plot_warning_report_ftp.log
${FTP_COMMAND} << END >>./plot_warning_report_ftp.log
!# status
put ${LOCAL_DIR_WARNING}/sta.health.${HTML_EXT} ${FTP_DIR_WARNING}/sta.health.${HTML_EXT}_PART
rename ${FTP_DIR_WARNING}/sta.health.${HTML_EXT}_PART ${FTP_DIR_WARNING}/sta.health.${HTML_EXT}
put ${LOCAL_DIR_WARNING}/station.map.html.${JS_EXT} ${FTP_DIR_WARNING}/station.map.html_PART
put ${LOCAL_DIR_WARNING}/station.map.${JS_EXT} ${FTP_DIR_WARNING}/station.map.${JS_EXT}_PART
rename ${FTP_DIR_WARNING}/station.map.html_PART ${FTP_DIR_WARNING}/station.map.html
rename ${FTP_DIR_WARNING}/station.map.${JS_EXT}_PART ${FTP_DIR_WARNING}/station.map.${JS_EXT}
!# status
put ${LOCAL_DIR_WARNING}/hypos.csv ${FTP_DIR_WARNING}/hypos.csv_PART
put ${LOCAL_DIR_WARNING}/hypos.txt ${FTP_DIR_WARNING}/hypos.txt_PART
put ${LOCAL_DIR_WARNING}/picks.csv ${FTP_DIR_WARNING}/picks.csv_PART
put ${LOCAL_DIR_WARNING}/picks.txt ${FTP_DIR_WARNING}/picks.txt_PART
put ${LOCAL_DIR_WARNING}/pickdata_nlloc.txt ${FTP_DIR_WARNING}/pickdata_nlloc.txt_PART
put ${LOCAL_DIR_WARNING}/hypolist.csv ${FTP_DIR_WARNING}/hypolist.csv_PART
!#put ${LOCAL_DIR_WARNING}/hypolist.txt ${FTP_DIR_WARNING}/hypolist.txt_PART
put ${LOCAL_DIR_WARNING}/monitor.${XML_EXT} ${FTP_DIR_WARNING}/monitor.${XML_EXT}_PART
rename ${FTP_DIR_WARNING}/monitor.${XML_EXT}_PART ${FTP_DIR_WARNING}/monitor.${XML_EXT}
!# status
rename ${FTP_DIR_WARNING}/hypos.csv_PART ${FTP_DIR_WARNING}/hypos.csv
rename ${FTP_DIR_WARNING}/hypos.txt_PART ${FTP_DIR_WARNING}/hypos.txt
rename ${FTP_DIR_WARNING}/picks.csv_PART ${FTP_DIR_WARNING}/picks.csv
rename ${FTP_DIR_WARNING}/picks.txt_PART ${FTP_DIR_WARNING}/picks.txt
rename ${FTP_DIR_WARNING}/pickdata_nlloc.txt_PART ${FTP_DIR_WARNING}/pickdata_nlloc.txt
rename ${FTP_DIR_WARNING}/hypolist.csv_PART ${FTP_DIR_WARNING}/hypolist.csv
!#rename ${FTP_DIR_WARNING}/hypolist.txt_PART ${FTP_DIR_WARNING}/hypolist.txt
!#
put ${LOCAL_DIR_WARNING}/t50.pdf ${FTP_DIR_WARNING}/t50.pdf_PART
rename ${FTP_DIR_WARNING}/t50.pdf_PART ${FTP_DIR_WARNING}/t50.pdf
bye
END
echo "plot_warning_report_seedlink_runtime.bash: ${INSTANCE_NAME}: end FTP MAIN2"
cd ${PWDIR}

		echo "> Finished MAIN2 FTP transfer. [${INSTANCE_NAME} dt=${SECONDS}]"  >>./plot_warning_report_ftp.log
	fi

fi
#echo "DEBUG: plot_warning_report_seedlink_runtime.bash: ${INSTANCE_NAME} dt=${SECONDS}: TP 8"

if [ ${USE_COMPRESSION_FOR_JAVASCRIPT_HTML} == YES ] ; then
	rm ${LOCAL_DIR_WARNING}/hypolist.${HTML_EXT}
	rm ${LOCAL_DIR_WARNING}/warning_list.html.${HTML_EXT}
	rm ${LOCAL_DIR_WARNING}/warning.html.${HTML_EXT}
	rm ${LOCAL_DIR_WARNING}/pickmessage.${HTML_EXT}
	if [ ${DO_FTP_TO_REMOTE} == YES ] ; then
		rm ${LOCAL_DIR_WARNING}/sta.health.${HTML_EXT}
		rm ${LOCAL_DIR_WARNING}/station.map.html.${JS_EXT}
	fi
fi


# ==============================================
# event processing 
#echo "DEBUG: plot_warning_report_seedlink_runtime.bash: ${INSTANCE_NAME} dt=${SECONDS}: TP 9"


# event processing =======================
# HASH focal mechanism calculation is run before plot_warning_report_GMTX.gmt so focal mechanisms plotted on main monitor graphic
COMMAND="python python/processEvents${DEV}.py EVENTS ${SAVE_DIR} ${ROOT_OUT_PATH} ${INSTANCE_PATH} ${INSTANCE_PATH_NAME} ${INSTANCE_NAME} ${PLOT_MAP_MECHANISM_TYPE}"
echo ${COMMAND}
echo "Start: ${COMMAND}" > ./processEvents_EVENTS.log
${COMMAND} >> ./processEvents_EVENTS.log
echo "Finished: ${COMMAND}" >> ./processEvents_EVENTS.log
#echo "DEBUG: plot_warning_report_seedlink_runtime.bash: ${INSTANCE_NAME} dt=${SECONDS}: TP 10"


# 20110621 AJL - events handling
cp -pRf ${INSTANCE_PATH_NAME}/events/* ${SAVE_DIR}/events 2> /dev/null
cp -pRf ${SAVE_DIR}/events/* ${LOCAL_DIR_WARNING}/events 2> /dev/null


# get list of old event files =======================
# get list of active event id's
ACTIVE_EVENT_ID_LIST=" "
{
	read HEADER_LINE
	while read EVENT_ID remainder ; do
		ACTIVE_EVENT_ID_LIST="${ACTIVE_EVENT_ID_LIST} ${EVENT_ID}"
	done
} < ${LOCAL_DIR_WARNING}/hypolist.csv
#echo "######### DEBUG: ACTIVE_EVENT_ID_LIST (hypolist.csv) = <${ACTIVE_EVENT_ID_LIST}>"
# get list of event id's for all existing event files
ls ${LOCAL_DIR_WARNING}/events/hypo.*.picks.html > _temp_list.tmp 2> /dev/null
ALL_EVENT_ID_LIST=" "
{
	while read EVENT_FILE ; do
		EVENT_FILE=${EVENT_FILE#${LOCAL_DIR_WARNING}/events/hypo.}
		#EVENT_FILE=${EVENT_FILE#hypo.}
		EVENT_FILE=${EVENT_FILE%.picks.html}
		ALL_EVENT_ID_LIST="${ALL_EVENT_ID_LIST} ${EVENT_FILE}"
	done
} < _temp_list.tmp
#echo "######### DEBUG: ALL_EVENT_ID_LIST (events/hypo.*.html) = <${ALL_EVENT_ID_LIST}>"
# get list of event id's to delete
DELETE_EVENT_ID_LIST=${ALL_EVENT_ID_LIST}
for EVENT_ID in ${ACTIVE_EVENT_ID_LIST} ; do
	DELETE_EVENT_ID_LIST="${DELETE_EVENT_ID_LIST//${EVENT_ID}/}"
done
#echo "######### DEBUG: DELETE_EVENT_ID_LIST = <${DELETE_EVENT_ID_LIST}>"
DELETE_EVENT_FILE_LIST=" "
for EVENT_ID in ${DELETE_EVENT_ID_LIST} ; do
	DELETE_EVENT_FILE_LIST="${DELETE_EVENT_FILE_LIST} hypo.${EVENT_ID}.*"
done
#echo "######### DEBUG: DELETE_EVENT_FILE_LIST = <${DELETE_EVENT_FILE_LIST}>"

# delete old event files
PWDIR=$(pwd)
cd ${LOCAL_DIR_WARNING}/events
#echo "######### DEBUG: DELETE IN DIR ${LOCAL_DIR_WARNING}/events : rm -f ${DELETE_EVENT_FILE_LIST}"
rm -f ${DELETE_EVENT_FILE_LIST}
cd ${PWDIR}
#echo "DEBUG: plot_warning_report_seedlink_runtime.bash: ${INSTANCE_NAME} dt=${SECONDS}: TP 11"


# get list of updated event files =======================
# get list of event id's for all updated event files
TIME_OF_LAST_REPORT_FILE="_temp_report_timer.tmp"
UPDATED_EVENT_FILE_LIST=" "
if [ -e ${TIME_OF_LAST_REPORT_FILE} ] ; then
	# 20170102 ls ${LOCAL_DIR_WARNING}/events/hypo.*.html > _temp_list.tmp 2> /dev/null
	ls ${LOCAL_DIR_WARNING}/events/hypo.*.picks.html > _temp_list.tmp 2> /dev/null
	{
		while read EVENT_FILE ; do
			if [ ${EVENT_FILE} -nt ${TIME_OF_LAST_REPORT_FILE} ] ; then
				EVENT_FILE=${EVENT_FILE#${LOCAL_DIR_WARNING}/events/hypo.}
				#EVENT_FILE=${EVENT_FILE#hypo.}
				EVENT_FILE=${EVENT_FILE%.picks.html}
				UPDATED_EVENT_FILE_LIST="${UPDATED_EVENT_FILE_LIST} hypo.${EVENT_FILE}.*"
			fi
		done
	} < _temp_list.tmp
fi
#echo "######### DEBUG: UPDATED_EVENT_FILE_LIST = <${UPDATED_EVENT_FILE_LIST}>"
DEBUG_TEXT=$(ls -lt ${TIME_OF_LAST_REPORT_FILE})
#echo "######### DEBUG: BEFORE: TIME_OF_LAST_REPORT_FILE = ${DEBUG_TEXT}"
cat << END > ${TIME_OF_LAST_REPORT_FILE}
XXX
END
DEBUG_TEXT=$(ls -lt ${TIME_OF_LAST_REPORT_FILE})
#echo "######### DEBUG: AFTER: TIME_OF_LAST_REPORT_FILE = ${DEBUG_TEXT}"
#echo "DEBUG: plot_warning_report_seedlink_runtime.bash: ${INSTANCE_NAME} dt=${SECONDS}: TP 12"


if [ ${DO_FTP_TO_REMOTE} == YES ] ; then

	if [ ${FTP_OK} == NO ] ; then
		echo "X Another FTP process may be running: will not perform EVENTS FTP transfer. [${INSTANCE_NAME} dt=${SECONDS}]"  >>./plot_warning_report_ftp.log
	else
		echo "> Starting EVENTS FTP transfer. [${INSTANCE_NAME} dt=${SECONDS}]"  >>./plot_warning_report_ftp.log

# make sure there is at least one file available for mput and mdelete
cat << END > ${LOCAL_DIR_WARNING}/events/EMPTY_FILE.TXT
EMPTY
END

PWDIR=$(pwd)
#echo "######### DEBUG: UPDATED_EVENT_FILE_LIST = <${UPDATED_EVENT_FILE_LIST}>"
#echo "######### DEBUG: DELETE_EVENT_FILE_LIST = <${DELETE_EVENT_FILE_LIST}>"
echo "plot_warning_report_seedlink_runtime.bash: ${INSTANCE_NAME}: begin FTP EVENTS"
echo "FTP_COMMAND=${FTP_COMMAND}" >>./plot_warning_report_ftp.log
${FTP_COMMAND} << END >>./plot_warning_report_ftp.log
!# status
!# 20110621 AJL - events handling
!# status
mkdir ${FTP_DIR_WARNING}/events
cd ${FTP_DIR_WARNING}/events
lcd ${LOCAL_DIR_WARNING}/events
!#put ${LOCAL_DIR_WARNING}/events/hypo.1308825303011.html ${FTP_DIR_WARNING}/events/hypo.1308825303011.html
!# status
mput ${UPDATED_EVENT_FILE_LIST} EMPTY_FILE.TXT
!# status
mdelete ${DELETE_EVENT_FILE_LIST} EMPTY_FILE.TXT
!# status
bye
END
echo "plot_warning_report_seedlink_runtime.bash: ${INSTANCE_NAME}: end FTP EVENTS"
cd ${PWDIR}

		echo "> Finished EVENTS FTP transfer. [${INSTANCE_NAME} dt=${SECONDS}]"  >>./plot_warning_report_ftp.log
	fi

fi

#echo "DEBUG: plot_warning_report_seedlink_runtime.bash: ${INSTANCE_NAME} dt=${SECONDS}: TP 13"

archive_cleanup

# report completed# 20150506 AJL - added
if [ ${RUN_ONLY_IF_PREV_COMPLETED} == YES ] ; then
	rm ${NOT_COMPLETED_FILE}
fi

#gwenview ${SAVE_DIR}/t50.jpg

#echo "To make a movie:"
#echo "nice convert -delay 20 -quantize RGB ${SAVE_DIR}/${EVENT_NAME_ROOT}*.pdf ${SAVE_DIR}/${EVENT_NAME_ROOT}.mpeg"

#echo "kaffeine ${SAVE_DIR}/${EVENT_NAME_ROOT}.mpeg"

echo "plot_warning_report_seedlink_runtime.bash: ${INSTANCE_NAME} dt=${SECONDS}: end script =============="


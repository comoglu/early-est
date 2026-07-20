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

__author__ = "anthony"
__date__ = "$Jun 16, 2011 11:58:05 AM$"


import configparser
from datetime import *
from htmlDocGen import *
from parseQML import *
from evaluateEvent import EvaluateEvent
from tsunamilearn import rupturePredict


class ProcessEvents(object):

	global __ALARM_CRITICAL_VALUE, __ALARM_RED_CUTOFF, __ALARM_YELLOW_CUTOFF
	__ALARM_CRITICAL_VALUE = 8.0  # NOTE: Important!!!  Must equal value in timedomain_processing_report.h
	__ALARM_RED_CUTOFF = (1.1 * __ALARM_CRITICAL_VALUE)
	__ALARM_YELLOW_CUTOFF = (0.9 * __ALARM_CRITICAL_VALUE)
	global __MIN_NUMBER_WARNING_LEVELS_ALARM
	__MIN_NUMBER_WARNING_LEVELS_ALARM = 4

	global __TDT0_CRITICAL_VALUE, __TDT0_RED_CUTOFF, __TDT0_YELLOW_CUTOFF
	__TDT0_CRITICAL_VALUE = 510  # NOTE: Important!!!  Must equal value in papers
	__TDT0_RED_CUTOFF = (1.1 * __TDT0_CRITICAL_VALUE)
	__TDT0_YELLOW_CUTOFF = (0.9 * __TDT0_CRITICAL_VALUE)
	global __MIN_NUMBER_WARNING_LEVELS_TDT0
	__MIN_NUMBER_WARNING_LEVELS_TDT0 = __MIN_NUMBER_WARNING_LEVELS_ALARM

	global __UNDERWATER_RED_CUTOFF, __UNDERWATER_YELLOW_CUTOFF
	__UNDERWATER_RED_CUTOFF = 20.0  # percent
	__UNDERWATER_YELLOW_CUTOFF = 2.0  # percent

	global __SLAB_DEPTH_RED_CUTOFF, __SLAB_DEPTH_YELLOW_CUTOFF
	__SLAB_DEPTH_RED_CUTOFF = 35
	__SLAB_DEPTH_YELLOW_CUTOFF = 45

	global __MWP_RED_CUTOFF, __MWP_YELLOW_CUTOFF
	__MWP_RED_CUTOFF = 7.65  # NOTE: Important!!!  Must equal value in timedomain_processing_report.h
	__MWP_YELLOW_CUTOFF = 7.35
	global __MIN_NUMBER_MWP_ALARM
	__MIN_NUMBER_MWP_ALARM = 4

	global __MIN_MWP_FOR_VALID_MWPD
	__MIN_MWP_FOR_VALID_MWPD = 7.0

	global __PLATE_BNDY_SUB_MAX_DIST, __PLATE_BNDY_MAX_DIST
	__PLATE_BNDY_SUB_MAX_DIST = 1000.0  # km
	__PLATE_BNDY_MAX_DIST = 200.0  # km

	global __PLATE_BNDY_SUB_DIST_RED_CUTOFF, __PLATE_BNDY_SUB_DIST_YELLOW_CUTOFF
	__PLATE_BNDY_SUB_DIST_RED_CUTOFF = 150.0
	__PLATE_BNDY_SUB_DIST_YELLOW_CUTOFF = 200.0
	global __PLATE_BNDY_CB_DIST_RED_CUTOFF, __PLATE_BNDY_CB_DIST_YELLOW_CUTOFF
	__PLATE_BNDY_CB_DIST_RED_CUTOFF = 50.0
	__PLATE_BNDY_CB_DIST_YELLOW_CUTOFF = 100.0
	global __PLATE_BNDY_TF_DIST_RED_CUTOFF, __PLATE_BNDY_TF_DIST_YELLOW_CUTOFF
	__PLATE_BNDY_TF_DIST_RED_CUTOFF = 30.0
	__PLATE_BNDY_TF_DIST_YELLOW_CUTOFF = 60.0
	
	global __MIN_NUM_MWP_FOR_TSUNAMI_RUPTURE_PREDICTION, __MIN_MWP_FOR_TSUNAMI_RUPTURE_PREDICTION
	__MIN_NUM_MWP_FOR_TSUNAMI_RUPTURE_PREDICTION = 20
	__MIN_MWP_FOR_TSUNAMI_RUPTURE_PREDICTION = 6.95
   
	
	global COLOR_NONE
	COLOR_NONE = ""
	
	global MECH_HASH
	MECH_HASH = "hash"
	global MECH_FMAMP_POL
	MECH_FMAMP_POL = "fmamp_polarity"
	global MECH_FMAMP_POL_WAVEFORM
	MECH_FMAMP_POL_WAVEFORM = "fmamp_polarity_waveform"
	global MECH_FMAMP_AMP_AREF
	MECH_FMAMP_AMP_AREF = "fmamp_amp_aref"
	global MECH_FMAMP_AMP_MWP
	MECH_FMAMP_AMP_MWP = "fmamp_amp_mwp"
	
	global config
	global flag_config__event_info__warning_colors_show
	global flag_config__event_info__tsunami_decision_table_colors_show
	global flag_config__event_info__location_map_write
	global flag_config__event_info__tsunami_write
	global flag_config__event_info__tsunami_evaluation_write
	global flag_config__event_info__tsunami_decision_table_write
	global flag_config__event_info__first_motion_mech_run_hash
	global flag_config__event_info__first_motion_mech_run_fmamp_pol
	global flag_config__event_info__first_motion_mech_run_fmamp_pol_waveform
	global flag_config__event_info__first_motion_mech_run_fmamp_amp
	global flag_config__event_info__first_motion_mech_run
	global flag_config__event_info__first_motion_mech_write
	
	# rupture prediction parameters   #20160712 AJL added
	global flag_config__event_info__tsunami_rupture_predictions_write
	global config__event_info__ff_duration_pkl_file_root
	global config__event_info__ff_rupture_area_pkl_file_root
	global config__event_info__at_pkl_file_root
	global fconfig__event_info__it_pkl_file_root
	global tsunami_rupture_predictions_text
	tsunami_rupture_predictions_text = { \
										'It' : ('Tsunami importance (It)', '{0:.0f}', ''), \
										'At' : ('Tsunami amplitude @100km (At)', '{0:.1f}', 'm'), \
										'ff_duration' : ('Finite faulting duration', '{0:.0f}', 's'), \
										'ff_rupture_area' : ('Finite faulting rupture area', '{0:.0f}', 'km^2'), \
										}
	

	def space(self, nsp=1):
		return "&nbsp;" * nsp

	def tsunamiEvaluationText(self, warning_level_value, n_warning_level_value, warning_level, n_warning_level, warning_level_color):

		tsunami_info_text = Tag.FONT()
		tsunami_subject = ""

		# check Td*T50Ex level
		if n_warning_level_value < __MIN_NUMBER_WARNING_LEVELS_ALARM:
			tsunami_info_text.append("not available (Td*T50Ex Warning Level: ")
			tsunami_info_text.append(Tag.FONT(Tag.FONT(" " + warning_level_color, tag_class="strong"), tag_class="bg" + warning_level_color))
			tsunami_info_text.append(Tag.FONT(")"))
			tsunami_subject = ""
		elif warning_level_value >= __ALARM_RED_CUTOFF:
			tsunami_info_text.append(Tag.FONT("LIKELY TSUNAMIGENIC EVENT if shallow, non-strike-slip, oceanic event (Td*T50Ex Warning Level: ", tag_class="strong"))
			tsunami_info_text.append(Tag.FONT(Tag.FONT(" " + warning_level_color, tag_class="strong"), tag_class="bg" + warning_level_color))
			tsunami_info_text.append(Tag.FONT(")", tag_class="strong"))
			tsunami_subject = "LIKELY TSUNAMIGENIC: " + self.space(4)
		elif warning_level_value >= __ALARM_YELLOW_CUTOFF:
			tsunami_info_text.append(Tag.FONT("Possible tsunamigenic event if shallow, non-strike-slip, oceanic event (Td*T50Ex Warning Level: ", tag_class="strong"))
			tsunami_info_text.append(Tag.FONT(Tag.FONT(" " + warning_level_color, tag_class="strong"), tag_class="bg" + warning_level_color))
			tsunami_info_text.append(Tag.FONT(")", tag_class="strong"))
			tsunami_subject = "Possible tsunamigenic: " + self.space(4)
		else:
			tsunami_info_text.append(Tag.FONT("unlikely tsunamigenic event (Td*T50Ex Warning Level: "))
			tsunami_info_text.append(Tag.FONT(Tag.FONT(" " + warning_level_color, tag_class="strong"), tag_class="bg" + warning_level_color))
			tsunami_info_text.append(Tag.FONT(")"))
			tsunami_subject = ""

		tsunami_info_text.append(Tag.BR())
		tsunami_info_text.append(self.space(4))
		tsunami_info_text.append(Tag.FONT("(DISCLAIMER: Tsunamigenic evaluation is based on the value of the Td*T50Ex Warning Level.", tag_class="small"))
		tsunami_info_text.append(Tag.FONT(" This evaluation only concerns the likelihood that this earthquake generated a regional or larger scale tsunami.", tag_class="small"))
		tsunami_info_text.append(Tag.FONT(" This evaluation DOES NOT concern the size and effects of a possible tsunami, which depend on details of the", tag_class="small"))
		tsunami_info_text.append(Tag.FONT(" earthquake source, ocean bathymetry, coastal distances and population density, and many other factors.)", tag_class="small"))
		tsunami_info_text.append(Tag.FONT(" This evaluation DOES NOT apply to and DOES NOT EXCLUDE the possibility that this earthquake generated a local tsunami,", tag_class="small"))
		tsunami_info_text.append(Tag.FONT(" which can be destructive along coasts within a 100 km or more from the earthquake epicenter.", tag_class="small"))

		return [tsunami_subject, tsunami_info_text]


	# create compact hypo information string
	def create_compact_hypo_info_string(self, feregion_str, time_str, quality_code, lat, lon, depth,
										mag_mb, num_mb, mag_Mwp, num_Mwp, mag_T0, num_T0, mag_Mwpd, num_Mwpd, mag_wl, wlColor):

		# set FE-region, use << and >> for < and > in HTML
		hypo_info_string = '{0:s}  {1:s} {2:s}  lat:{3:s} lon:{4:s}  depth:{5:s}km  mb:{6:s}[{7:s}]  Mwp:{8:s}[{9:s}]  T0:{10:s}s[{11:s}]  Mwpd:{12:s}[{13:s}]  Td*T50Ex:{14:s}|{15:s}'\
		.format(feregion_str, time_str, quality_code, lat, lon, depth, mag_mb, num_mb, mag_Mwp, num_Mwp, mag_T0, num_T0, mag_Mwpd, num_Mwpd, mag_wl, wlColor)

		return hypo_info_string

	# create map link
	def createMapLink(self, event_id):

		# create Google map link, use && for & in HTML
		map_link = "hypo." + event_id + ".map.html"

		return map_link


	def styleTag(self):

		htmlText = Tag.STYLE(type="text/css")
		htmlText.append(Tag.COMMENT(
						"""
			body	  { font-family: sans-serif; font-size: medium; text-align: left; padding: 0px; background-color: #cccccc; }
			body.bgred  { background-color: #ff5555; }
			body.bgyellow  { background-color: #ffff66; }
			body.bggreen  { background-color: #c8c8c8; }
			font.bggrey  { background-color: #cccccc; }
			font.strong  { font-weight: bold; }
			font.small  { font-size: smaller; }
			font.bgRED  { background-color: #ff5555; }
			font.bgYELLOW  { background-color: #ffff66; }
			font.bgGREEN  { background-color: #77ff77; }
			font.bgGREY  { background-color: #cccccc; }
			font.bgORANGE  { background-color: #ffa500; }
			font.warning  { color: #bb0000; font-weight: bold; }
			
			h1		{ font-size: larger; text-align: left; border: 5px solid #ffffff; background-color: #ffffff; }
			h2		{ font-size: medium; font-weight: bold; color:#bb0000; text-align: left; border: 5px solid #ffffff; background-color: #ffffff; }
			
			table	 { border-collapse: collapse; padding: 0px; background-color: #cccccc;}
			caption   { font-weight: bold; text-align: left; border: 3px solid #cccccc;
						padding-left: 6px; padding-right: 6px;
						padding-top: 5px; padding-bottom: 3px;
						background-color: #eeeeee;
			}
			th		{ font-weight: normal; text-align: right; border 3px solid #cccccc; background-color: #eeeeee; 
						padding-left: 6px; padding-right: 6px;
						padding-top: 5px; padding-bottom: 0px;
			}
			tr		{ background-color: #cccccc; vertical-align:top;
			}
			td		{ background-color: #cccccc; 
						padding-left: 6px; padding-right: 6px;
						padding-top: 5px; padding-bottom: 0px;
			}
			td.wide   { width: 60%; }
			hr		{ padding 0px; height: 2px; border-width: 0; background-color: #cccccc; width: 100% }
			table.text	 { background-color: #ffffff; width: 100%; }
			tr.text		{ background-color: #cccccc; vertical-align:top;
						border-top:	2px solid #cccccc;
						border-bottom: 2px solid #cccccc;
			}
			td.text   { background-color: #ffffff; 
						padding-left: 6px; padding-right: 6px;
						padding-top: 5px; padding-bottom: 0px;
			}
			td.bgRED  { background-color: #ff5555; padding: 5px 6px 0px 6px; }
			td.bgYELLOW  { background-color: #ffff66; padding: 5px 6px 0px 6px; }
			td.bgGREEN  { background-color: #77ff77; padding: 5px 6px 0px 6px; }
			td.bgGREY  { background-color: #cccccc; padding: 5px 6px 0px 6px; }
			td.bgORANGE  { background-color: #ffa500; padding: 5px 6px 0px 6px; }

			table.diagram	 { background-color: #ffffff; width: 100%; border-collapse:collapse; border:0px solid #ffffff; border-left-width:2px;; border-right-width:2px;}
			tr.diagram		{ background-color: #cccccc; vertical-align:top }
			th.diagram   { background-color: #ffffff; border-left:2px solid #ffffff; padding: 5px 0px 0px 0px; white-space:nowrap; }
			td.diagram   { background-color: #ffffff; border-left:2px solid #ffffff; padding: 0px 0px 0px 0px; white-space:nowrap; }
			td.diagram_bgRED  { background-color: #ff5555; border-left:2px solid #ffffff; border-top:2px solid #ffffff; padding: 5px 6px 3px 6px; white-space:nowrap; }
			td.diagram_bgYELLOW  { background-color: #ffff66; border-left:2px solid #ffffff; border-top:2px solid #ffffff; padding: 5px 6px 3px 6px; white-space:nowrap; }
			td.diagram_bgGREEN  { background-color: #77ff77; border-left:2px solid #ffffff; border-top:2px solid #ffffff; padding: 5px 6px 3px 6px; white-space:nowrap; }
			td.diagram_bgLTGREEN  { background-color: #ddffdd; border-left:2px solid #ffffff; border-top:2px solid #ffffff; padding: 5px 6px 3px 6px; white-space:nowrap; }
			td.diagram_bgGREY  { background-color: #cccccc; border-left:2px solid #ffffff; border-top:2px solid #ffffff; padding: 5px 6px 3px 6px; white-space:nowrap; }
			td.diagram_continued_bgRED  { background-color: #ff5555; border-left:2px solid #ffffff; border-top:1px solid #ff5555; padding: 5px 6px 5px 6px; width: 10%; }
			td.diagram_continued_bgYELLOW  { background-color: #ffff66; border-left:2px solid #ffffff; border-top:1px solid #ffff66; padding: 5px 6px 5px 6px; width: 10%; }
			td.diagram_continued_bgGREEN  { background-color: #77ff77; border-left:2px solid #ffffff; border-top:1px solid #77ff77; padding: 5px 6px 5px 6px; width: 10%; }
			td.diagram_continued_bgGREY  { background-color: #cccccc; border-left:2px solid #ffffff; border-top:1px solid #cccccc; padding: 5px 6px 5px 6px; width: 10%; }
			td.diagram_continued_bgLTGREEN  { background-color: #ddffdd; border-left:2px solid #ffffff; border-top:1px solid #ddffdd; padding: 5px 6px 5px 6px; width: 10%; }
			
			"""
						))
		return htmlText


	def _formatDiscriminant(self, num, value):
		
		# try:
			
		cnum = str(num)

		if int(num) <= 0:  # no value
			cvalue_display = '-'
			cvalue_real = '-'
		else:
			fvalue = -1.0;
			if isinstance(value, str):
				fvalue = float(value.rstrip("X"))
			else:
				fvalue = float(value)

			cvalue_real = '{0:.1f}'.format(fvalue)
			if int(num) >= __MIN_NUMBER_WARNING_LEVELS_ALARM:  # have minimum number to report
				cvalue_display = cvalue_real
			else:  # do not have minimum number to report
				cvalue_display = '-'
				if int(num) > 0:  # have readings
					cvalue_real += 'X'

		return cnum, cvalue_display, cvalue_real
	
		# except Exception as e:
		#	print "error: invoking _formatDiscriminant: num=", num, ", value=", value
		#	return "-1", "ERROR", "ERROR"


	def evaluateLevelColorIncreasing(self, level, num, min_num, level_red, level_yellow, color_unavailable, show_none):
		
		if not show_none:
			return COLOR_NONE

		if num >= min_num:
			if level >= level_red:
				return"RED"
			elif level >= level_yellow:
				return"YELLOW"
			else:
				return"GREEN"
			
		return color_unavailable

				
	def evaluateLevelColorDecreasing(self, level, num, min_num, level_red, level_yellow, color_unavailable, show_none):
		
		if not show_none:
			return COLOR_NONE

		if num >= min_num:
			if level <= level_red:
				return"RED"
			elif level <= level_yellow:
				return"YELLOW"
			else:
				return"GREEN"
			
		return color_unavailable
	
	
	def updateCumulativeAlertColor(self, alertColor, newColor):
		
		if alertColor == "GREEN" or newColor == "GREEN":
			return "GREEN"
		if alertColor == "YELLOW" or newColor == "YELLOW":
			return "YELLOW"
		if alertColor == "RED" or newColor == "RED":
			return "RED"
		
		return "GREY"
		
		
	def getOriginQualityColor(self, origin_quality_code):
		
		if origin_quality_code == "A":
			return "GREEN"
		if origin_quality_code == "B":
			return "YELLOW"
		if origin_quality_code == "C":
			return "ORANGE"
		if origin_quality_code == "D":
			return "RED"
		
		return "GREY"
		
		
		
	def addInformationTableData(self, tableRow, padCellColors, bgColor, text):
		
		tableRowInternal = Tag.TR(tag_class="diagram")
		
		# use following if putting pad cells after new information in row
		# for i in range(len(padCellColors) - 1, -1, -1):
		#	color = padCellColors[i]
		for color in padCellColors:
			tableData = Tag.TD(tag_class="diagram_continued_bg" + color)
			tableData.append(Tag.FONT(self.space(1), tag_class="small"))
			tableRowInternal.append(tableData)

		color = "GREY"
		if bgColor is not None:
			color = bgColor
		tableData = Tag.TD(tag_class="diagram_bg" + color)
		tableData.append(Tag.FONT(self.space(1) + text, tag_class="small"))
		tableRowInternal.append(tableData)
		
		tableInternal = Tag.TABLE(tag_class="diagram")
		tableInternal.append(tableRowInternal)
		tableRow.append(Tag.TD(tableInternal, tag_class="diagram"))
						
		return tableRow
		

	def addNecessaryTableData(self, tableRow, padCellColors, bgColor, text):
		
		tableRow = self.addInformationTableData(tableRow, padCellColors, bgColor, text)
		padCellColors.append(bgColor)
		return tableRow, padCellColors
		

	def writeHtmlDecisionDiagram(self, ipercent_prob_underwater, percent_prob_underwater_str,
				mag_wl_value, inum_wl, mag_wl_display,
				mag_tdt0_value, inum_tdt0, mag_tdt0_display,
				closest_plate_boundaries,
				is_inside_slab, slab_depth, slab_depth_str,
				mag_Mwp_real, inum_Mwp, mag_Mwp_display,
				hypo_depth, hypo_depth_str,
				maxHorizontalUncertainty
				):
		
		alertColorInterplateThrust = "GREY" 
		alertColorThrustFault = "GREY" 
		alertColorTransformFault = "GREY" 
		   
		table = Tag.TABLE(tag_class="diagram")
		
		# list of cell  background colors to propagate/emphasize necessary discriminant color level 
		padCellColorsInterplateThrust = []
		padCellColorsThrustFault = []
		padCellColorsTransformFault = []
		
		# event class labels
		tableRow = Tag.TR(tag_class="diagram")
		tableData = Tag.TH(tag_class="diagram")
		tableData.append(Tag.FONT("Event type:"))
		tableRow.append(tableData)
		# Interplate Thrust
		tableData = Tag.TD(tag_class="diagram_bg" + alertColorInterplateThrust)
		tableData.append(Tag.FONT(Tag.FONT(" " + "Subduction Zone", tag_class="strong")))
		tableRow.append(tableData)
		# Thrust Fault
		tableData = Tag.TD(tag_class="diagram_bg" + alertColorThrustFault)
		tableData.append(Tag.FONT(Tag.FONT(" " + "Other Thrust/Normal", tag_class="strong")))
		tableRow.append(tableData)
		# Transform Fault
		tableData = Tag.TD(tag_class="diagram_bg" + alertColorTransformFault)
		tableData.append(Tag.FONT(Tag.FONT(" " + "Strike-slip", tag_class="strong")))
		tableRow.append(tableData)
		table.append(tableRow)

		# plate boundary type and distance
		tableRow = Tag.TR(tag_class="diagram")
		tableData = Tag.TH(tag_class="diagram")
		tableData.append(Tag.FONT("Compatible close plate boundary?:"))
		tableRow.append(tableData)
		plate_boundary_color_interplate_thrust = None
		plate_boundary_color_thrust_fault = None
		plate_boundary_color_transform_fault = None
		closest_bndry_dist_min_interplate_thrust = -1.0
		closest_bndry_dist_min_thrust_fault = -1.0
		closest_bndry_dist_min_transform_fault = -1.0
		for closest_plate_boundary_distance_min, closest_plate_boundary_code, closest_plate_boundary_class, closest_boundary_class_id in closest_plate_boundaries:
			# Interplate Thrust
			if plate_boundary_color_interplate_thrust is None:
				if closest_boundary_class_id == "SUB":
					plate_boundary_color_interplate_thrust = self.evaluateLevelColorDecreasing(closest_plate_boundary_distance_min, 1, -1, \
						__PLATE_BNDY_SUB_DIST_RED_CUTOFF + 2.0 * hypo_depth + maxHorizontalUncertainty, \
						__PLATE_BNDY_SUB_DIST_YELLOW_CUTOFF + hypo_depth + maxHorizontalUncertainty, \
						"GREY", flag_config__event_info__tsunami_decision_table_colors_show)
					closest_bndry_dist_min_interplate_thrust = closest_plate_boundary_distance_min
			# Thrust Fault
			if plate_boundary_color_thrust_fault is None:
				if closest_boundary_class_id == "OCB" or closest_boundary_class_id == "CCB" :
					plate_boundary_color_thrust_fault = self.evaluateLevelColorDecreasing(closest_plate_boundary_distance_min, 1, -1, \
						__PLATE_BNDY_CB_DIST_RED_CUTOFF + maxHorizontalUncertainty, __PLATE_BNDY_CB_DIST_YELLOW_CUTOFF + maxHorizontalUncertainty, \
						"GREY", flag_config__event_info__tsunami_decision_table_colors_show)
					closest_bndry_dist_min_thrust_fault = closest_plate_boundary_distance_min
			# Transform Fault
			if plate_boundary_color_transform_fault is None:
				if closest_boundary_class_id == "OTF" or closest_boundary_class_id == "CTF" :
					plate_boundary_color_transform_fault = self.evaluateLevelColorDecreasing(closest_plate_boundary_distance_min, 1, -1, \
						__PLATE_BNDY_TF_DIST_RED_CUTOFF + maxHorizontalUncertainty, __PLATE_BNDY_TF_DIST_YELLOW_CUTOFF + maxHorizontalUncertainty, \
						"GREY", flag_config__event_info__tsunami_decision_table_colors_show)
					closest_bndry_dist_min_transform_fault = closest_plate_boundary_distance_min
		# Interplate Thrust
		if plate_boundary_color_interplate_thrust == "RED" or plate_boundary_color_interplate_thrust == "YELLOW":
			tableRow = self.addInformationTableData(tableRow, padCellColorsInterplateThrust, plate_boundary_color_interplate_thrust, "YES (" + str(int(0.5 + closest_bndry_dist_min_interplate_thrust)) + "km)")
		else:
			tableRow = self.addInformationTableData(tableRow, padCellColorsInterplateThrust, \
													("GREEN" if flag_config__event_info__tsunami_decision_table_colors_show else ""), "no")
		# Thrust Fault
		if plate_boundary_color_thrust_fault == "RED" or plate_boundary_color_thrust_fault == "YELLOW":
			tableRow = self.addInformationTableData(tableRow, padCellColorsThrustFault, plate_boundary_color_thrust_fault, "YES (" + str(int(0.5 + closest_bndry_dist_min_thrust_fault)) + "km)")
		else:
			tableRow = self.addInformationTableData(tableRow, padCellColorsThrustFault, \
													("GREEN" if flag_config__event_info__tsunami_decision_table_colors_show else ""), "no")
		# Transform Fault
		if plate_boundary_color_transform_fault == "RED" or plate_boundary_color_transform_fault == "YELLOW":
			tableRow = self.addInformationTableData(tableRow, padCellColorsTransformFault, plate_boundary_color_transform_fault, "YES (" + str(int(0.5 + closest_bndry_dist_min_transform_fault)) + "km)")
		else:
			tableRow = self.addInformationTableData(tableRow, padCellColorsTransformFault, \
													("GREEN" if flag_config__event_info__tsunami_decision_table_colors_show else ""), "no")
		table.append(tableRow)

		# probablilty underwater
		tableRow = Tag.TR(tag_class="diagram")
		tableData = Tag.TH(tag_class="diagram")
		tableData.append(Tag.FONT("Prob underwater:"))
		tableRow.append(tableData)
		underWaterColor = self.evaluateLevelColorIncreasing(ipercent_prob_underwater, 1, -1, __UNDERWATER_RED_CUTOFF, __UNDERWATER_YELLOW_CUTOFF, \
															"GREY", flag_config__event_info__tsunami_decision_table_colors_show)
		# Interplate Thrust
		tableRow, padCellColorsInterplateThrust = self.addNecessaryTableData(tableRow, padCellColorsInterplateThrust, underWaterColor, percent_prob_underwater_str)
		# Thrust Fault
		tableRow, padCellColorsThrustFault = self.addNecessaryTableData(tableRow, padCellColorsThrustFault, underWaterColor, percent_prob_underwater_str)
		# Transform Fault
		tableRow, padCellColorsTransformFault = self.addNecessaryTableData(tableRow, padCellColorsTransformFault, underWaterColor, percent_prob_underwater_str)
		table.append(tableRow)
		
		# TdT50Ex warning level
		tableRow = Tag.TR(tag_class="diagram")
		tableData = Tag.TH(tag_class="diagram")
		tableData.append(Tag.FONT("Td*T50Ex:"))
		tableRow.append(tableData)
		wlColor = self.evaluateLevelColorIncreasing(mag_wl_value, inum_wl, __MIN_NUMBER_WARNING_LEVELS_ALARM, __ALARM_RED_CUTOFF, __ALARM_YELLOW_CUTOFF, \
													"LTGREEN", flag_config__event_info__tsunami_decision_table_colors_show)
		# Interplate Thrust
		tableRow, padCellColorsInterplateThrust = self.addNecessaryTableData(tableRow, padCellColorsInterplateThrust, wlColor, str(mag_wl_display))
		# Thrust Fault
		tableRow, padCellColorsThrustFault = self.addNecessaryTableData(tableRow, padCellColorsThrustFault, wlColor, str(mag_wl_display))
		# Transform Fault
		tableRow, padCellColorsTransformFault = self.addNecessaryTableData(tableRow, padCellColorsTransformFault, wlColor, str(mag_wl_display))
		table.append(tableRow)
	   
		# TdT0 warning level
		tableRow = Tag.TR(tag_class="diagram")
		tableData = Tag.TH(tag_class="diagram")
		tableData.append(Tag.FONT("Td*To:"))
		tableRow.append(tableData)
		tdt0Color = self.evaluateLevelColorIncreasing(mag_tdt0_value, inum_tdt0, __MIN_NUMBER_WARNING_LEVELS_TDT0, __TDT0_RED_CUTOFF, __TDT0_YELLOW_CUTOFF, \
													  "GREY", flag_config__event_info__tsunami_decision_table_colors_show)
		# Interplate Thrust
		tableRow, padCellColorsInterplateThrust = self.addNecessaryTableData(tableRow, padCellColorsInterplateThrust, tdt0Color, str(mag_tdt0_display))
		# Thrust Fault
		tableRow, padCellColorsThrustFault = self.addNecessaryTableData(tableRow, padCellColorsThrustFault, tdt0Color, str(mag_tdt0_display))
		# Transform Fault
		tableRow, padCellColorsTransformFault = self.addNecessaryTableData(tableRow, padCellColorsTransformFault, tdt0Color, str(mag_tdt0_display))
		table.append(tableRow)
	   
		# Mwp magnitude
		tableRow = Tag.TR(tag_class="diagram")
		tableData = Tag.TH(tag_class="diagram")
		tableData.append(Tag.FONT("Mwp magnitude:"))
		tableRow.append(tableData)
		mwpColor = self.evaluateLevelColorIncreasing(mag_Mwp_real, inum_Mwp, __MIN_NUMBER_MWP_ALARM, __MWP_RED_CUTOFF, __MWP_YELLOW_CUTOFF, \
													 "LTGREEN", flag_config__event_info__tsunami_decision_table_colors_show)
		# Interplate Thrust
		tableRow = self.addInformationTableData(tableRow, padCellColorsInterplateThrust, mwpColor, str(mag_Mwp_display))
		# Thrust Fault
		tableRow = self.addInformationTableData(tableRow, padCellColorsThrustFault, mwpColor, str(mag_Mwp_display))
		# Transform Fault
		tableRow = self.addInformationTableData(tableRow, padCellColorsTransformFault, mwpColor, str(mag_Mwp_display))
		table.append(tableRow)
	   
		# hypo depth
		tableRow = Tag.TR(tag_class="diagram")
		tableData = Tag.TH(tag_class="diagram")
		tableData.append(Tag.FONT("Hypocenter depth:"))
		tableRow.append(tableData)
		depthColor = self.evaluateLevelColorDecreasing(hypo_depth, 1, -1, __SLAB_DEPTH_RED_CUTOFF, __SLAB_DEPTH_YELLOW_CUTOFF, \
													   "GREY", flag_config__event_info__tsunami_decision_table_colors_show)
		# Interplate Thrust
		tableRow = self.addInformationTableData(tableRow, padCellColorsInterplateThrust, depthColor, str(hypo_depth_str) + "km")
		# Thrust Fault
		tableRow = self.addInformationTableData(tableRow, padCellColorsThrustFault, depthColor, str(hypo_depth_str) + "km")
		# Transform Fault
		tableRow = self.addInformationTableData(tableRow, padCellColorsTransformFault, depthColor, str(hypo_depth_str) + "km")
		table.append(tableRow)
	   
		# slab depth
		tableRow = Tag.TR(tag_class="diagram")
		tableData = Tag.TH(tag_class="diagram")
		tableData.append(Tag.FONT("Slab depth at epicenter:"))
		tableRow.append(tableData)
		# Interplate Thrust
		slabDepthColor = None
		if is_inside_slab and slab_depth != EvaluateEvent.SLAB_DEPTH_ERROR:
			slabDepthColor = self.evaluateLevelColorDecreasing(slab_depth, 1, -1, __SLAB_DEPTH_RED_CUTOFF, __SLAB_DEPTH_YELLOW_CUTOFF, \
															   "GREY", flag_config__event_info__tsunami_decision_table_colors_show)
			tableRow = self.addInformationTableData(tableRow, padCellColorsInterplateThrust, slabDepthColor, slab_depth_str)
		else:
			tableRow = self.addInformationTableData(tableRow, padCellColorsInterplateThrust, slabDepthColor, "n/a")
		# Thrust Fault
		tableRow = self.addInformationTableData(tableRow, padCellColorsThrustFault, slabDepthColor, "-")
		# Transform Fault
		tableRow = self.addInformationTableData(tableRow, padCellColorsTransformFault, slabDepthColor, "-")
		table.append(tableRow)
		
		return table



	def writeHtmlEventPicks(self, instance_path_name, event_id):
		
		f_html_event_pick_out = open(instance_path_name + "/events/hypo." + event_id + ".picks.html", "w")
		f_html_picks_in = open(instance_path_name + "/pickmessage.html", "r")

		for line in f_html_picks_in:
			if not "<tr" in line :
				f_html_event_pick_out.write(line)
			elif event_id in line:
				line = line.replace("events/", "")
				f_html_event_pick_out.write(line)

		f_html_picks_in.close()
		f_html_event_pick_out.close()


	def writeHtmlEventInfo(self, instance_path_name, event_id, event_root_node, mech_root_nodes_list, event_info_last_update_time):

		parseQML = ParseQML()

		# event unique id
		publicID = parseQML.getAtribute(event_root_node, "publicID")

		# set event-origin node
		origin_node = parseQML.getFirstChildWithName(event_root_node, "origin")
		region = parseQML.getValueOfFirstChildWithName(origin_node, "region")
		origin_time = parseQML.getValueInFirstChildWithName(origin_node, "time", "value")
		origin_time = parseQML.formatTime(origin_time, 2)
		origin_quality_node = parseQML.getFirstChildWithName(origin_node, "quality")
		quality_node = parseQML.getFirstChildWithName(origin_quality_node, "ee:qualityIndicators")
		origin_ee_quality_code = "X"
		origin_ee_quality_code = parseQML.getValueOfFirstChildWithName(quality_node, "ee:qualityCode")
		amplitude_attenuation_power_node = parseQML.getFirstChildWithName(origin_quality_node, "ee:amplitudeAttenuationPower")
		origin_uncertainty_node = parseQML.getFirstChildWithName(origin_node, "originUncertainty")
		confidence_ellipsoid_node = parseQML.getFirstChildWithName(origin_uncertainty_node, "ee:confidenceEllipsoidNLL")

		# retrieve TdT50Ex warning-level and color
		try:
			num_T50Ex, mag_T50Ex_display, mag_T50Ex_real = self._formatDiscriminant(
																				parseQML.getEarlyEstDiscriminantChildValue(event_root_node, "T50Ex", "stationCount"),
																				parseQML.getEarlyEstDiscriminantChildValue(event_root_node, "T50Ex", "value"))
		except:
			num_T50Ex, mag_T50Ex_display, mag_T50Ex_real = self._formatDiscriminant('0', '0')
		#
		try:
			num_Td, mag_Td_display, mag_Td_real = self._formatDiscriminant(
																	   parseQML.getEarlyEstDiscriminantChildValue(event_root_node, "Td", "stationCount"),
																	   parseQML.getEarlyEstDiscriminantChildValue(event_root_node, "Td", "value"))
		except:
			num_Td, mag_Td_display, mag_Td_real = self._formatDiscriminant('0', '0')
		#
		mag_wl_str = parseQML.getEarlyEstDiscriminantChildValue(event_root_node, "TdT50Ex", "value")
		try:
			mag_wl_value = float(mag_wl_str)
		except:
			mag_wl_value = -1.0
		try:
			num_wl, mag_wl_display, mag_wl_real = self._formatDiscriminant(
																	   parseQML.getEarlyEstDiscriminantChildValue(event_root_node, "TdT50Ex", "stationCount"),
																	   mag_wl_str)
		except:
			num_wl, mag_wl_display, mag_wl_real = self._formatDiscriminant('0', '0')
		try:
			inum_wl = int(num_wl)
		except:
			inum_wl = 0
		wlColor = self.evaluateLevelColorIncreasing(mag_wl_value, inum_wl, __MIN_NUMBER_WARNING_LEVELS_ALARM, __ALARM_RED_CUTOFF, __ALARM_YELLOW_CUTOFF, \
													"GREY", flag_config__event_info__warning_colors_show)
		try:
			num_T0, mag_T0_display, mag_T0_real = self._formatDiscriminant(
																	   parseQML.getEarlyEstDiscriminantChildValue(event_root_node, "T0", "stationCount"),
																	   parseQML.getEarlyEstDiscriminantChildValue(event_root_node, "T0", "value"))
		except:
			num_T0, mag_T0_display, mag_T0_real = self._formatDiscriminant('0', '0')

		# warning level text
		tsunamiTextList = self.tsunamiEvaluationText(mag_wl_value, inum_wl, mag_wl_display, num_wl, wlColor)
		
		# TdTo warning level
		inum = int(min(int(num_Td), int(num_T0)))
		mag_tdt0_value = -1.0
		if inum > 0:
			mag_tdt0_value = float(mag_Td_real.rstrip("X")) * float(mag_T0_real.rstrip("X"))
		try:
			num_tdt0, mag_tdt0_display, mag_tdt0_real = self._formatDiscriminant(inum, mag_tdt0_value)
		except:
			num_tdt0, mag_tdt0_display, mag_tdt0_real = self._formatDiscriminant('0', '0')
		inum_tdt0 = int(num_tdt0)

		# retrieve magnitudes
		try:
			preferredMagnitudeID = parseQML.getValueOfFirstChildWithName(event_root_node, "preferredMagnitudeID")
			#print("DEBUG: preferredMagnitudeID", preferredMagnitudeID)
			# example preferredMagnitudeID: smi:alomax.net/ee/event/1515695183892/magnitude/Mwp
			preferredMagnitude = preferredMagnitudeID.rsplit("/", 1)[1]
		except:
			preferredMagnitude = "NONE"   
		#print("DEBUG: preferredMagnitude", preferredMagnitude)
		#
		try:
			num_mb, mag_mb_display, mag_mb_real = self._formatDiscriminant(
																		   parseQML.getMagnitudeChildValue(event_root_node, "mb", "stationCount"),
																		   parseQML.getMagnitudeChildValue(event_root_node, "mb", "value"))
			if (preferredMagnitude == "mb"):
				mag_mb_display = mag_mb_display + "*"
				mag_mb_real = mag_mb_real + "*"
		except:
			num_mb, mag_mb_display, mag_mb_real = self._formatDiscriminant('0', '0')
		#
		try:
			num_Mwp, mag_Mwp_display, mag_Mwp_real = self._formatDiscriminant(
																			  parseQML.getMagnitudeChildValue(event_root_node, "Mwp", "stationCount"),
																			  parseQML.getMagnitudeChildValue(event_root_node, "Mwp", "value"))
			if (preferredMagnitude == "Mwp"):
				mag_Mwp_display = mag_Mwp_display + "*"
				mag_Mwp_real = mag_Mwp_real + "*"
		except:
			num_Mwp, mag_Mwp_display, mag_Mwp_real = self._formatDiscriminant('0', '0')
		#
		try:
			num_Mwpd, mag_Mwpd_display, mag_Mwpd_real = self._formatDiscriminant(
																			 parseQML.getMagnitudeChildValue(event_root_node, "Mwpd", "stationCount"),
																			 parseQML.getMagnitudeChildValue(event_root_node, "Mwpd", "value"))
			if (preferredMagnitude == "Mwpd"):
				mag_Mwpd_display = mag_Mwpd_display + "*"
				mag_Mwpd_real = mag_Mwpd_real + "*"
		except:
			num_Mwpd, mag_Mwpd_display, mag_Mwpd_real = self._formatDiscriminant('0', '0')
		try:
			mag_Mwp_value = float(parseQML.getMagnitudeChildValue(event_root_node, "Mwp", "value"))
		except:
			mag_Mwp_value = -9.9
		inum_Mwpd = int(num_Mwpd)
		if inum_Mwpd > 0 and mag_Mwp_value < __MIN_MWP_FOR_VALID_MWPD:
			mag_Mwpd_display = '-'
			if inum_Mwpd >= __MIN_NUMBER_WARNING_LEVELS_ALARM:
				mag_Mwpd_real += 'X'
			mag_Mwpd_real += 'inv'

		# set event title
		titleRoot1 = ""
		if flag_config__event_info__tsunami_evaluation_write:
			titleRoot1 = tsunamiTextList[0]
		titleRoot1+= region + self.space(4) + origin_time \
			+ self.space(4)
		titleRoot2 = self.space(4) \
			+ "mb:" + mag_mb_display + self.space(4) \
			+ "Mwp:" + mag_Mwp_display  + self.space(4) \
			+ "Mwpd:"  + mag_Mwpd_display  + self.space(4) \
			+ "Td*T50Ex:" + mag_wl_display + " "

		# set misc data
		hypo_lon = float(parseQML.getValueInFirstChildWithName(origin_node, "longitude", "value"))
		hypo_lon_str = '{0:.2f}'.format(hypo_lon)
		hypo_lat = float(parseQML.getValueInFirstChildWithName(origin_node, "latitude", "value"))
		hypo_lat_str = '{0:.2f}'.format(hypo_lat)
		hypo_depth = float(parseQML.getValueInFirstChildWithName(origin_node, "depth", "value"))
		hypo_depth = hypo_depth / 1000.0	# 20180105 AJL - QML uses depth in meters !
		hypo_depth_str = '{0:.0f}'.format(hypo_depth)
		# confidence ellipse
		minHorizontalUncertainty = float(parseQML.getValueOfFirstChildWithName(origin_uncertainty_node, "minHorizontalUncertainty"))
		minHorizontalUncertainty = minHorizontalUncertainty / 1000.0	# 20191112 AJL - QML uses meters !
		maxHorizontalUncertainty = float(parseQML.getValueOfFirstChildWithName(origin_uncertainty_node, "maxHorizontalUncertainty"))
		maxHorizontalUncertainty = maxHorizontalUncertainty / 1000.0	# 20191112 AJL - QML uses meters !
		azimuthMaxHorizontalUncertainty = float(parseQML.getValueOfFirstChildWithName(origin_uncertainty_node, "azimuthMaxHorizontalUncertainty"))
		
		# calculated values
		ipercent_prob_underwater = -1
		is_inside_slab, n_slab, slab_name, slab_depth = False, None, None, None
		closest_plate_boundaries = []
		try:
			evaluateEvent = EvaluateEvent()
			percent_prob_underwater, percent_prob_underwater_std_dev = evaluateEvent.percentProbIsUnderwater(hypo_lat, hypo_lon, minHorizontalUncertainty, maxHorizontalUncertainty, azimuthMaxHorizontalUncertainty)
			ipercent_prob_underwater = int(0.5 + percent_prob_underwater)
			ipercent_prob_underwater_std_dev = int(0.5 + percent_prob_underwater_std_dev)
			exclude_boundary_codes = ""
			nboundary = 0
			while nboundary < 99:
				closest_plate_boundary_distance_min, closest_plate_boundary_code, closest_plate_boundary_class, closest_boundary_class_id \
 = evaluateEvent.findClosestPlateBoundary(hypo_lat, hypo_lon, exclude_boundary_codes)
				if closest_plate_boundary_distance_min > __PLATE_BNDY_SUB_MAX_DIST:  # maximum distance for close boundaries
					break
				nboundary += 1
				if (nboundary > 4 or closest_plate_boundary_distance_min > __PLATE_BNDY_MAX_DIST) and closest_boundary_class_id != "SUB":  # maximum of 4 boundaaries at <= MAX_DIST, except if subduction boundary (since deep events can be far from associated subduction boundary)
					continue	  
				closest_plate_boundary_distance_min = 10.0 * round(closest_plate_boundary_distance_min / 10.0)
				closest_plate_boundaries.append((closest_plate_boundary_distance_min, closest_plate_boundary_code, closest_plate_boundary_class, closest_boundary_class_id))
				exclude_boundary_codes += "$" + closest_plate_boundary_code
			is_inside_slab, n_slab, slab_name, slab_depth = evaluateEvent.isInsideSlab(hypo_lat, hypo_lon)
			# print(">>>>>> DEBUG: is_inside_slab, n_slab, slab_name, slab_depth", is_inside_slab, n_slab, slab_name, slab_depth)
			# write values to file		# 20160608 AJL - added
			f_calc_out = open(instance_path_name + "/events/hypo." + event_id + ".calc.csv", "w")
			f_calc_out.write('P_underwater P_underwater_std is_over_slab n_slab slab_name slab_depth')
			f_calc_out.write(' plate_bdy_list')
			f_calc_out.write('\n')
			f_calc_out.write(str(percent_prob_underwater) + ' ' + str(percent_prob_underwater_std_dev))
			if (is_inside_slab):
				f_calc_out.write( ' ' + str(is_inside_slab) + ' ' +  str(n_slab) + ' ' + slab_name.replace(' ', '_') + ' ' + "{:.1f}".format(slab_depth))
			else:
				f_calc_out.write( ' ' + str(False) + ' ' +  str(-1) + ' ' + '-' + ' ' + str(-1))
			f_calc_out.write(' ')
			if (len(closest_plate_boundaries) > 0) :
				for closest_plate_boundary_distance_min, closest_plate_boundary_code, closest_plate_boundary_class, closest_boundary_class_id in closest_plate_boundaries:
					f_calc_out.write(str(closest_plate_boundary_distance_min) + '|' + closest_plate_boundary_code \
								 + '|' +  closest_boundary_class_id + ';' )
			else:
				f_calc_out.write('-');
			f_calc_out.write('\n')
			f_calc_out.close()
		except Exception as e:
			print("error: generating calculated values in EvaluateEvent:", str(e))

		document = Tag.HTML()

		docTitle = Tag.TITLE(titleRoot1 + origin_ee_quality_code + titleRoot2 + wlColor)
		docHead = Tag.HEAD(docTitle)
		docHead.append(self.styleTag())

		docBody = Tag.BODY()
		document.append([docHead, docBody])

		docH1 = Tag.H1()
		# link to Ealry-est real-time display
		docH1.append(Tag.A("Early-est", href="../", title="Open Early-est real-time display", target="_new"))
		docH1.append(": " + self.space(4))
		docH1.append(titleRoot1)
		docH1.append(Tag.FONT("q:" + origin_ee_quality_code + "&nbsp;", tag_class="bg" + self.getOriginQualityColor(origin_ee_quality_code)))
		docH1.append(titleRoot2)
		docH1.append(Tag.FONT(wlColor, tag_class="bg" + wlColor))
		# focal mechanism thumbnails
		if flag_config__event_info__first_motion_mech_write:
			# docH1.append(Tag.BR())
			for (mech_type, mech_root_node) in mech_root_nodes_list :				
				image_file = "./hypo." + event_id + ".mech." + mech_type + ".thumb.jpg"
				image_tag = Tag.IMG(image_file, image_file, align="top", height="20")
				docH1.append(" " + self.space(4) + mech_type + ":")
				docH1.append(image_tag)
		docBody.append(docH1)
		
		docH2 = Tag.H2()
		docH2.append("Automatic solution - this event has not been reviewed by a seismologist")
		docH2.append(Tag.BR())
		docH2.append("Automatically determined event information may be incorrect!")
		docBody.append(docH2)


		#
		# earthquake information
		#

		tableEarthquakeInfo = Tag.TABLE()
		tableEarthquakeInfoRow = Tag.TR()

		tableText = Tag.TABLE()

		#
		# Earthquake details
		#

		table = Tag.TABLE(tag_class="text")
		table.append(Tag.CAPTION("Earthquake Details:"))

		# magnitude
		tableRow = Tag.TR(tag_class="text")
		tableRow.append((Tag.TH("Magnitude:")))
		tableData = Tag.TD(tag_class="text")
		tableData.append(Tag.FONT("mb: " + mag_mb_real, tag_class="strong"))
		tableData.append(Tag.FONT(" [" + num_mb + "]", tag_class="small"))
		tableData.append(Tag.FONT(self.space(3) + "Mwp: " + mag_Mwp_real, tag_class="strong"))
		tableData.append(Tag.FONT(" [" + num_Mwp + "]", tag_class="small"))
		tableData.append(Tag.FONT(self.space(3) + "Mwpd: " + mag_Mwpd_real, tag_class="strong"))
		tableData.append(Tag.FONT(" [" + num_Mwpd + "]", tag_class="small"))
		tableRow.append(tableData)
		table.append(tableRow)

		# region
		tableRow = Tag.TR(tag_class="text")
		tableRow.append((Tag.TH("Region:")))
		#hypo_description = self.create_compact_hypo_info_string(region, origin_time, origin_ee_quality_code, hypo_lat_str, hypo_lon_str, hypo_depth_str, \
		#														mag_mb_display, num_mb, mag_Mwp_display, num_Mwp, mag_T0_display, num_T0, mag_Mwpd_display, num_Mwpd, mag_wl_display, wlColor)
		map_link = self.createMapLink(event_id)
		aRef = Tag.A(region, href=map_link, title="View epicenter in Google Maps", target="target=map_" + publicID)
		tableRow.append((Tag.TD(Tag.FONT(aRef, tag_class="strong"), tag_class="text")))
		table.append(tableRow)

		# origin time
		tableRow = Tag.TR(tag_class="text")
		tableRow.append((Tag.TH("Origin&nbsp;Time:")))  # &nbsp; prevents newline in tabe heading
		tableRow.append((Tag.TD(Tag.FONT(origin_time + " UTC", tag_class="strong"), tag_class="text")))
		table.append(tableRow)

		# hypocenter
		tableRow = Tag.TR(tag_class="text")
		tableRow.append((Tag.TH("Hypocenter:")))
		tableData = Tag.TD(tag_class="text")
		tableData.append(Tag.FONT("lat: " + hypo_lat_str + "&deg;" + 
						 self.space(2) + "lon: " + hypo_lon_str + "&deg;", tag_class="strong"))
		tableData.append(Tag.FONT(self.space(1) + "[+/-" + '{0:.0f}'.format(maxHorizontalUncertainty) + "km]", tag_class="small"))
		tableData.append(Tag.FONT(self.space(3) + "depth: " + hypo_depth_str + "km", tag_class="strong"))
		tableData.append(Tag.FONT(self.space(1) + "[+/-" + '{0:.0f}'.format(float(parseQML.getValueInFirstChildWithName(origin_node, "depth", "uncertainty")) / 1000.0) + "km]", tag_class="small"))  # 20180115 AJL - QML uses depth in meters !
		tableRow.append(tableData)
		table.append(tableRow)

		# location quality
		tableRow = Tag.TR(tag_class="text")
		tableRow.append((Tag.TH("Location Quality:")))
		tableData = Tag.TD(tag_class="text")
		tableData.append("qualityCode: ")
		tableData.append(Tag.FONT("&nbsp;" + origin_ee_quality_code + "&nbsp;", tag_class="bg" + self.getOriginQualityColor(origin_ee_quality_code)))
		tableData.append(self.space(3))
		tableData.append("stdErr: " + '{0:.1f}'.format(float(parseQML.getValueOfFirstChildWithName(origin_quality_node, "standardError"))) + "s")
		tableData.append(self.space(3))
		tableData.append(Tag.A("assocPhases", href="hypo." + event_id + ".picks.html", title="View list of associated phases", target="_self"))
		tableData.append(": " + parseQML.getValueOfFirstChildWithName(origin_quality_node, "associatedPhaseCount"))
		tableData.append(self.space(3) + "usedPhases: " + parseQML.getValueOfFirstChildWithName(origin_quality_node, "usedPhaseCount"))
		tableData.append(Tag.BR())
		tableData.append("minDist: " + '{0:.1f}'.format(float(parseQML.getValueOfFirstChildWithName(origin_quality_node, "minimumDistance"))) + "&deg;" + 
						 self.space(3) + "maxDist: " + '{0:.1f}'.format(float(parseQML.getValueOfFirstChildWithName(origin_quality_node, "maximumDistance"))) + "&deg;" + 
						 self.space(3) + "AzGap: " + '{0:.0f}'.format(float(parseQML.getValueOfFirstChildWithName(origin_quality_node, "azimuthalGap"))) + "&deg;" + 
						 self.space(3) + "2ndAzGap: " + '{0:.0f}'.format(float(parseQML.getValueOfFirstChildWithName(origin_quality_node, "secondaryAzimuthalGap"))) + "&deg;")
		# amplitdue attenuation   20140217 AJL - added
		tableData.append(self.space(3) + "atten: " + '{0:.2f}'.format(float(parseQML.getValueOfFirstChildWithName(amplitude_attenuation_power_node, "value"))))
		tableData.append(Tag.FONT(self.space(1) + "[+/-" + '{0:.2f}'.format(float(parseQML.getValueOfFirstChildWithName(amplitude_attenuation_power_node, "uncertainty"))) + "]", tag_class="small"))
		tableRow.append(tableData)
		table.append(tableRow)

		# location uncertainty
		tableRow = Tag.TR(tag_class="text")
		tableRow.append((Tag.TH("Location Uncertainty:")))
		tableData = Tag.TD(tag_class="text")
		# confidence ellipse
		ellipse = parseQML.getValueOfFirstChildWithName(origin_uncertainty_node, "confidenceLevel") + "%ConfEllipse:" + \
			self.space(3) + "minHorUnc: " + '{0:.0f}'.format(minHorizontalUncertainty) + "km" + \
			self.space(3) + "maxHorUnc: " + '{0:.0f}'.format(maxHorizontalUncertainty) + "km" + \
			self.space(3) + "azMaxHorUnc: " + '{0:.0f}'.format(azimuthMaxHorizontalUncertainty) + "&deg;"
		tableData.append(ellipse)
		tableData.append(Tag.HR())
		# confidence ellipsoid
		#  20191112 AJL - QML uses meters !  Corrected: semiMajorAxisLength, semiIntermediateAxisLength, semiMinorAxisLength
		ellipsoid = parseQML.getValueOfFirstChildWithName(confidence_ellipsoid_node, "confidenceLevel") + "%ConfEllipsoid:" + \
			self.space(3) + "semiMajorAxisLength: " + '{0:.0f}'.format(float(parseQML.getValueOfFirstChildWithName(confidence_ellipsoid_node, "semiMajorAxisLength")) / 1000.0) + "km" + \
			self.space(3) + "majorAxisAzimuth: " + '{0:.0f}'.format(float(parseQML.getValueOfFirstChildWithName(confidence_ellipsoid_node, "majorAxisAzimuth"))) + "&deg;" + \
			self.space(3) + "majorAxisPlunge: " + '{0:.0f}'.format(float(parseQML.getValueOfFirstChildWithName(confidence_ellipsoid_node, "majorAxisPlunge"))) + "&deg;" + \
			self.space(3) + "semiIntermediateAxisLength: " + '{0:.0f}'.format(float(parseQML.getValueOfFirstChildWithName(confidence_ellipsoid_node, "semiIntermediateAxisLength")) / 1000.0) + "km" + \
			self.space(3) + "intermediateAxisAzimuth: " + '{0:.0f}'.format(float(parseQML.getValueOfFirstChildWithName(confidence_ellipsoid_node, "intermediateAxisAzimuth"))) + "&deg;" + \
			self.space(3) + "intermediateAxisPlunge: " + '{0:.0f}'.format(float(parseQML.getValueOfFirstChildWithName(confidence_ellipsoid_node, "intermediateAxisPlunge"))) + "&deg;" + \
			self.space(3) + "semiMinorAxisLength: " + '{0:.0f}'.format(float(parseQML.getValueOfFirstChildWithName(confidence_ellipsoid_node, "semiMinorAxisLength")) / 1000.0) + "km"
		tableData.append(ellipsoid)
		tableRow.append(tableData)
		table.append(tableRow)


		tableText.append(Tag.TR(Tag.TD(table, tag_class="wide")))


		#
		# Tsunamigeneic discriminants, assesment and Evaluate event
		#

		if flag_config__event_info__tsunami_write:

			table = Tag.TABLE(tag_class="text")
			table.append(Tag.CAPTION("Tsunamigeneic Assesment:"))
	
			# warning level and T0
			# tsunami discriminants
			tableRow = Tag.TR(tag_class="text")
			tableRow.append((Tag.TH("Discriminants:")))
			tableData = Tag.TD(tag_class="text")
			# evaluate event - probability underwater
			try:
				underWaterColor = "GREY"
				if mag_wl_value >= __ALARM_YELLOW_CUTOFF:
					underWaterColor = self.evaluateLevelColorIncreasing(ipercent_prob_underwater, 1, -1, __UNDERWATER_RED_CUTOFF, __UNDERWATER_YELLOW_CUTOFF, \
																		"GREY", flag_config__event_info__warning_colors_show)
				tableData.append(Tag.FONT("Prob Underwater: ", tag_class="strong"))
				percent_prob_underwater_str = "?"
				if ipercent_prob_underwater < 1:
					percent_prob_underwater_str = "<1%"
				elif ipercent_prob_underwater > 99:
					percent_prob_underwater_str = ">99%"
				else:
					percent_prob_underwater_str = str(ipercent_prob_underwater) + "%"
				tableData.append(Tag.FONT(percent_prob_underwater_str, tag_class="strong"))
				# tableData.append(Tag.FONT(self.space(1) + "[+/-" + str(ipercent_prob_underwater_std_dev) + "%]", tag_class="small"))
				if mag_wl_value >= __ALARM_YELLOW_CUTOFF:
					tableData.append(Tag.FONT(Tag.FONT(" " + underWaterColor, tag_class="strong"), tag_class="bg" + underWaterColor))
			except Exception as e:
				tableData.append(Tag.FONT("error: evaluating probability underwater: " + str(e)))
			# TdT50Ex warning level
			nWL = int(min(num_wl, num_Td))
			tableData.append(Tag.BR())
			tableData.append(Tag.FONT("T50Ex: " + mag_T50Ex_real))
			tableData.append(Tag.FONT(" [" + num_T50Ex + "]", tag_class="small"))
			tableData.append(Tag.FONT(self.space(3) + "Td: " + mag_Td_real + "s"))
			tableData.append(Tag.FONT(" [" + num_Td + "]", tag_class="small"))
			tableData.append(Tag.FONT(self.space(3) + "Td*T50Ex: " + mag_wl_real, tag_class="strong"))
			tableData.append(Tag.FONT(" [" + str(nWL) + "] ", tag_class="small"))
			tableData.append(Tag.FONT(Tag.FONT(" " + wlColor, tag_class="strong"), tag_class="bg" + wlColor))
			tableData.append(Tag.FONT(self.space(3) + "To: " + mag_T0_real + "s", tag_class="strong"))
			tableData.append(Tag.FONT(" [" + num_T0 + "]", tag_class="small"))
			# evaluate epicenter - plate boundary
			try:
				tableData.append(Tag.BR())
				tableData.append(Tag.FONT("Closest plate boundaries: "))
				for closest_plate_boundary_distance_min, closest_plate_boundary_code, closest_plate_boundary_class, closest_boundary_class_id in closest_plate_boundaries:
					tableData.append(Tag.BR())
					closest_plate_boundary_distance_min_str = str(int(0.5 + closest_plate_boundary_distance_min)) + "km"
					plate_boundary_identifiers, plate_boundary_split_chr = evaluateEvent.evaluatePlateBoundaryCode(closest_plate_boundary_code)
					tableData.append(Tag.FONT(self.space(3) + plate_boundary_identifiers[0] + " " + plate_boundary_split_chr + " " + plate_boundary_identifiers[1] + \
						" [" + closest_plate_boundary_class + "]: distance: ~" + closest_plate_boundary_distance_min_str))
			except Exception as e:
				tableData.append(Tag.FONT("error: evaluating plate boundary: " + str(e)))
			try:
				# slab geometry
				tableData.append(Tag.BR())
				tableData.append(Tag.FONT("Subduction geometry: "))
				slab_depth_str = "unknown"
				slab_depth_error = None
				slabDepthColor = "GREY"
				if is_inside_slab:
					# print "DEBUG: slab_depth", slab_depth, "EvaluateEvent.SLAB_DEPTH_ERROR:", EvaluateEvent.SLAB_DEPTH_ERROR
					if slab_depth != EvaluateEvent.SLAB_DEPTH_ERROR:
						try:
							slab_depth_str = str(int(0.5 + slab_depth)) + "km"
							if mag_wl_value >= __ALARM_YELLOW_CUTOFF:
								slabDepthColor = self.evaluateLevelColorDecreasing(slab_depth, 1, -1, __SLAB_DEPTH_RED_CUTOFF, __SLAB_DEPTH_YELLOW_CUTOFF, \
																				   "GREY", flag_config__event_info__warning_colors_show)
						except Exception as e:  # slab_depth NaN, infinite, ...
							# slab_depth_str = "unknown: " + str(e)
							slab_depth_error = "error: evaluating slab depth: "
							slab_depth_str = str(slab_depth)
							slab_depth = EvaluateEvent.SLAB_DEPTH_ERROR
					else:
						slab_depth_error = "error: evaluating slab depth: "
						slab_depth_str = str(slab_depth)
					tableData.append(Tag.FONT(self.space(3) + "Zone: "))
					tableData.append(Tag.FONT(slab_name, tag_class="strong"))
					tableData.append(Tag.FONT(self.space(3) + "Slab depth at epicenter: "))
					if slab_depth_error is not None:
						tableData.append(Tag.FONT(slab_depth_error, tag_class="strong"))
					tableData.append(Tag.FONT(slab_depth_str, tag_class="strong"))
					# if mag_wl_value >= __ALARM_YELLOW_CUTOFF and slabDepthColor is not None:
					#	tableData.append(Tag.FONT(Tag.FONT(" " + slabDepthColor, tag_class="strong"), tag_class="bg" + slabDepthColor))
				else:
					tableData.append(Tag.FONT("none or not available"))
			except Exception as e:
				tableData.append(Tag.FONT("error: evaluating slab geometry: " + str(e)))
			tableRow.append(tableData)
			table.append(tableRow)
			
			# tsunami assesment
			if flag_config__event_info__tsunami_evaluation_write:
				tableRow = Tag.TR(tag_class="text")
				tableRow.append(Tag.TH("Tsunami evaluation:"))
				tableData = Tag.TD(tag_class="text")
				tableData.append(tsunamiTextList[1])
				tableRow.append(tableData)
				table.append(tableRow)
				
			# set Mwp values
			mag_Mwp_value = 0.0
			try:
				mag_Mwp_value = float(mag_Mwp_real.rstrip("X"))
			except:
				mag_Mwp_value = 0.0
			inum_Mwp = 0
			try:
				inum_Mwp = int(num_Mwp)
			except:
				inum_Mwp = 0
	
			# decision diagram
			if flag_config__event_info__tsunami_decision_table_write:
				tableRow = Tag.TR(tag_class="text")
				tableRow.append(Tag.TH("Decision table (TEST!):"))
				tableData = Tag.TD(tag_class="text")
				try:
					tableData.append(self.writeHtmlDecisionDiagram(ipercent_prob_underwater, percent_prob_underwater_str,
														mag_wl_value, inum_wl, mag_wl_display,
														mag_tdt0_value, inum_tdt0, mag_tdt0_display,
														  closest_plate_boundaries,
														  is_inside_slab, slab_depth, slab_depth_str,
														  mag_Mwp_value, inum_Mwp, mag_Mwp_display,
														  hypo_depth, hypo_depth_str,
														  maxHorizontalUncertainty))
				except Exception as e:
					print("error: invoking writeHtmlDecisionDiagram:", str(e))
				tableRow.append(tableData)
				table.append(tableRow)
	
			# tsunamiLearn and ruptureLearn assessment
			if flag_config__event_info__tsunami_rupture_predictions_write:
				tableRow = Tag.TR(tag_class="text")
				tableRow.append(Tag.TH("Tsunami and Rupture predictions (TEST!):"))
				tableData = Tag.TD(tag_class="text")
				if inum_Mwp >= __MIN_NUM_MWP_FOR_TSUNAMI_RUPTURE_PREDICTION and mag_Mwp_value >= __MIN_MWP_FOR_TSUNAMI_RUPTURE_PREDICTION:
					hypolist_file = instance_path_name + "/hypos"
					predictor = rupturePredict.RupturePredict()
					if len(mech_root_nodes_list) > 0:
						# read Ndip from prefferred mechanism
						mech_type, mech_root_node = mech_root_nodes_list[0]
						mech_file = instance_path_name + '/events/hypo.' + event_id + '.mech.' + mech_type + '.preferred.axes'
						try:
							f_mech = open(mech_file, "r")
							f_mech.readline()  # header
							line = f_mech.readline()
							tokens  = line.split()
							Ndip = tokens[5]
						except:
							Ndip = 45.0  # intermediate value
						for process_type, pkl_file_root in [ \
															('It', fconfig__event_info__it_pkl_file_root), \
															('At', config__event_info__at_pkl_file_root), \
															('ff_duration', config__event_info__ff_duration_pkl_file_root), \
															('ff_rupture_area', config__event_info__ff_rupture_area_pkl_file_root), \
															]:
							#print('DEBUG: Try: process_type:', process_type, 'pkl_file_root:', pkl_file_root)
							#print('DEBUG: process_type, hypolist_file, event_id, pkl_file_root, ipercent_prob_underwater, Ndip', process_type, hypolist_file, event_id, pkl_file_root, ipercent_prob_underwater, Ndip)
							if (not pkl_file_root == None):
								prediction = predictor.predict(process_type, hypolist_file, event_id, pkl_file_root, ipercent_prob_underwater, Ndip)
								#print('DEBUG: Out: process_type:', process_type, 'prediction:', prediction, 'ipercent_prob_underwater:', ipercent_prob_underwater, 'Ndip:', Ndip)
								text, fmt, units = tsunami_rupture_predictions_text[process_type]
								tableData.append(text + ': ' + fmt.format(float(prediction)) + units)
								tableData.append(Tag.BR())
				else:
					tableData.append('No prediction: Event too small (Mwp<' \
									 + '{0:.1f}'.format(__MIN_MWP_FOR_TSUNAMI_RUPTURE_PREDICTION)  \
									+ ') or too few Mwp readings (N<' \
									 + str(__MIN_NUM_MWP_FOR_TSUNAMI_RUPTURE_PREDICTION)  \
									 + ')')
					tableData.append(Tag.BR())
				
				tableRow.append(tableData)
				table.append(tableRow)
	
	
			tableText.append(Tag.TR(Tag.TD(table, tag_class="wide")))


		tableEarthquakeInfoRow.append(Tag.TD(tableText, tag_class="wide"))


		# event location map
		if flag_config__event_info__location_map_write:
			tableData = Tag.TD()
			image_file = "./hypo." + event_id + ".map.jpg"
			tableData.append(Tag.A(Tag.IMG(image_file, image_file, width="80%"), image_file))
			tableEarthquakeInfoRow.append(tableData)


		tableEarthquakeInfo.append(tableEarthquakeInfoRow)
		docBody.append(tableEarthquakeInfo)


		docBody.append(Tag.BR())


		#
		# first motion mechanism
		#

		if flag_config__event_info__first_motion_mech_write:
			
			tableMechanism = Tag.TABLE()

			for (mech_type, mech_root_node) in mech_root_nodes_list :
				# set event-origin node
				mech_node = parseQML.getFirstChildWithName(mech_root_node, "FocalMechanism")
				nodel_planes_node = parseQML.getFirstChildWithName(mech_node, "nodalPlanes")
				nodel_plane1_node = parseQML.getFirstChildWithName(nodel_planes_node, "nodalPlane1")
				nodel_plane2_node = parseQML.getFirstChildWithName(nodel_planes_node, "nodalPlane2")
		
				qualityCode = None
				qualityCode = parseQML.getValueOfFirstChildWithName(mech_node, "hash:qualityCode")
				if qualityCode == None :  # may be other type
					qualityCode = parseQML.getValueOfFirstChildWithName(mech_node, "fmamp:qualityCode")
				quality_message = ""
				if qualityCode == "A":
					quality_message = Tag.FONT()
				elif qualityCode == "B":
					quality_message = Tag.FONT()
				elif qualityCode == "C":
					quality_message = Tag.FONT(self.space(4) + "(Warning: Unreliable solution, qualityCode=C)", tag_class="warning")
				elif qualityCode == "D":
					quality_message = Tag.FONT(self.space(4) + "(Warning: VERY UNRELIABLE SOLUTION, qualityCode=D)", tag_class="warning")
				else:
					quality_message = Tag.FONT(self.space(4) + "(No solution available)", tag_class="strong")
		
				tableMechanismRow = Tag.TR()
		
				table = Tag.TABLE(tag_class="text")
				caption = Tag.CAPTION("First-motion Focal Mechanism (" + str(mech_type).upper() + "):")
				caption.append(quality_message)
				table.append(caption)
				tableRow = Tag.TR(tag_class="text")
				tableRow.append((Tag.TH("Nodal&nbsp;Plane&nbsp;1:")))  # &nbsp; prevents newline in tabe heading
				tableData = Tag.TD(tag_class="text")
				tableData.append("strike: " + '{0:.0f}'.format(float(parseQML.getValueInFirstChildWithName(nodel_plane1_node, "strike", "value"))) + "&deg;" + 
								 self.space(3) + "dip: " + '{0:.0f}'.format(float(parseQML.getValueInFirstChildWithName(nodel_plane1_node, "dip", "value"))) + "&deg;" + 
								 self.space(3) + "rake: " + '{0:.0f}'.format(float(parseQML.getValueInFirstChildWithName(nodel_plane1_node, "rake", "value"))) + "&deg;"
								 )
				tableRow.append(tableData)
				table.append(tableRow)
				#
				tableRow = Tag.TR(tag_class="text")
				tableRow.append((Tag.TH("Nodal&nbsp;Plane&nbsp;2:")))  # &nbsp; prevents newline in tabe heading
				tableData = Tag.TD(tag_class="text")
				tableData.append("strike: " + '{0:.0f}'.format(float(parseQML.getValueInFirstChildWithName(nodel_plane2_node, "strike", "value"))) + "&deg;" + 
								 self.space(3) + "dip: " + '{0:.0f}'.format(float(parseQML.getValueInFirstChildWithName(nodel_plane2_node, "dip", "value"))) + "&deg;" + 
								 self.space(3) + "rake: " + '{0:.0f}'.format(float(parseQML.getValueInFirstChildWithName(nodel_plane2_node, "rake", "value"))) + "&deg;"
								 )
				tableRow.append(tableData)
				table.append(tableRow)
				#
				tableRow = Tag.TR(tag_class="text")
				tableRow.append((Tag.TH("Quality:")))
				tableData = Tag.TD(tag_class="text")
				tableData.append("azimuthalGap: " + '{0:.0f}'.format(float(parseQML.getValueOfFirstChildWithName(mech_node, "azimuthalGap"))) + "&deg;" + 
								 self.space(3) + "stationPolarityCount: " + parseQML.getValueOfFirstChildWithName(mech_node, "stationPolarityCount")
								 )
				# fmamp specific tags
				try :
					tableData.append(
								 self.space(3) + "misfitWeightCount: " + '{0:.1f}'.format(float(parseQML.getValueOfFirstChildWithName(mech_node, "fmamp:misfitWeightCount")))
									 )
				except :  # may be other type
					pass
				tableData.append(self.space(3) + "misfit: " + '{0:.2f}'.format(float(parseQML.getValueOfFirstChildWithName(mech_node, "misfit"))) + 
								 self.space(3) + "stationDistributionRatio: " + '{0:.2f}'.format(float(parseQML.getValueOfFirstChildWithName(mech_node, "stationDistributionRatio")))
								 )
				# HASH specific tags
				try :
					tableData.append(self.space(3) + "rmsAngDiffAccPref: " + '{0:.2f}'.format(float(parseQML.getValueOfFirstChildWithName(mech_node, "hash:rmsAngDiffAccPref"))) + 
									 self.space(3) + "fracAcc30degPref: " + '{0:.2f}'.format(float(parseQML.getValueOfFirstChildWithName(mech_node, "hash:fracAcc30degPref")))
									 )
				except :  # may be other type
					pass
				# fmamp specific tags
				try :
					tableData.append(
								 self.space(3) + "meanDistP: " + '{0:.1f}'.format(float(parseQML.getValueOfFirstChildWithName(mech_node, "fmamp:meanDistP")))
									 )
				except :  # may be other type
					pass
				try :
					tableData.append(
								 self.space(3) + "meanDistT: " + '{0:.1f}'.format(float(parseQML.getValueOfFirstChildWithName(mech_node, "fmamp:meanDistT")))
									 )
				except :  # may be other type
					pass
				tableData.append(self.space(3) + "qualityCode: " + qualityCode
								 )
				tableRow.append(tableData)
				table.append(tableRow)
				#
				tableRow = Tag.TR(tag_class="text")
				tableRow.append((Tag.TH("Method:")))
				tableData = Tag.TD(tag_class="text")
				tableData.append("methodID: " + parseQML.getValueOfFirstChildWithName(mech_node, "methodID")
								 )
				tableRow.append(tableData)
				table.append(tableRow)
		
		
				tableMechanismRow.append(Tag.TD(table, tag_class="wide"))
		
		
				tableData = Tag.TD()
				image_file = "./hypo." + event_id + ".mech." + mech_type + ".mechanism.jpg"
				tableData.append(Tag.A(Tag.IMG(image_file, image_file, width="40%"), image_file))
				tableMechanismRow.append(tableData)
		
		
				tableMechanism.append(tableMechanismRow)
				
			docBody.append(tableMechanism)


		paragraph = Tag.P()
		paragraph.append("Event ID: " + publicID)
		paragraph.append(Tag.BR())
		paragraph.append("Event info last updated: " + event_info_last_update_time + " UTC")
		paragraph.append(Tag.BR())
		creation_info_node = parseQML.getFirstChildWithName(event_root_node, "creationInfo")
		creation_time = parseQML.getValueOfFirstChildWithName(creation_info_node, "creationTime")
		creation_time = parseQML.formatTime(creation_time, 0)
		paragraph.append("Event first association/location: " + creation_time + " UTC")
		paragraph.append(Tag.BR())
		paragraph.append("Software: " + parseQML.getValueOfFirstChildWithName(creation_info_node, "version"))
		paragraph.append(Tag.BR())
		paragraph.append("Agency: " + parseQML.getValueOfFirstChildWithName(creation_info_node, "agencyID"))
		docBody.append(paragraph)

		f_html_event_out = open(instance_path_name + "/events/hypo." + event_id + ".html", "w")
		document.writeHtml(f_html_event_out)
		f_html_event_out.close()

		# from sys import stdout
		# document.writeHtml(stdout)



	def mech2qml(self, f_mech_in, origin_public_id, event_parameters_public_id, event_public_id, event_id, type):

		# read mechanism file to a dictionary
		mech_dict = dict()
		for line in f_mech_in:
			# print line
			tokens = []
			tokens = line.split()
			mech_dict[tokens[0]] = tokens[1]
			# print tokens[0] + "=" + tokens[1]
			# print tokens[0] + "=" + mech_dict[tokens[0]]


		# dom = xml.dom.getDOMImplementation()
		# doc = dom.createDocument ("http://quakeml.org/xmlns/bed/1.2", "quakeml", None)
		doc = xml.dom.minidom.Document ()
		quakeml = doc.createElement("q:quakeml")
		quakeml.setAttribute("xmlns", "http://quakeml.org/xmlns/bed/1.2")
		quakeml.setAttribute("xmlns:q", "http://quakeml.org/xmlns/quakeml/1.2")
		quakeml.setAttribute("xmlns:ee", "http://net.alomax/earlyest/xmlns/ee")
		if type == MECH_HASH:
			quakeml.setAttribute("xmlns:hash", "http://net.alomax/earlyest/xmlns/hash")
		elif type == MECH_FMAMP_POL or type == MECH_FMAMP_POL_WAVEFORM or type == MECH_FMAMP_AMP_AREF or type == MECH_FMAMP_AMP_MWP:
			quakeml.setAttribute("xmlns:fmamp", "http://net.alomax/earlyest/xmlns/fmamp")
		# quakeml = doc.documentElement
		# <q:quakeml xmlns="http://quakeml.org/xmlns/bed/1.2" 
		 #  xmlns:q="http://quakeml.org/xmlns/quakeml/1.2"
		 #  xmlns:ext="http://example.com/ext"
		 #  ext:foo="bar">

		# EventParameters
		eventParameters = doc.createElement("EventParameters")
		eventParameters.setAttribute("publicID", event_parameters_public_id)
		quakeml.appendChild(eventParameters)

		# Event
		event = doc.createElement("Event")
		event.setAttribute("publicID", event_public_id)
		eventParameters.appendChild(event)

		# FocalMechanism
		focalMechanism = doc.createElement("FocalMechanism")
		focalMechanism.setAttribute("publicID", origin_public_id + "/focal_mechanism/" + type)
		event.appendChild(focalMechanism)

		# triggeringOriginID
		triggeringOriginID = doc.createElement("triggeringOriginID")
		textNode = doc.createTextNode(origin_public_id)
		triggeringOriginID.appendChild(textNode)
		focalMechanism.appendChild(triggeringOriginID)

		# nodalPlanes
		nodalPlanes = doc.createElement("nodalPlanes")
		focalMechanism.appendChild(nodalPlanes)
		#
		nodalPlane1 = doc.createElement("nodalPlane1")
		nodalPlanes.appendChild(nodalPlane1)
		#
		strike = doc.createElement("strike")
		nodalPlane1.appendChild(strike)
		value = doc.createElement("value")
		textNode = doc.createTextNode(mech_dict["nodalPlanes.nodalPlane1.strike"])
		value.appendChild(textNode)
		strike.appendChild(value)
		#
		dip = doc.createElement("dip")
		nodalPlane1.appendChild(dip)
		value = doc.createElement("value")
		textNode = doc.createTextNode(mech_dict["nodalPlanes.nodalPlane1.dip"])
		value.appendChild(textNode)
		dip.appendChild(value)
		#
		rake = doc.createElement("rake")
		nodalPlane1.appendChild(rake)
		value = doc.createElement("value")
		textNode = doc.createTextNode(mech_dict["nodalPlanes.nodalPlane1.rake"])
		value.appendChild(textNode)
		rake.appendChild(value)
		#
		nodalPlane2 = doc.createElement("nodalPlane2")
		nodalPlanes.appendChild(nodalPlane2)
		#
		strike = doc.createElement("strike")
		nodalPlane2.appendChild(strike)
		value = doc.createElement("value")
		textNode = doc.createTextNode(mech_dict["nodalPlanes.nodalPlane2.strike"])
		value.appendChild(textNode)
		strike.appendChild(value)
		#
		dip = doc.createElement("dip")
		nodalPlane2.appendChild(dip)
		value = doc.createElement("value")
		textNode = doc.createTextNode(mech_dict["nodalPlanes.nodalPlane2.dip"])
		value.appendChild(textNode)
		dip.appendChild(value)
		#
		rake = doc.createElement("rake")
		nodalPlane2.appendChild(rake)
		value = doc.createElement("value")
		textNode = doc.createTextNode(mech_dict["nodalPlanes.nodalPlane2.rake"])
		value.appendChild(textNode)
		rake.appendChild(value)
		#
		preferredPlane = doc.createElement("preferredPlane")
		textNode = doc.createTextNode(mech_dict["nodalPlanes.preferredPlane"])
		preferredPlane.appendChild(textNode)
		nodalPlanes.appendChild(preferredPlane)

		# azimuthalGap
		azimuthalGap = doc.createElement("azimuthalGap")
		textNode = doc.createTextNode(mech_dict["azimuthalGap"])
		azimuthalGap.appendChild(textNode)
		focalMechanism.appendChild(azimuthalGap)

		# stationPolarityCount
		stationPolarityCount = doc.createElement("stationPolarityCount")
		textNode = doc.createTextNode(mech_dict["stationPolarityCount"])
		stationPolarityCount.appendChild(textNode)
		focalMechanism.appendChild(stationPolarityCount)

		# stationDistributionRatio
		stationDistributionRatio = doc.createElement("stationDistributionRatio")
		textNode = doc.createTextNode(mech_dict["stationDistributionRatio"])
		stationDistributionRatio.appendChild(textNode)
		focalMechanism.appendChild(stationDistributionRatio)

		# methodID
		methodID = doc.createElement("methodID")
		textNode = doc.createTextNode(mech_dict["methodID"])
		methodID.appendChild(textNode)
		focalMechanism.appendChild(methodID)
		
		if type == MECH_HASH:
			# HASH specific tags
			# rmsAngDiffAccPref (Note: Non-standard extension to QuakeML!!!)
			rmsAngDiffAccPref = doc.createElement("hash:rmsAngDiffAccPref")
			textNode = doc.createTextNode(mech_dict["rmsAngDiffAccPref"])
			rmsAngDiffAccPref.appendChild(textNode)
			focalMechanism.appendChild(rmsAngDiffAccPref)
			# fracAcc30degPref (Note: Non-standard extension to QuakeML!!!)
			fracAcc30degPref = doc.createElement("hash:fracAcc30degPref")
			textNode = doc.createTextNode(mech_dict["fracAcc30degPref"])
			fracAcc30degPref.appendChild(textNode)
			focalMechanism.appendChild(fracAcc30degPref)
			# qualityCode (Note: Non-standard extension to QuakeML!!!)
			qualityCode = doc.createElement("hash:qualityCode")
			textNode = doc.createTextNode(mech_dict["qualityCode"])
			qualityCode.appendChild(textNode)
			focalMechanism.appendChild(qualityCode)
			# misfit
			misfit = doc.createElement("misfit")
			textNode = doc.createTextNode(mech_dict["misfit"])
			misfit.appendChild(textNode)
			focalMechanism.appendChild(misfit)

		elif  type == MECH_FMAMP_POL or type == MECH_FMAMP_POL_WAVEFORM or type == MECH_FMAMP_AMP_AREF or type == MECH_FMAMP_AMP_MWP:
			# fmamp specific tags
			# qualityCode (Note: Non-standard extension to QuakeML!!!)
			qualityCode = doc.createElement("fmamp:qualityCode")
			textNode = doc.createTextNode(mech_dict["qualityCode"])
			qualityCode.appendChild(textNode)
			focalMechanism.appendChild(qualityCode)
			# misfit
			misfit = doc.createElement("misfit")
			if  type == MECH_FMAMP_POL or type == MECH_FMAMP_POL_WAVEFORM:
				textNode = doc.createTextNode(mech_dict["polarityMisfit"])
			else :
				textNode = doc.createTextNode(mech_dict["amplitudeMisfit"])
			misfit.appendChild(textNode)
			focalMechanism.appendChild(misfit)
			# misfitWeight (Note: Non-standard extension to QuakeML!!!)
			misfitWeight = doc.createElement("fmamp:misfitWeightCount")
			try :
				if  type == MECH_FMAMP_POL or type == MECH_FMAMP_POL_WAVEFORM:
					textNode = doc.createTextNode(mech_dict["sumPolarityMisfitWeight"])
				else :
					textNode = doc.createTextNode(mech_dict["sumAmplitudeMisfitWeight"])
				misfitWeight = doc.createElement("fmamp:misfitWeightCount")
				misfitWeight.appendChild(textNode)
				focalMechanism.appendChild(misfitWeight)
			except :  # may be other type
					pass
			# meanDistP (Note: Non-standard extension to QuakeML!!!)
			meanDistP = doc.createElement("fmamp:meanDistP")
			textNode = doc.createTextNode(mech_dict["meanDistP"])
			meanDistP.appendChild(textNode)
			focalMechanism.appendChild(meanDistP)
			# meanDistT (Note: Non-standard extension to QuakeML!!!)
			meanDistT = doc.createElement("fmamp:meanDistT")
			textNode = doc.createTextNode(mech_dict["meanDistT"])
			meanDistT.appendChild(textNode)
			focalMechanism.appendChild(meanDistT)

		doc.appendChild(quakeml)

		return doc



	def doFocalMechInversion(self, instance_path_name):

		import os
		import shlex
		import subprocess

		# cwd_path = os.getcwd()
		cwd_path = "."

		try:
			os.mkdir(instance_path_name + "/events",)
		except OSError:  # directory exists
			pass

		# pre-processing
		if flag_config__event_info__first_motion_mech_run_hash :
			try :
				# HASH focal mechanism calculation and plotting =======================
				t1 = time.time()
				subprocess_cmd = cwd_path + "/hash_driver_early_est.bash " + instance_path_name
				print(subprocess_cmd)
				args = shlex.split(subprocess_cmd)
				retcode = subprocess.call(args)
				print("DEBUG: TIME: run: hash_driver_early_est.bash: total time = %0.3fs" % ((time.time() - t1)))
			except Exception as e:
				print("error: running hash_driver_early_est.bash:", e)
		if flag_config__event_info__first_motion_mech_run_fmamp_pol :
			try :
				# fmamp focal mechanism calculation and plotting =======================
				t1 = time.time()
				subprocess_cmd = cwd_path + "/fmamp_driver_early_est_POL.bash " + instance_path_name
				print(subprocess_cmd)
				args = shlex.split(subprocess_cmd)
				retcode = subprocess.call(args)
				print("DEBUG: TIME: run: fmamp_driver_early_est_POL.bash: total time = %0.3fs" % ((time.time() - t1)))
			except Exception as e:
				print("error: running fmamp_driver_early_est_POL.bash:", e)
		if flag_config__event_info__first_motion_mech_run_fmamp_pol_waveform :
			try :
				# fmamp focal mechanism calculation and plotting =======================
				t1 = time.time()
				subprocess_cmd = cwd_path + "/fmamp_driver_early_est_POLWF.bash " + instance_path_name
				print(subprocess_cmd)
				args = shlex.split(subprocess_cmd)
				retcode = subprocess.call(args)
				print("DEBUG: TIME: run: fmamp_driver_early_est_POLWF.bash: total time = %0.3fs" % ((time.time() - t1)))
			except Exception as e:
				print("error: running fmamp_driver_early_est_POLWF.bash:", e)
		if flag_config__event_info__first_motion_mech_run_fmamp_amp :
			try :
				# fmamp focal mechanism calculation and plotting =======================
				t1 = time.time()
				subprocess_cmd = cwd_path + "/fmamp_driver_early_est_AMP_AREF.bash " + instance_path_name
				print(subprocess_cmd)
				args = shlex.split(subprocess_cmd)
				retcode = subprocess.call(args)
				print("DEBUG: TIME: run: fmamp_driver_early_est_AMP_AREF.bash: total time = %0.3fs" % ((time.time() - t1)))
			except Exception as e:
				print("error: running fmamp_driver_early_est_AMP_AREF.bash:", e)
			try :
				# fmamp focal mechanism calculation and plotting =======================
				t1 = time.time()
				subprocess_cmd = cwd_path + "/fmamp_driver_early_est_AMP_MWP.bash " + instance_path_name
				print(subprocess_cmd)
				args = shlex.split(subprocess_cmd)
				retcode = subprocess.call(args)
				print("DEBUG: TIME: run: fmamp_driver_early_est_AMP_MWP.bash: total time = %0.3fs" % ((time.time() - t1)))
			except Exception as e:
				print("error: running fmamp_driver_early_est_AMP_MWP.bash:", e)


	def processEvents(self, save_dir, root_out_path, instance_path_name, instance_name, plot_map_mechanism_type):

		import os
		import shlex
		import subprocess

		# cwd_path = os.getcwd()
		cwd_path = "."

		try:
			os.mkdir(instance_path_name + "/events",)
		except OSError:  # directory exists
			pass

		# parse events in monitor.xml, run plot_event_info_GMTX.gmt, write mechanism qml and event html
		# parse monitor.xml
		t1 = time.time()
		print("Start parse of: " + instance_path_name + '/monitor.xml')
		f_xml = open(instance_path_name + "/monitor.xml", "r")
		monitor_xml = xml.dom.minidom.parse(f_xml)  # parse an XML file
		print("DEBUG: TIME: parse of: monitor.xml: total time = %0.3fs" % ((time.time() - t1)))
		t0 = time.time()
		parseQML = ParseQML()
		nhypo = 0;
		for node0 in monitor_xml.childNodes:
			if (node0.nodeName == "q:quakeml"):
				for node1 in node0.childNodes:
					event_parameters_public_id = "ERROR"
					if (node1.nodeName == "eventParameters"):
						creation_info_node = parseQML.getFirstChildWithName(node1, "creationInfo")
						event_info_last_update_time = parseQML.getValueOfFirstChildWithName(creation_info_node, "creationTime")
						event_info_last_update_time = parseQML.formatTime(event_info_last_update_time, 0)
						agencyId = parseQML.getValueOfFirstChildWithName(creation_info_node, "agencyID")
						# resource_id_root ="smi:" + str(agencyId) + "/ee"
						event_parameters_public_id = parseQML.getAtribute(node1, "publicID")
						#resource_id_root = event_parameters_public_id[0:event_parameters_public_id.find("/event_parameters")]
						for node2 in node1.childNodes:
							event_public_id = "ERROR"
							if (node2.nodeName == "event"):
								t1 = time.time()
								event_node = node2
								event_public_id = parseQML.getAtribute(event_node, "publicID")
								dummy1, dummy2, event_id = event_public_id.rpartition('/')
								# get basic event description
								origin_time = False
								origin_public_id = "ERROR"
								for node3 in event_node.childNodes:
									if (node3.nodeName == "origin"):
										origin_public_id = parseQML.getAtribute(node3, "publicID")
										region = parseQML.getValueOfFirstChildWithName(node3, "region")
										for node4 in node3.childNodes:
											if (node4.nodeName == "time"):
												origin_time = parseQML.getValueOfFirstChildWithName(node4, "value")
								# check if empty event (e.g. type="not existing")  # 20221017 AJL -  - added to support reporting of cancelled events in xml
								if not origin_time:
									continue
								# run plot_event_info_GMTX.gmt
								plot_mechanisms = "NO"
								if flag_config__event_info__first_motion_mech_run:
									plot_mechanisms = "YES"
								subprocess_cmd = cwd_path + "/plot_event_info_GMT4.gmt " + save_dir + " " + root_out_path + " " + instance_path_name + " " + instance_name \
								+ " " + event_id + " " + region.replace(" ", "_") + " " + origin_time + " " + plot_mechanisms + " " + plot_map_mechanism_type
								print(subprocess_cmd)
								args = shlex.split(subprocess_cmd)
								t2 = time.time()
								retcode = subprocess.call(args)
								print("DEBUG: TIME: plot_event_info_GMT4.gmt: total time = %0.3fs" % ((time.time() - t2)))
								# write mechanism qml
								mech_root_nodes_list = []
								# order of event types here determines order displayed in HTML event page, see also mech type for map plotting in plot_warning_report_GMTX.gmt, plot_event_info_GMTX.gmt
								# check for fmamp polarity mechanism
								fmamp_mech_doc = None
								try:
									# read key / value data
									fname = instance_path_name + "/events/hypo." + event_id + ".mech.fmamp_polarity.qml_data"
									f_mech_in = open(fname, "r")
									fmamp_mech_doc = self.mech2qml(f_mech_in, origin_public_id, event_parameters_public_id, event_public_id, event_id, MECH_FMAMP_POL)
									f_mech_in.close()
									mech_root_nodes_list.append(("fmamp_polarity", fmamp_mech_doc))
									# write qml
									fname = instance_path_name + "/events/hypo." + event_id + ".mech.fmamp_polarity.xml"
									f_mech_out = open(fname, "w")
									f_mech_out.write(fmamp_mech_doc.toprettyxml("   "))
									f_mech_out.close()
								#except:  # probably no mechanism solution available
								except Exception as e:
									print("error: generating mech.fmamp_polarity.xml:", str(e))
									pass
								# check for fmamp waveform polarity mechanism
								fmamp_mech_doc = None
								try:
									# read key / value data
									fname = instance_path_name + "/events/hypo." + event_id + ".mech.fmamp_polarity_waveform.qml_data"
									f_mech_in = open(fname, "r")
									fmamp_mech_doc = self.mech2qml(f_mech_in, origin_public_id, event_parameters_public_id, event_public_id, event_id, MECH_FMAMP_POL_WAVEFORM)
									f_mech_in.close()
									mech_root_nodes_list.append(("fmamp_polarity_waveform", fmamp_mech_doc))
									# write qml
									fname = instance_path_name + "/events/hypo." + event_id + ".mech.fmamp_polarity_waveform.xml"
									f_mech_out = open(fname, "w")
									f_mech_out.write(fmamp_mech_doc.toprettyxml("   "))
									f_mech_out.close()
								#except:  # probably no mechanism solution available
								except Exception as e:
									print("error: generating mech.fmamp_polarity_waveform.xml:", str(e))
									pass
								# check for HASH mechanism
								hash_mech_doc = None
								try:
									# read key / value data
									fname = instance_path_name + "/events/hypo." + event_id + ".mech.hash.qml_data"
									f_mech_in = open(fname, "r")
									hash_mech_doc = self.mech2qml(f_mech_in, origin_public_id, event_parameters_public_id, event_public_id, event_id, MECH_HASH)
									f_mech_in.close()
									mech_root_nodes_list.append(("hash", hash_mech_doc))
									# write qml
									fname = instance_path_name + "/events/hypo." + event_id + ".mech.hash.xml"
									f_mech_out = open(fname, "w")
									f_mech_out.write(hash_mech_doc.toprettyxml("   "))
									# hash_mech_doc.writexml(f_mech_out, "   ")
									f_mech_out.close()
								#except:  # probably no HASH mechanism solution available
								except Exception as e:
									print("error: generating mech.hash.xml:", str(e))
									pass
								# check for fmamp amplitude mechanism
								fmamp_mech_doc = None
								try:
									# read key / value data
									fname = instance_path_name + "/events/hypo." + event_id + ".mech.fmamp_amp_aref.qml_data"
									f_mech_in = open(fname, "r")
									fmamp_mech_doc = self.mech2qml(f_mech_in, origin_public_id, event_parameters_public_id, event_public_id, event_id, MECH_FMAMP_AMP_AREF)
									f_mech_in.close()
									mech_root_nodes_list.append(("fmamp_amp_aref", fmamp_mech_doc))
									# write qml
									fname = instance_path_name + "/events/hypo." + event_id + ".mech.fmamp_amp_aref.xml"
									f_mech_out = open(fname, "w")
									f_mech_out.write(fmamp_mech_doc.toprettyxml("   "))
									f_mech_out.close()
								except:  # probably no mechanism solution available
									pass
								# check for fmamp amplitude mechanism
								fmamp_mech_doc = None
								try:
									# read key / value data
									fname = instance_path_name + "/events/hypo." + event_id + ".mech.fmamp_amp_mwp.qml_data"
									f_mech_in = open(fname, "r")
									fmamp_mech_doc = self.mech2qml(f_mech_in, origin_public_id, event_parameters_public_id, event_public_id, event_id, MECH_FMAMP_AMP_MWP)
									f_mech_in.close()
									mech_root_nodes_list.append(("fmamp_amp_mwp", fmamp_mech_doc))
									# write qml
									fname = instance_path_name + "/events/hypo." + event_id + ".mech.fmamp_amp_mwp.xml"
									f_mech_out = open(fname, "w")
									f_mech_out.write(fmamp_mech_doc.toprettyxml("   "))
									f_mech_out.close()
								except:  # probably no mechanism solution available
									pass
								# write event to HTML
								t2 = time.time()
								self.writeHtmlEventInfo(instance_path_name, event_id, event_node, mech_root_nodes_list, event_info_last_update_time)
								print("DEBUG: TIME: writeHtmlEventInfo(): total time = %0.3fs" % ((time.time() - t2)))
								self.writeHtmlEventPicks(instance_path_name, event_id)
								nhypo = nhypo + 1
								print("DEBUG: TIME: single event processing: total time = %0.3fs" % ((time.time() - t1)))

		print("DEBUG: TIME: total event processing: total time = %0.3fs" % ((time.time() - t0)))




if __name__ == "__main__":
	import sys
	import time
	
	# get properties file settings
	config = configparser.ConfigParser()
	config.read('process_events.prop')
	flag_config__event_info__warning_colors_show = False
	try:
		flag_config__event_info__warning_colors_show = config.getboolean("EventInfo", "warning.colors.show")
	except:  # not present or syntax error
	   pass
	flag_config__event_info__location_map_write = True
	try:
		flag_config__event_info__location_map_write = config.getboolean("EventInfo", "location_map.write")
	except:  # not present or syntax error
	   pass
	flag_config__event_info__tsunami_write = True
	try:
		flag_config__event_info__tsunami_write = config.getboolean("EventInfo", "tsunami.write")
	except:  # not present or syntax error
	   pass
	flag_config__event_info__tsunami_evaluation_write = False
	try:
		flag_config__event_info__tsunami_evaluation_write = config.getboolean("EventInfo", "tsunami.evaluation.write")
	except:  # not present or syntax error
	   pass
	flag_config__event_info__tsunami_decision_table_write = False
	try:
		flag_config__event_info__tsunami_decision_table_write = config.getboolean("EventInfo", "tsunami.decision_table.write")
	except:  # not present or syntax error
	   pass
	flag_config__event_info__tsunami_decision_table_colors_show = True
	try:
		flag_config__event_info__tsunami_decision_table_colors_show = config.getboolean("EventInfo", "tsunami.decision_table.colors.show")
	except:  # not present or syntax error
	   pass
   # focal mechanism
	flag_config__event_info__first_motion_mech_run_hash = False
	try:
		flag_config__event_info__first_motion_mech_run_hash = config.getboolean("EventInfo", "first_motion_mech.run.hash")
	except:  # not present or syntax error
	   pass
	flag_config__event_info__first_motion_mech_run_fmamp_pol = True
	try:
		flag_config__event_info__first_motion_mech_run_fmamp_pol = config.getboolean("EventInfo", "first_motion_mech.run.fmamp.polarity")
	except:  # not present or syntax error
	   pass
	flag_config__event_info__first_motion_mech_run_fmamp_pol_waveform = False
	try:
		flag_config__event_info__first_motion_mech_run_fmamp_pol_waveform = config.getboolean("EventInfo", "first_motion_mech.run.fmamp.polarity.waveform")
	except:  # not present or syntax error
	   pass
	flag_config__event_info__first_motion_mech_run_fmamp_amp = False
	try:
		flag_config__event_info__first_motion_mech_run_fmamp_amp = config.getboolean("EventInfo", "first_motion_mech.run.fmamp.amplitude")
	except:  # not present or syntax error
	   pass
	flag_config__event_info__first_motion_mech_run = flag_config__event_info__first_motion_mech_run_hash \
		or flag_config__event_info__first_motion_mech_run_fmamp_pol \
		or flag_config__event_info__first_motion_mech_run_fmamp_pol_waveform \
		or flag_config__event_info__first_motion_mech_run_fmamp_amp
	try:
		flag_config__event_info__first_motion_mech_run = config.getboolean("EventInfo", "first_motion_mech.run")
	except:  # not present or syntax error
	   pass
	flag_config__event_info__first_motion_mech_write = flag_config__event_info__first_motion_mech_run
	if flag_config__event_info__first_motion_mech_run:
		try:
			flag_config__event_info__first_motion_mech_write = config.getboolean("EventInfo", "first_motion_mech.write")
		except:  # not present or syntax error
		   pass
	# rupture prediction parameters   #20160712 AJL added
	flag_config__event_info__tsunami_rupture_predictions_write = False
	try:
		flag_config__event_info__tsunami_rupture_predictions_write = config.getboolean("EventInfo", "tsunami.tsunami_rupture_predicitions.write")
	except:  # not present or syntax error
	   pass
	config__event_info__ff_duration_pkl_file_root = None
	#try:
	config__event_info__ff_duration_pkl_file_root = config.get("EventInfo", "tsunami.tsunami_rupture_predicitions.ff_duration.pkl_file_root")
	#except:  # not present or syntax error
	#   pass
	config__event_info__ff_rupture_area_pkl_file_root = None
	try:
		config__event_info__ff_rupture_area_pkl_file_root = config.get("EventInfo", "tsunami.tsunami_rupture_predicitions.ff_rupture_area.pkl_file_root")
	except:  # not present or syntax error
	   pass
	config__event_info__at_pkl_file_root = None
	try:
		config__event_info__at_pkl_file_root = config.get("EventInfo", "tsunami.tsunami_rupture_predicitions.at.pkl_file_root")
	except:  # not present or syntax error
	   pass
	fconfig__event_info__it_pkl_file_root = None
	try:
		fconfig__event_info__it_pkl_file_root = config.get("EventInfo", "tsunami.tsunami_rupture_predicitions.it.pkl_file_root")
	except:  # not present or syntax error
	   pass
 
	print("DEBUG: sys.argv:", sys.argv[0], sys.argv[1], sys.argv[2], \
		sys.argv[3], sys.argv[4], sys.argv[5], sys.argv[6], sys.argv[7])
	process_type = sys.argv[1]
	save_dir = sys.argv[2]
	root_out_path = sys.argv[3]
	instance_path = sys.argv[4]
	instance_path_name = sys.argv[5]
	instance_name = sys.argv[6]
	plot_map_mechanism_type = sys.argv[7]
	
	pe = ProcessEvents()
	if process_type == "MECHANISM":
		if flag_config__event_info__first_motion_mech_run:
			t1 = time.time()
			pe.doFocalMechInversion(instance_path_name)
			print("DEBUG: TIME: ProcessEvents(MECHANISM): total time = %0.3fs" % ((time.time() - t1)))
	elif process_type == "EVENTS":
		t1 = time.time()
		pe.processEvents(save_dir, root_out_path, instance_path_name, instance_name, plot_map_mechanism_type)
		print("DEBUG: TIME: ProcessEvents(EVENTS): total time = %0.3fs" % ((time.time() - t1)))
	else:
		raise ValueError("ProcessEvents: process type must be MECHANISM or EVENTS, is: " + process_type)

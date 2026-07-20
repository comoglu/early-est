# This file is part of the Anthony Lomax Python Library.
# 
# Copyright (C) 2012 Anthony Lomax <anthony@alomax.net www.alomax.net>
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


import os
import sys
import shlex
import subprocess
import math
from struct import unpack

# determine if a point is inside a given polygon or not
# Polygon is a list of (x,y) pairs.
# from: http://www.ariel.com.au/a/python-point-int-poly.html

def pointInsidePolygon(x, y, poly):

    inside = False

    p1x, p1y = poly[0]
    n = len(poly)
    for i in range(n + 1):
        p2x, p2y = poly[i % n]
        if y > min(p1y, p2y):
            if y <= max(p1y, p2y):
                if x <= max(p1x, p2x):
                    if p1y != p2y:
                        if p1x == p2x or x <=  p1x + (y - p1y) * (p2x - p1x) / (p2y - p1y):
                            inside = not inside
        p1x, p1y = p2x, p2y

    return inside

    
"""
Returns the distance between two Positions. 
The distance is calculated using a tangent plane approximation. This
method is accurate to within 0.1 km at 400 km and 0.3 km at 700 km.
"""
DEG2RAD = math.pi / 180.0
def distGlobal(latA, lonA, latB, lonB):

    lonA *= DEG2RAD
    latA *= DEG2RAD
    lonB *= DEG2RAD
    latB *= DEG2RAD

    # distance
    dist = \
            math.acos( \
            math.sin(latA) * math.sin(latB) \
            + math.cos(latA) * math.cos(latB) * math.cos((lonB - lonA)))

    dist /= DEG2RAD

    return dist


class EvaluateEvent(object):

    SLAB_DEPTH_ERROR = -999.9

    global __DEG2KM
    __DEG2KM = 10000.0 / 90.0

    global __UNDERWATER_DATA_PATH
    __UNDERWATER_DATA_PATH = os.path.join(os.path.dirname(__file__), "evaluate/data/underwater")  
    
    global __PLATE_BOUNDARY_DATA_PATH
    __PLATE_BOUNDARY_DATA_PATH = os.path.join(os.path.dirname(__file__), "evaluate/data/plate_boundaries")  
    global __PLATE_BOUNDARY_LIST
    __PLATE_BOUNDARY_LIST = [
        ("PB2002_steps.dat.txt.CTF.xy", "continental transform fault", "CTF"),
        ("PB2002_steps.dat.txt.OTF.xy", "oceanic transform fault", "OTF"),
        ("PB2002_steps.dat.txt.CCB.xy", "continental convergent boundary", "CCB"),
        ("PB2002_steps.dat.txt.OCB.xy", "oceanic convergent boundary", "OCB"),
        ("PB2002_steps.dat.txt.SUB.xy", "subduction zone", "SUB"),
        ("PB2002_steps.dat.txt.CRB.xy", "continental rift boundary", "CRB"),
        ("PB2002_steps.dat.txt.OSR.xy", "oceanic spreading ridge", "OSR")
        ]
    PLATE_IDENTIFIERS =  dict(list(zip(
                                  ("AF", "AM", "AN", "AP", "AR", "AS", "AT", "AU", "BH", "BR", "BS", "BU", "CA", "CL", "CO", "CR", "EA", "EU", "FT", 
                                   "GP", "IN", "JF", "JZ", "KE", "MA", "MN", "MO", "MS", "NA", "NB", "ND", "NH", "NI", "NZ", "OK", "ON", "PA", "PM", 
                                   "PS", "RI", "SA", "SB", "SC", "SL", "SO", "SS", "SU", "SW", "TI", "TO", "WL", "YA"), 
                                  ("Africa", "Amur", "Antarctica", "Altiplano", "Arabia", "Aegean Sea", "Anatolia", "Australia", "Birds Head", 
                                   "Balmoral Reef", "Banda Sea", "Burma", "Caribbean", "Caroline", "Cocos", "Conway Reef", "Easter", "Eurasia", 
                                   "Futuna", "Galapagos", "India", "Juan de Fuca", "Juan Fernandez", "Kermadec", "Mariana", "Manus", "Maoke", 
                                   "Molucca Sea", "North America", "North Bismarck", "North Andes", "New Hebrides", "Niuafo'ou", "Nazca", "Okhotsk", 
                                   "Okinawa", "Pacific", "Panama", "Philippine Sea", "Rivera", "South America", "South Bismarck", "Scotia", 
                                   "Shetland", "Somalia", "Solomon Sea", "Sunda", "Sandwich", "Timor", "Tonga", "Woodlark", "Yangtze")
                                  )))
    PLATE_IDENTIFIERS.setdefault("ERROR")
    
    global __SLAB_DATA_PATH
    __SLAB_DATA_PATH = os.path.join(os.path.dirname(__file__), "evaluate/data/slab/models")
    global __SLAB_LIST
    __SLAB_LIST = [
        ("Alaska-Aleutians", "alu_slab1.0.clip", "alu_slab1.0_clip.grd"),
        ("Central America", "mex_slab1.0.clip", "mex_slab1.0_clip.grd"),
        ("Izu-Bonin", "izu_slab1.0.clip", "izu_slab1.0_clip.grd"),
        ("Kermadec-Tonga", "ker_slab1.0.clip", "ker_slab1.0_clip.grd"),
        ("Kamchatka/Kurils/Japan", "kur_slab1.0.clip", "kur_slab1.0_clip.grd"),
        ("Philippines", "phi_slab1.0.clip", "phi_slab1.0_clip.grd"),
        ("Ryukyu", "ryu_slab1.0.clip", "ryu_slab1.0_clip.grd"),
        ("Santa Cruz Islands/Vanuatu/Loyalty Islands", "van_slab1.0.clip", "van_slab1.0_clip.grd"),
        ("Scotia", "sco_slab1.0.clip", "sco_slab1.0_clip.grd"),
        ("Solomon Islands", "sol_slab1.0.clip", "sol_slab1.0_clip.grd"),
        ("South America", "sam_slab1.0.clip", "sam_slab1.0_clip.grd"),
        ("Sumatra-Java", "sum_slab1.0.clip", "sum_slab1.0_clip.grd"),
        ]
        
    
    def __init__(self):  # @UnusedVariable
        """
        Initializes an EvaluateEvent.

        See :class:`obspy.core.trace.Trace` for all parameters.
        """
        # initialize polygon list
        self.perim_polygons = []

    def readSlabPerimeters(self):
    
        for slab in __SLAB_LIST:
            filename = os.path.join(__SLAB_DATA_PATH, slab[1])
            polygon = []
            #print "DEBUG: read:", filename
            f_perim = open(filename, "r")
            flines = f_perim.readlines()
            for line in flines:
                coords = line.split()
                lon = float(coords[0])
                lat = float(coords[1])
                polygon.append((lon, lat))
            self.perim_polygons.append(polygon)
    
    
    
    def isInsideSlabPerimeter(self, lat, lon):
    
        if len(self.perim_polygons) < len(__SLAB_LIST):
            self.readSlabPerimeters()
            
        if lon < 0.0:
            lon = lon + 360.0
    
        #n_polygon = 0   # DEBUG
        for polygon in self.perim_polygons:
            #print "DEBUG: n_polygon:", self.perim_polygons.index(polygon), ", point:", point
            if pointInsidePolygon(lon, lat, polygon):
                return True, self.perim_polygons.index(polygon)
            #n_polygon = n_polygon + 1   # DEBUG
    
        return False, -1
    
    
    def isInsideSlab(self, lat, lon):

        if lon < 0.0:
            lon = lon + 360.0
        #print ">>>>>> DEBUG: lat, lon", lat, lon
    
        is_inside_slab, n_slab = self.isInsideSlabPerimeter(lat, lon)
        if is_inside_slab:
            slab_depth = -9.9
            slab_name = __SLAB_LIST[n_slab][0]
            try: 
                fname_in_temp = "temp_grdtrack_in.txt"
                f_in = open(fname_in_temp, "w")
                f_in.write(str(lon) + " " + str(lat) + " \n")
                f_in.close()
                filename = os.path.join(__SLAB_DATA_PATH, __SLAB_LIST[n_slab][2])
                subprocess_cmd = "gmt grdtrack" + " " + fname_in_temp + " -G" + filename
                #print subprocess_cmd
                args = shlex.split(subprocess_cmd)
                fname_out_tmp = "temp_grdtrack_out.txt"
                f_out = open(fname_out_tmp, "w")
                retcode = subprocess.call(args, stdout = f_out)
                f_out.close()
                f_out = open(fname_out_tmp, "r")
                line = f_out.readline()
                f_out.close()
                values = line.split()
                slab_depth = -float(values[2])
            except Exception as e:
                slab_name += ": DEBUG: error: " + str(e)
                slab_depth = EvaluateEvent.SLAB_DEPTH_ERROR
            #print ">>>>>> DEBUG: is_inside_slab, n_slab, slab_name, slab_depth", is_inside_slab, n_slab, slab_name, slab_depth
            return is_inside_slab, n_slab, slab_name, slab_depth

        return is_inside_slab, None, None, None
    
    
    def percentProbIsUnderwater(self, lat, lon, minHorUnc, maxHorUnc, azMaxHorUnc):
    
        if lon < 0.0:
            lon = lon + 360.0

        filename = os.path.join(__UNDERWATER_DATA_PATH, "percent_prob_underwater.hdr")
        f_in = open(filename, "r")
        line = f_in.readline()
        f_in.close()
        tokens = line.split()
        degree_step = float(tokens[0])
        #print "DEBUG: degree_step:", degree_step
        # Gridline registration
        # In this registration, the nodes are centered on the grid line intersections 
        # and the data points represent the average value in a cell of dimensions (xinc*yinc) 
        # centered on the nodes (Figure B.1). In the case of grid line registration 
        # the number of nodes are related to region and grid spacing by
        # nx =(xmax-xmin)/xinc+1 ny =(ymax-ymin)/yinc+1
        n_lon = int(0.5 + 360.0 / degree_step) + 1
        n_lat = int(0.5 + 180.0 / degree_step) + 1
        ilat = int((lat - -90.0) / degree_step)
        ilon = int(lon / degree_step)
        #print "DEBUG: offset = ilat * n_lon + ilon:", offset, ilat, n_lon, ilon
        filename = os.path.join(__UNDERWATER_DATA_PATH, "percent_prob_underwater.table")
        f_in = open(filename, "rb")
        # get prob at specified lat/lon
        percent = self.readPercentProbIsUnderwater(f_in, ilat, ilon, n_lon)
        percent_sum = percent
        percent_sqr_sum = percent * percent
        nvalues = 1
        # get prob at cells within radius of specified lat/lon
        if maxHorUnc > 0.0:
            nshell = 1
            scale2km = degree_step * __DEG2KM
            dx_scale = math.cos(math.radians(lat))
            azimuth = math.radians(azMaxHorUnc)
            cosa = math.cos(azimuth)
            sina = math.sin(azimuth)
            found = True
            while found:
                found = False
                for n_ndx in range(ilat - nshell, ilat + nshell + 1):
                    n = n_ndx
                    if n < 0:
                        n = n + n_lat
                    elif n > n_lat - 1:
                        n = n - n_lat
                    if n == ilat - nshell or n == ilat + nshell:    # top or bottom row, evaluate all lon cells
                        lonrange = list(range(ilon - nshell, ilon + nshell + 1))
                    else:    # intermediate row, evaluate boundary lon cells
                        lonrange = [ilon - nshell, ilon + nshell]
                    for m_ndx in lonrange:
                        m = m_ndx
                        if m < 0:
                            m = m + n_lon
                        elif m > n_lon - 1:
                            m = m - n_lon
                        dx = scale2km * dx_scale * (m - ilon)
                        dy = scale2km * (n - ilat)
                        if nshell == 1:     # always evaluate first shell to avoid pixelization errors in table and to have enough samples for std err calculation
                            percent = self.readPercentProbIsUnderwater(f_in, n, m, n_lon)
                            percent_sum += percent
                            percent_sqr_sum += percent * percent
                            nvalues += 1
                        elif abs(dx) < maxHorUnc and abs(dy) < maxHorUnc:
                            # check if point is inside ellipse, using, e.g. http://www.maa.org/joma/volume8/kalman/general.html
                            minterm = (dx * cosa - dy * sina) / minHorUnc
                            maxterm = (dy * cosa + dx * sina) / maxHorUnc
                            ellipse_eq = minterm * minterm + maxterm * maxterm
                            #print "DEBUG: dx:", str(dx) + ", dy:", str(dy) + ", ellipse_eq:", str(ellipse_eq)
                            if ellipse_eq <= 1.0:   # check if point on or inside boundary of ellipse
                                found = True
                                percent = self.readPercentProbIsUnderwater(f_in, n, m, n_lon)
                                percent_sum += percent
                                percent_sqr_sum += percent * percent
                                nvalues += 1
                nshell += 1

        percent_mean = percent_sum / float(nvalues)
        percent_std_dev = 0.0
        vtemp = percent_sqr_sum / float(nvalues) - percent_mean * percent_mean
        if vtemp > sys.float_info.min:
            percent_std_dev = math.sqrt(percent_sqr_sum / float(nvalues) - percent_mean * percent_mean)
        f_in.close()
        return percent_mean, percent_std_dev


    def readPercentProbIsUnderwater(self, f_in, ilat, ilon, n_lon):
    
        offset = ilat * n_lon + ilon
        f_in.seek(offset)
        byte_read = f_in.read(1)
        values = unpack('b', byte_read)
        ipercent = values[0]
        #print "DEBUG: ilat:", str(ilat) + ", ilon:", str(ilon) + ", ipercent:", str(ipercent)
        
        return ipercent


    def findClosestPlateBoundary(self, lat, lon, exclude_boundary_codes):
    
        closest_boundary_class = "XXX"
        closest_boundary_class_id = "XXX"
        closest_boundary_code = "XXX"
        current_boundary_code = "XXX"
        distance_min = 999.9
        
        for plate_boundary in __PLATE_BOUNDARY_LIST:
            filename = os.path.join(__PLATE_BOUNDARY_DATA_PATH, plate_boundary[0])
            #print "DEBUG: read:", filename
            f_perim = open(filename, "r")
            flines = f_perim.readlines()
            for line in flines:
                tokens = line.split()
                if tokens[0] == ">":
                    current_boundary_code = tokens[1];
                    continue
                if current_boundary_code in exclude_boundary_codes:
                    continue
                lon_test = float(tokens[0])
                lat_test = float(tokens[1])
                distance = distGlobal(lat_test, lon_test, lat, lon)
                if distance < distance_min:
                    distance_min = distance
                    closest_boundary_code = current_boundary_code
                    closest_boundary_class = plate_boundary[1]
                    closest_boundary_class_id = plate_boundary[2]

        return distance_min * DEG2KM, closest_boundary_code, closest_boundary_class, closest_boundary_class_id
    
    
    def evaluatePlateBoundaryCode(self, boundary_code):
        
        split_chr = "/"
        plate_codes = boundary_code.split(split_chr)
        if len(plate_codes) < 2:
            split_chr = "\\"
            plate_codes = boundary_code.split(split_chr)
        if len(plate_codes) < 2:
            split_chr = "-"
            plate_codes = boundary_code.split(split_chr)
        if len(plate_codes) == 2:
            plate_boundary_identifiers = (EvaluateEvent.PLATE_IDENTIFIERS.get(plate_codes[0]), EvaluateEvent.PLATE_IDENTIFIERS.get(plate_codes[1]))
        else:
            plate_boundary_identifiers = ("error: evaluating plate codes: " + boundary_code, "")
            
        return plate_boundary_identifiers, split_chr
            
    
DEG2KM=10000.0/90.0
if __name__ == '__main__':

    import sys
    import os
    from struct import *

    lat = float(sys.argv[1])
    lon = float(sys.argv[2])
    minHorUnc = float(sys.argv[3])
    maxHorUnc = float(sys.argv[4])
    azMaxHorUnc = float(sys.argv[5])
    print("Epicenter: lat:", lat, " lon:", lon, " minHorUnc:", minHorUnc, " maxHorUnc:", maxHorUnc, " azMaxHorUnc:", azMaxHorUnc)   
        
    evaluateEvent = EvaluateEvent()
    
    # underwater
    if True:

        percent_mean, percent_std_dev = evaluateEvent.percentProbIsUnderwater(lat, lon, minHorUnc, maxHorUnc, azMaxHorUnc)
        print("Probability underwater:", int(0.5 + percent_mean), "+/-", int(0.5 + percent_std_dev), "%")

    # plate boundary
    if True:

        print("Closest plate boundaries:")
        exclude_boundary_codes = ""
        nboundary = 0
        while nboundary < 4:
            distance_min, boundary_code, closest_boundary_class, closest_boundary_class_id = evaluateEvent.findClosestPlateBoundary(lat, lon, exclude_boundary_codes)
            if distance_min > 1000.0:
                break
            plate_boundary_identifiers, split_chr = evaluateEvent.evaluatePlateBoundaryCode(boundary_code)
            print(boundary_code, "[", \
                plate_boundary_identifiers[0], split_chr, \
                plate_boundary_identifiers[1], \
                ",", closest_boundary_class, ", ", closest_boundary_class_id, "]: distance:", "< " + str(int(distance_min)), "km")
            exclude_boundary_codes += "$" + boundary_code
            nboundary += 1

    # isInsideSlab slab
    if True:

        is_inside_slab, n_slab, slab_name, slab_depth = evaluateEvent.isInsideSlab(lat, lon)
    
        if is_inside_slab:
            try:
                slab_depth_str = int(round(slab_depth))
            except:     # NaN, infinite, ...
                slab_depth_str = "unknown"
            print("In a slab:", slab_name + ": slab depth:", slab_depth_str, "km")
        else:
            print("Not in a slab.")







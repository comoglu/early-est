/*
 * This file is part of the Anthony Lomax Java Library.
 *
 * Copyright (C) 2021 Anthony Lomax <anthony@alomax.net www.alomax.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

/*
 * File:   json.h
 * Author: anthony
 *
 * Created on 23 November 2021, 10:10
 */

#ifndef JSON_H
#define JSON_H

#ifdef __cplusplus
extern "C" {
#endif


#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>

    // xml
    void xml2jsonc_convert_elements(xmlNode *anode, json_object *jobj);
    int writeJSONcopyOfxml(xmlDocPtr xml_doc, char *json_filename);

    // GeoJSON
#define GEOJSON_LAT "lat"
#define GEOJSON_LON "lon"
#define GEOJSON_ELEV "elev"
#define GEOJSON_DEPTH "depth"
    // geometry types: Point, LineString, Polygon, MultiPoint, MultiLineString, and MultiPolygon
#define GEOJSON_Point "Point"
#define GEOJSON_LineString "LineString"
#define GEOJSON_Polygon "Polygon"
#define GEOJSON_MultiPoint "MultiPoint"
#define GEOJSON_MultiLineString "MultiLineString"
#define GEOJSON_MultiPolygon "MultiPolygon"
    int geoJson_isMulti1level(char *geometry_type);
    int writeGeoJSONcopyOftable(char *table_filename, int ncolumns, char **columns, int nproperties, char **properties, char **values, char *separator, char *geometry_type, char *json_filename);
    int writeJSONcopyOftable(char *table_filename, int ncolumns, char **columns, int nproperties, char **properties, char **values,
            char *separator, char *json_filename);


#ifdef __cplusplus
}
#endif

#endif /* JSON_H */


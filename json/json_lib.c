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

// JSON functions
// 20211103 AJL - added

// to install see https://github.com/json-c/json-c

/*
 *
 * from https://github.com/katie-snow/xml2json-c/blob/master/src/libs/xml2json-c.c
 *
 * GNU General Public License v3.0
 * https://github.com/katie-snow/xml2json-c/blob/master/LICENSE
 *
 ============================================================================
 Name        : xml2json-c.c
 Author      : Katie Snow
 Version     :
 Copyright   : Copyright 2015 Katie Snow
 Description : XML 2 JSON-C main configuration library
 ============================================================================
 */


#include <stdio.h>
#include <string.h>
#include <time.h>

#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>
//#include <libxml/tree.h>

#include <json-c/json.h>
#include "../json/json_lib.h"


#define STANDARD_STRLEN 4096
static char tmp_str[STANDARD_STRLEN];


#define NAME_LIST_LEN 4096
#define NODE_NAME_MAX_LEN 128
#define NUM_REPEAT_ELEMENTS_MAX ((NAME_LIST_LEN-1)/NODE_NAME_MAX_LEN)

void xml2jsonc_convert_elements(xmlNode *anode, json_object *jobj) {

    xmlNode *cur_node = NULL;
    static char node_ns_name[NODE_NAME_MAX_LEN];

    // check for repeated elements, which must be put in JSON arrays
    char name_list[NAME_LIST_LEN] = "\0";
    char repeated_list[NUM_REPEAT_ELEMENTS_MAX][NODE_NAME_MAX_LEN];
    struct json_object * array_list[NUM_REPEAT_ELEMENTS_MAX];
    int num_repeat_elements = 0;
    for (cur_node = anode; cur_node; cur_node = cur_node->next) {
        if (cur_node->type == XML_ELEMENT_NODE) {

            // node name included namespace prefix
            if (cur_node->ns->prefix != NULL) {
                sprintf(node_ns_name, "%s:%s", cur_node->ns->prefix, cur_node->name);
            } else {
                sprintf(node_ns_name, "%s", cur_node->name);
            }

            if (xmlChildElementCount(cur_node) != 0) {
                // not a JSON string object
                if (strstr(name_list, node_ns_name) == NULL) {
                    // not in list, add
                    strncat(name_list, node_ns_name, sizeof (name_list) - strlen(name_list) - 1);
                    strncat(name_list, "$", sizeof (name_list) - strlen(name_list) - 1); // separator
                    ////printf("DEBUG: new name:%s nchild:%d\n", node_ns_name, (int) xmlChildElementCount(cur_node));
                } else {
                    if (num_repeat_elements < NUM_REPEAT_ELEMENTS_MAX) {
                        ////printf("DEBUG: found REPEAT name:%s nchild:%d\n", node_ns_name, (int) xmlChildElementCount(cur_node));
                        // check if already in repeated list
                        int n = 0;
                        for (; n < num_repeat_elements; n++) {
                            if (strcmp(node_ns_name, repeated_list[n]) == 0) {
                                break;
                            }
                        }
                        if (n == num_repeat_elements) {
                            // not present, add to lists of repeated node names
                            strncpy(repeated_list[num_repeat_elements], node_ns_name, sizeof (repeated_list) - 1);
                            array_list[num_repeat_elements] = json_object_new_array();
                            num_repeat_elements++;
                            //printf("DEBUG: new REPEAT name:%s nchild:%d\n", node_ns_name, (int) xmlChildElementCount(cur_node));
                        }
                    }
                }
            }
        }
    }

    json_object *cur_jobj = NULL;
    int cur_jobj_added = 0;
    json_object *cur_jstr = NULL;

    for (cur_node = anode; cur_node; cur_node = cur_node->next) {

        //printf("DEBUG: cur_node->type: %d name:%s\n", cur_node->type, node_ns_name);

        if (cur_node->type == XML_ELEMENT_NODE) {

            // node name included namespace prefix
            if (cur_node->ns->prefix != NULL) {
                sprintf(node_ns_name, "%s:%s", cur_node->ns->prefix, cur_node->name);
            } else {
                sprintf(node_ns_name, "%s", cur_node->name);
            }

            cur_jobj = json_object_new_object();

            // get attributes
            xmlAttr* attribute = cur_node->properties;
            while (attribute) {
                xmlChar* value = xmlNodeListGetString(cur_node->doc, attribute->children, 1);
                //do something with value
                //printf("DEBUG: name:%s attribute:%s|%s\n", node_ns_name, (char *) attribute->name, (char *) value);
                cur_jstr = json_object_new_string((char *) value);
                json_object_object_add(cur_jobj, (char *) attribute->name, cur_jstr);
                xmlFree(value);
                attribute = attribute->next;
            }

            if (xmlChildElementCount(cur_node) == 0) {
                // JSON string object
                cur_jstr = json_object_new_string((char *) xmlNodeGetContent(cur_node));
                json_object_object_add(jobj, node_ns_name, cur_jstr);

            } else {
                // check if in repeated list
                int n = 0;
                for (; n < num_repeat_elements; n++) {
                    if (strcmp(node_ns_name, repeated_list[n]) == 0) {
                        break;
                    }
                }
                if (n < num_repeat_elements) {
                    // JSON array element
                    // 20220223 AJL Bug Fix:  json_object_array_add(array_list[n], json_object_get(cur_jobj));
                    json_object_array_add(array_list[n], cur_jobj);
                    cur_jobj_added++;
                    ////printf("DEBUG: new array element: array:%s name:%s nchild:%d\n", repeated_list[n], node_ns_name, (int) xmlChildElementCount(cur_node));
                } else {
                    // JSON object
                    // 20220223 AJL Bug Fix:  json_object_object_add(jobj, node_ns_name, json_object_get(cur_jobj));
                    json_object_object_add(jobj, node_ns_name, cur_jobj);
                    cur_jobj_added++;
                }
            }

            // process children
            xml2jsonc_convert_elements(cur_node->children, cur_jobj);

        }
    }

    // add all arrays
    for (int n = 0; n < num_repeat_elements; n++) {
        json_object_object_add(jobj, repeated_list[n], array_list[n]);
    }

    // clean any orphan objects
    if (!cur_jobj_added) {
        json_object_put(cur_jobj);
    }

}

/**
 * writeJSONcopyOfxml:
 * @xml_doc: xmlDocPtr xml document to convert to JSON
 * @json_filename: string JSON output file root
 *
 * Converts @xml_doc into JSON and writes to @json_filename
 *
 * Returns -1 in case of error.
 */
int writeJSONcopyOfxml(xmlDocPtr xml_doc, char *json_filename) {

    //int itp = 0;
    //printf("DEBUG: TP %d\n", itp++);

    // create JSON object
    json_object* jobj = json_object_new_object();
    //printf("DEBUG: TP %d\n", itp++);
    if (jobj == NULL) {
        printf("writeJSONcopyOfxml: Error at json_object_new_object\n");
        return (-1);
    }

    // get the root element node
    xmlNodePtr root_element = xmlDocGetRootElement(xml_doc);
    //printf("DEBUG: TP %d\n", itp++);
    if (root_element == NULL) {
        printf("writeJSONcopyOfxml: Error at xmlDocGetRootElement\n");
        return (-1);
    }

    xml2jsonc_convert_elements(root_element, jobj);
    //printf("DEBUG: TP %d\n", itp++);

    // write JSON to file
    int rc = json_object_to_file_ext(json_filename, jobj, JSON_C_TO_STRING_PRETTY);
    //printf("DEBUG: TP %d json_filename: %s\n", itp++, json_filename);
    if (rc < 0) {
        printf("writeJSONcopyOfxml: Error at json_object_to_file\n");
        return (-1);
    }

    // Decrement the reference count of the JSON object and free if it reaches zero.
    json_object_put(jobj);
    //printf("DEBUG: TP %d\n", itp++);

    return (0);

}



// GeoJSON

int geoJson_isMulti1level(char *geometry_type) {

    if (strcmp(geometry_type, GEOJSON_LineString) == 0
            || strcmp(geometry_type, GEOJSON_MultiPoint) == 0) {
        return (1);
    }

    return (0);

}

json_object * getJsonObject_coerceToNumber(char *cvalue) {

    static char fmt_str[STANDARD_STRLEN];

    //printf("getJsonObject_coerceToNumber: cvalue %s\n", cvalue);
    json_object * jobj = NULL;

    double dvalue;
    if (sscanf(cvalue, "%lf", &dvalue) == 1) {
        //printf("   getJsonObject_coerceToNumber: dvalue %lf\n", dvalue);
        // is double
        //jobj = json_object_new_double(dvalue);
        // try to preserve original decimal precision
        snprintf(fmt_str, STANDARD_STRLEN - 1, "%%%lug", strlen(cvalue));
        //printf("   getJsonObject_coerceToNumber: cvalue |%s|  strlen(cvalue) %lu  fmt_str %s\n", cvalue, strlen(cvalue), fmt_str);
        jobj = json_object_new_double(dvalue);
        json_object_set_serializer(jobj, json_object_double_to_json_string, fmt_str, NULL);
    } else {
        // int
        int64_t i64value;
        if (sscanf(cvalue, "%ld", (long *) &i64value) == 1) {
            //printf("   getJsonObject_coerceToNumber: i64value %lld\n", i64value);
            // is integer
            jobj = json_object_new_int64(i64value);
        } else {
            //printf("   getJsonObject_coerceToNumber: cvalue %s\n", cvalue);
            // keep as string
            jobj = json_object_new_string(cvalue);
        }
    }

    return (jobj);

}

/**
 * writeGeoJSONcopyOftable:
 * @table_filename: string table input file to convert to JSON
 * @json_filename: string JSON output file root
 *
 * Converts @table_filename into JSON and writes to @json_filename
 *
 * Returns -1 in case of error.
 */
int writeGeoJSONcopyOftable(char *table_filename, int ncolumns, char **columns, int nproperties, char **properties, char **values,
        char *delimiters, char *geometry_type, char *json_filename) {

    // set properties object
    json_object *jobj_properties = NULL;
    int jobj_properties_added = 0;
    if (nproperties > 0) {
        jobj_properties = json_object_new_object();
        for (int nprop = 0; nprop < nproperties; nprop++) {
            json_object_object_add(jobj_properties, properties[nprop], json_object_new_string(values[nprop]));
        }
    }

    // open table file
    FILE *fp_tableStream = fopen(table_filename, "r");
    if (fp_tableStream == NULL) {
        sprintf(tmp_str, "ERROR: writeGeoJSONcopyOftable(): opening table input file: %s", table_filename);
        perror(tmp_str);
        return (-1);
    }

    // create JSON object
    json_object* jobj = json_object_new_object();
    //printf("DEBUG: TP %d\n", itp++);
    if (jobj == NULL) {
        printf("writeGeoJSONcopyOftable: Error at json_object_new_object: %s\n", table_filename);
        return (-1);
    }

    // create JSON array
    json_object *jarray_feature_collection = json_object_new_array();
    //printf("DEBUG: TP %d\n", itp++);
    if (jarray_feature_collection == NULL) {
        printf("writeGeoJSONcopyOftable: Error at json_object_new_array: %s\n", table_filename);
        return (-1);
    }

    json_object_object_add(jobj, "type", json_object_new_string("FeatureCollection"));
    // add features (which will be filled later)
    json_object_object_add(jobj, "features", jarray_feature_collection);

    json_object * jobj_feature = NULL;
    int jobj_feature_added = 0;
    json_object * jobj_geometry = NULL;
    int jobj_geometry_added = 0;
    json_object * jarray_multi_coordinates = NULL;
    int jarray_multi_coordinates_added = 0;

    // MultiPoint
    if (geoJson_isMulti1level(geometry_type)) {
        jobj_feature = json_object_new_object();
        if (jobj_feature == NULL) {
            printf("writeGeoJSONcopyOftable: Error at json_object_new_object: jobj_feature: %s\n", table_filename);
            return (-1);
        }
        json_object_object_add(jobj_feature, "type", json_object_new_string("Feature"));
        // add any properties
        if (nproperties > 0) {
            json_object_object_add(jobj_feature, "properties", jobj_properties);
            jobj_properties_added++;
        }

        // Geometry object for MultiPoint
        jobj_geometry = json_object_new_object();
        if (jobj_geometry == NULL) {
            printf("writeGeoJSONcopyOftable: Error at json_object_new_object: jobj_geometry: %s\n", table_filename);
            return (-1);
        }
        json_object_object_add(jobj_geometry, "type", json_object_new_string(geometry_type));

        // coordinates array
        jarray_multi_coordinates = json_object_new_array();
        if (jarray_multi_coordinates == NULL) {
            printf("writeGeoJSONcopyOftable: Error at json_object_new_array: %s\n", table_filename);
            return (-1);
        }
    }

    // loop over rows and create JSON
    static char line[STANDARD_STRLEN];
    char data[ncolumns][STANDARD_STRLEN];
    static char stemp[STANDARD_STRLEN];
    char *pline, *token;
    while (fgets(line, STANDARD_STRLEN, fp_tableStream) != NULL) {

        // if line starts with >, assume GMT comment and output as properties
        if (strlen(line) > 0) {
            int ifound = 0;
            char *c = line;
            while (*c != '\0') {
                if (*c == '>') {
                    ifound = 1;
                    break;
                } else if (*c != ' ') {
                    break;
                }
                c++;
            }
            // add any properties
            if (ifound) {
                json_object *jobj_properties_line = json_object_new_object();
                json_object_object_add(jobj_properties_line, "comment", json_object_new_string(line));
                if (geoJson_isMulti1level(geometry_type)) {
                    json_object_object_add(jobj_feature, "properties", jobj_properties_line);
                } else {
                    json_object_object_add(jarray_feature_collection, "properties", jobj_properties_line);
                }
                continue;
            }
        }

        // non-MultiPoint
        if (!geoJson_isMulti1level(geometry_type)) {

            // Feature object for non-MultiPoint
            jobj_feature = json_object_new_object();
            if (jobj_feature == NULL) {
                printf("writeGeoJSONcopyOftable: Error at json_object_new_object: jobj_feature: %s\n", table_filename);
                return (-1);
            }
            json_object_object_add(jobj_feature, "type", json_object_new_string("Feature"));
            // add any properties
            if (nproperties > 0) {
                json_object_object_add(jobj_feature, "properties", jobj_properties);
                jobj_properties_added++;
            }

            // Geometry object for non-MultiPoint
            jobj_geometry = json_object_new_object();
            if (jobj_geometry == NULL) {
                printf("writeGeoJSONcopyOftable: Error at json_object_new_object: jobj_geometry: %s\n", table_filename);
                return (-1);
            }
            json_object_object_add(jobj_geometry, "type", json_object_new_string(geometry_type));
        }

        // coordinates array
        json_object *jarray_coordinates = json_object_new_array();
        if (jarray_coordinates == NULL) {
            printf("writeGeoJSONcopyOftable: Error at json_object_new_array: %s\n", table_filename);
            return (-1);
        }

        pline = line;
        int ncol = 0;
        while (ncol < ncolumns && (token = strsep(&pline, delimiters)) != NULL) {
            // remove any trailing newline
            token[strcspn(token, "\n")] = 0;
            strncpy(data[ncol++], token, STANDARD_STRLEN - 1);
        }
        if (ncol != ncolumns) {
            printf("writeGeoJSONcopyOftable: Error: ncolumn read != ncolumns specified: %s: %s\n", table_filename, line);
            return (-1);
        }

        // reverse lat, lon if necessary
        if (ncolumns > 1 && strcmp(columns[0], "lat") == 0 && strcmp(columns[1], "lon") == 0) {
            strncpy(stemp, data[0], STANDARD_STRLEN);
            strncpy(data[0], data[1], STANDARD_STRLEN);
            strncpy(data[1], stemp, STANDARD_STRLEN);
        }
        // load coordinates array
        for (int ncol = 0; ncol < ncolumns; ncol++) {
            json_object_array_add(jarray_coordinates, getJsonObject_coerceToNumber(data[ncol]));
        }

        if (geoJson_isMulti1level(geometry_type)) {
            // MultiPoint
            // load multi-coordinates array
            json_object_array_add(jarray_multi_coordinates, jarray_coordinates);
        } else {
            // non-MultiPoint
            json_object_object_add(jobj_geometry, "coordinates", jarray_coordinates);
            json_object_object_add(jobj_feature, "geometry", jobj_geometry);
            jobj_geometry_added++;
            json_object_array_add(jarray_feature_collection, jobj_feature);
            jobj_feature_added++;
        }

    }
    if (geoJson_isMulti1level(geometry_type)) {
        json_object_object_add(jobj_geometry, "coordinates", jarray_multi_coordinates);
        jarray_multi_coordinates_added++;
        json_object_object_add(jobj_feature, "geometry", jobj_geometry);
        jobj_geometry_added++;
        json_object_array_add(jarray_feature_collection, jobj_feature);
        jobj_feature_added++;
    }


    // close table file
    int retval = fclose(fp_tableStream);
    if (retval != 0) {
        sprintf(tmp_str, "ERROR: writeGeoJSONcopyOftable(): closing table input file: %s", table_filename);
        perror(tmp_str);
    }



    // write JSON to file
    int rc = json_object_to_file_ext(json_filename, jobj, JSON_C_TO_STRING_PRETTY);
    //printf("DEBUG: writeGeoJSONcopyOftable:  json_filename: %s: %s\n", table_filename, json_filename);
    if (rc < 0) {
        printf("writeGeoJSONcopyOftable: Error at json_object_to_file: %s\n", table_filename);
        return (-1);
    }

    // Decrement the reference count of the JSON object and free if it reaches zero.
    json_object_put(jobj);
    //printf("DEBUG: TP %d\n", itp++);

    // clean any orphan objects
    if (!jarray_multi_coordinates_added) {
        json_object_put(jarray_multi_coordinates);
    }
    if (!jobj_properties_added) {
        json_object_put(jobj_properties);
    }
    if (!jobj_feature_added) {
        json_object_put(jobj_feature);
    }

    return (0);

}

/**
 * writeJSONcopyOftable:
 * @table_filename: string table input file to convert to JSON
 * @json_filename: string JSON output file root
 *
 * Converts @table_filename into JSON and writes to @json_filename
 *
 * Returns -1 in case of error.
 */
int writeJSONcopyOftable(char *table_filename, int ncolumns, char **columns, int nproperties, char **properties, char **values,
        char *deimiters, char *json_filename) {

    // open table file
    FILE *fp_tableStream = fopen(table_filename, "r");
    if (fp_tableStream == NULL) {
        sprintf(tmp_str, "ERROR: writeJSONcopyOfxml(): opening table input file: %s", table_filename);
        perror(tmp_str);
        return (-1);
    }

    // create JSON object
    json_object* jobj = json_object_new_object();
    //printf("DEBUG: TP %d\n", itp++);
    if (jobj == NULL) {
        printf("writeGeoJSONcopyOftable: Error at json_object_new_object: %s\n", table_filename);
        return (-1);
    }

    // create JSON array
    json_object *jarray_rows = json_object_new_array();
    //printf("DEBUG: TP %d: %s\n", table_filename, itp++);
    if (jarray_rows == NULL) {
        printf("writeJSONcopyOftable: Error at json_object_new_array: %s\n", table_filename);
        return (-1);
    }

    json_object_object_add(jobj, "type", json_object_new_string("DataTable"));
    // add any properties
    if (nproperties > 0) {
        json_object *jobj_properties = json_object_new_object();
        for (int nprop = 0; nprop < nproperties; nprop++) {
            json_object_object_add(jobj_properties, properties[nprop], json_object_new_string(values[nprop]));
        }
        json_object_object_add(jobj, "properties", jobj_properties);
    }
    // add rows (which will be filled later)
    json_object_object_add(jobj, "rows", jarray_rows);

    // loop over rows and create JSON
    static char line[STANDARD_STRLEN];
    char data[ncolumns][STANDARD_STRLEN];
    char *pline, *token;
    while (fgets(line, STANDARD_STRLEN, fp_tableStream) != NULL) {

        // row data array
        json_object *jarray_row = json_object_new_array();
        if (jarray_row == NULL) {
            printf("writeJSONcopyOftable: Error at json_object_new_array: %s\n", table_filename);
            return (-1);
        }

        pline = line;
        //printf("DEBUG: pline: |%s|\n", pline);
        while (pline[0] && strncmp(pline, deimiters, strlen(deimiters)) == 0) {
            pline += strlen(deimiters);
        }
        int ncol = 0;
        //printf("DEBUG: pline: |%s|\n", pline);
        while (ncol < ncolumns && (token = strsep(&pline, deimiters)) != NULL) {
            //printf("DEBUG: token: |%s| pline: |%s|\n", token, pline);

            // remove any trailing newline
            token[strcspn(token, "\n")] = 0;
            strncpy(data[ncol++], token, STANDARD_STRLEN - 1);
        }
        if (ncol != ncolumns) {
            printf("writeJSONcopyOftable: Error: ncolumn read != ncolumns specified: %s\n", table_filename);
            return (-1);
        }

        // load coordinates array
        for (int ncol = 0; ncol < ncolumns; ncol++) {
            json_object_array_add(jarray_row, getJsonObject_coerceToNumber(data[ncol]));
        }

        json_object_array_add(jarray_rows, jarray_row);

    }


    // close table file
    int retval = fclose(fp_tableStream);
    if (retval != 0) {
        sprintf(tmp_str, "ERROR: writeJSONcopyOftable(): closing table input file: %s", table_filename);
        perror(tmp_str);
    }



    // write JSON to file
    int rc = json_object_to_file_ext(json_filename, jobj, JSON_C_TO_STRING_PRETTY);
    printf("DEBUG: writeJSONcopyOftable:  json_filename: %s\n", json_filename);
    if (rc < 0) {
        printf("writeJSONcopyOftable: Error at json_object_to_file: %s\n", table_filename);
        return (-1);
    }

    // Decrement the reference count of the JSON object and free if it reaches zero.
    json_object_put(jobj);
    //printf("DEBUG: TP %d\n", itp++);

    return (0);

}


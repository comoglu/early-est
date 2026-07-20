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

__author__ = "anthony"
__date__ = "$Jun 16, 2011 11:58:05 AM$"


from xml.dom.minidom import *

class ParseQML(object):

    def getAtribute(self, root_node, attrName):
        attr_node = root_node.getAttributeNode(attrName)
        if attr_node is None:
            print("Warning: ParseQML: for element:", root_node, ": no attribute found with name:", attrName)
            return None
        return str(attr_node.nodeValue)

    def getFirstChildWithName(self, root_node, childName):
        child_nodes = root_node.getElementsByTagName(childName)
        if len(child_nodes) < 1:
            print("Warning: ParseQML: no child element found with tag:", childName)
            return None
        return child_nodes[0]

    def getValueOfFirstChildWithName(self, root_node, childName):
        child_node = self.getFirstChildWithName(root_node, childName)
        if child_node is None:
            print("Warning: ParseQML: no child element found with tag:", childName)
            return None
        return str(child_node.childNodes[0].nodeValue)

    def getValueInFirstChildWithName(self, root_node, childName, valueName):
        child_node = self.getFirstChildWithName(root_node, childName)
        if child_node is None:
            print("Warning: ParseQML: no child element found with tag:", childName)
            return None
        value_node = self.getFirstChildWithName(child_node, valueName)
        if value_node is None:
            print("Warning: ParseQML: no value found with name :", valueName, " in child element with tag:", childName)
            return None
        return str(value_node.childNodes[0].nodeValue)

    def getMagnitudeChildValue(self, root_node, typeName, childName):
        mag_nodes = root_node.getElementsByTagName("magnitude")
        for mag_node in mag_nodes:
            type_nodes = mag_node.getElementsByTagName("type")
            if len(type_nodes) < 1:
                print("Warning: ParseQML: no type element found in magnitude node")
                continue
            if str(type_nodes[0].childNodes[0].nodeValue) == typeName:
                value_nodes = mag_node.getElementsByTagName(childName)
                if len(value_nodes) < 1:
                    continue
                return str(value_nodes[0].childNodes[0].nodeValue)
        print("Warning: ParseQML: no magnitude element found with type:", typeName)
        return None

    def getEarlyEstDiscriminantChildValue(self, root_node, typeName, childName):
        target_nodes = root_node.getElementsByTagNameNS("http://net.alomax/earlyest/xmlns/ee", "discriminant")
        for target_node in target_nodes:
            type_nodes = target_node.getElementsByTagName("type")
            if len(type_nodes) < 1:
                print("Warning: ParseQML: no type element found in ee:discriminant node")
                continue
            if str(type_nodes[0].childNodes[0].nodeValue) == typeName:
                value_nodes = target_node.getElementsByTagName(childName)
                if len(value_nodes) < 1:
                    continue
                return str(value_nodes[0].childNodes[0].nodeValue)
        print("Warning: ParseQML: no ee:discriminant element found with type:", typeName)
        return None

    def formatTime(self, timeStr, nDecSec):
        timeStr = timeStr.replace("-", ".").replace("T", "-")
        ndx = timeStr.rfind(":") + 1
        width = 2
        if nDecSec > 0:
            width = nDecSec + 3
        secStr = ('{0:0' + str(width) + '.' + str(nDecSec) + 'f}').format(float(timeStr[ndx:]))
        timeStr = timeStr[:ndx] + secStr
        return timeStr




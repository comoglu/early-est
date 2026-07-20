import sys
#! /usr/bin/python

# This file is part of the Anthony Lomax Java Library.
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
__date__ = "$Dec 8, 2011 9:04:49 AM$"


import io
import smtplib
import email.message
import email.generator


if __name__ == "__main__":

    if len(sys.argv) != 7:
        print "Usage:", sys.argv[0], \
            "fromaddr toaddrs server_host server_port user password"
    else:
        print "DEBUG: sys.argv:",sys.argv[0], sys.argv[1], sys.argv[2], \
            sys.argv[3], sys.argv[4], sys.argv[5], sys.argv[6]
        fromaddr = sys.argv[1]
        toaddrs = sys.argv[2]
        server_host = sys.argv[3]
        server_port = sys.argv[4]
        user = sys.argv[5]
        password = sys.argv[6]

        print "Enter message, end with ^D (Unix) or ^Z (Windows):"

        # Add the From: and To: headers at the start!
        msg = ("From: %s\r\nTo: %s\r\n" % (fromaddr, toaddrs))
        while 1:
            try:
                line = raw_input()
            except EOFError:
                break
            if line.strip() == ".":
                break
            msg = msg + line + "\r\n"

        if False:
            print "Message length is " + repr(len(msg))
            print '/' * 60
            print msg
            print '/' * 60

        if False:
            message = email.message.Message()
            message.set_payload(msg)
            print '-' * 60
            print message
            print '-' * 60
            f_gen_out = io.StringIO()
            generator = email.generator.Generator(f_gen_out)
            generator.flatten(message)
            print '=' * 60
            print f_gen_out.getvalue()
            print '=' * 60

        # Send message
        if True:
            server = smtplib.SMTP(server_host, server_port)
            #server.login(user, password)
            server.set_debuglevel(1)
            server.sendmail(fromaddr, toaddrs, msg)
            server.quit()


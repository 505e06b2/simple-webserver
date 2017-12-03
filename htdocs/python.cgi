#!/bin/python2

import time

print "Content-Type: text/html\r\n\r\nServer's localtime: %s<br>This is written in Python2" % time.strftime("%T")
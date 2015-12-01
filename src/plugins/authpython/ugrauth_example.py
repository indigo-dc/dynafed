#!/usr/bin/python
# -*- coding: utf-8 -*-

# Simple script that prints its arguments and then decides if the user has
# to be authorized
# usage:
# ugrauth_example.py clientname remoteaddr <fqan1> .. <fqanN>

import sys
def isallowed(clientname="unknown", remoteaddr="nowhere", resource="none", mode="0", fqans=None, keys=None):
    print "clientname", clientname
    print "remote address", remoteaddr
    print "fqans", fqans
    print "keys", keys
    print "resource", resource
    
    return 1234


#------------------------------
if __name__ == "__main__":
    isallowed(sys.argv[1], sys.argv[2:])

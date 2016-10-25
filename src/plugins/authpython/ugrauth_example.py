#!/usr/bin/python
# -*- coding: utf-8 -*-

# Simple script that prints its arguments and then decides if the user has
# to be authorized
# usage:
# ugrauth_example.py <clientname> <remoteaddr> <fqan1> .. <fqanN>
#
# Return value means:
# 0 --> access is GRANTED
# nonzero --> access is DENIED
#

import sys

# A class that one day may implement an authorization list loaded
# from a file during the initialization of the module.
# If this list is written only during initialization, and used as a read-only thing
# no synchronization primitives (e.g. semaphores) are needed, and the performance will be maximized
class _Authlist(object):
    def __init__(self):
        print "I claim I am loading an authorization list from a file, maybe one day I will :-)"

# Initialize a global instance of the authlist class, to be used inside the isallowed() function
myauthlist = _Authlist()



# The main function that has to be invoked from ugr to determine if a request
# has to be performed or not
def isallowed(clientname="unknown", remoteaddr="nowhere", resource="none", mode="0", fqans=None, keys=None):
    print "clientname", clientname
    print "remote address", remoteaddr
    print "fqans", fqans
    print "keys", keys
    print "mode", mode
    print "resource", resource

    # Read/list modes are always open
    if (mode == 'r') or (mode == 'l'):
      return 0

    # deny to anonymous user for any write mode
    if (clientname == 'nobody'):
      return 1

    # allow writing to anyone else who is not nobody
    return 0


#------------------------------
if __name__ == "__main__":
    r = isallowed(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4], sys.argv[5:])
    sys.exit(r)

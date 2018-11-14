#!/usr/bin/python
# -*- coding: utf-8 -*-

import sys
import shlex

class _Authlist(object):
    gridmap = {}
    access = []

    def __init__(self):
        print "Loading mapfile..."
        nmaps = 0
        # note that close() is not needed
	with open('/etc/grid-security/grid-mapfile') as search: 
          for line in search:
            items = shlex.split(line)
            print "0:", items[0], "1:", items[1]

            if (items[0] != "") and (items[1] != ""):
              gmentry = self.gridmap.get( items[0].strip('\"') , [] )
              gmentry.append(items[1])
              self.gridmap[  items[0].strip('\"') ] = gmentry
              nmaps = nmaps+1
            else:
              print "Skipped mapfile line", items

        print "Loading perms..."
        naccs = 0;
        # note that close() is not needed
        with open('/etc/grid-security/accessfile') as access:
          for line in access:
            items = line.split()
            print "0:", items[0], "1:", items[1], "2:", items[2]

            if (items[0] != "") and (items[1] != "") and (items[2] != ""):
              self.access.append( items )
              naccs = naccs+1
            else:
              print "Skipped accessfile line", items

        # Longer paths must appear first, for the search to work correctly
        self.access.sort(reverse=True)

        print 'Init done. mapfile: %d access: %d' % (nmaps, naccs)


myauthlist = _Authlist()
verbose = False

def isallowed(clientname="unknown", remoteaddr="nowhere", resource="none", mode="0", fqans=None, keys=None):


    path = resource.split('/')
    if(path[-1] == ''):
      del path[-1]
    if(path[0] == ''):
      del path[0]

    # Allow to read or list the top level directory
    if '/belle' in resource:
       if (mode=='r' or mode == 'l'):
           return 0
    if '/static' in resource:
       if (mode=='r' or mode == 'l'):
           return 0

    # deny to anonymous user for any mode
    if (clientname == 'nobody'):
      return 1

    # Get the groups of this user, from the mapfile
    clientgrps =  myauthlist.gridmap.get( clientname, "" )
    if verbose:
      print "User %s found. groups: %s" % (clientname, clientgrps)
    if clientgrps == "":
      print "unknown user '%s'. Denied." % clientname
      return 1
      
    # Find an entry in the access list that matches the incoming path. Here a loop seems acceptable
    for accessitems in myauthlist.access:
      # Does it match the incoming path?
      if resource.startswith( accessitems[0] ):
        if verbose:
          print "Found rule for path '%s' " % accessitems
        accessFQAN = accessitems[1]
        accesskeys = accessitems[2]
        if (accessFQAN in clientgrps):
          if (mode in accesskeys):
            if verbose:
              print "User '%s' Allowed." % clientname
            return 0
          else:
            print "User '%s' Denied by rule %s." % (clientname, accessitems)
            return 1

    print "User '%s' Denied." % clientname
    return 1










# main proggy, for testing. This sets the verbose mode which normally is off
if __name__ == "__main__":
    verbose = True;
    r = isallowed(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4], sys.argv[5:])
    sys.exit(r) 


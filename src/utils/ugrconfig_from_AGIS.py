#!/usr/bin/env python

# ugrconfig_from_AGIS extract info that is used to configure an HTTP federation
# that uses the Dynamic Federations
#
# Credit to Ilija Vukotic for the original version, used for FAX and XROOTD

import os, sys
import urllib2,simplejson

out=[]

try:
    req = urllib2.Request("http://atlas-agis-api.cern.ch/request/service/query/get_se_services/?json&flavour=HTTP", None)
    opener = urllib2.build_opener()
    f = opener.open(req)
    res=simplejson.load(f)
    for s in res:
        si={}
        si['rc_site']=s["rc_site"]
        si['aprotocols']=[]
        si['endpoint']=s["endpoint"]
        #print  s["rc_site"], s["flavour"], s["endpoint"]
        #print 'aprotocols:'
        pro=s["aprotocols"]
        if 'r' in pro:
            pr=pro['r']
            for p in pr:
                si['aprotocols'].append(p[2])
        #        print '\t', p
        #print '-------------------------------'
        if len(si['aprotocols'])>0:
            out.append(si)
    print (simplejson.dumps(out, sort_keys=True, indent=4))
except:
    print "Unexpected error:", sys.exc_info()[0]    

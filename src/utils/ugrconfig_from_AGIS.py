#!/usr/bin/env python

# ugrconfig_from_AGIS extract info that is used to configure an HTTP federation
# that uses the Dynamic Federations
#
# Credit to Ilija Vukotic for the original version, used for FAX and XROOTD

import os, sys
import urllib2,simplejson

try:
    req = urllib2.Request("http://atlas-agis-api.cern.ch/request/service/query/get_se_services/?json&flavour=HTTP", None)
    opener = urllib2.build_opener()
    f = opener.open(req)
    res=simplejson.load(f)
    for s in res:
        protocols = []
        if 'r' in s["aprotocols"]:
            for p in s["aprotocols"]['r']:
                protocols.append(p[2])
        prefix = os.path.commonprefix(protocols)
        prefix = prefix[:prefix.rfind("/")]
        pro = ""
        for p in protocols:
            pro += " " + p[len(prefix):]
        if len(protocols) > 0:
            impl = s["impl"]
            if not s["impl"]:
                impl = "???"
            url = s["endpoint"] + prefix 
            print """
###########
## Talk to a %s instance in %s
##
glb.locplugin[]: /usr/local/lib64/ugr/libugrlocplugin_davrucio.so %s 1 %s
locplugin.%s.pfxmultiply: %s""" % (impl, s["rc_site"], s["rc_site"], url, s["rc_site"], pro)
except:
    print "Unexpected error:", sys.exc_info()[0]    

#!/usr/bin/env python

# ugrconfig_from_AGIS extract info that is used to configure an HTTP federation
# that uses the Dynamic Federations
#
# Credit to Ilija Vukotic for the original version, used for FAX and XROOTD

import os, sys
import urllib2,simplejson
import getopt
import pycurl

TIMEOUT = 5

def main(argv):
    flags, args = getopt.getopt(argv, "e:p:t:", ["exclude=","proxy=","threads="])
    option = {}
    for flag, arg in flags:
        flag = flag[flag.rfind('-')+1:]
        if flag == "e":
            flag = "exclude"
        elif flag == "p":
            flag = "proxy"
        elif flag == "t":
            flag = "threads"
        option[flag] = arg
    exclude = None
    if "exclude" in option:
        exclude = option["exclude"].split(',')
    if "threads" in option:
        threads = str(option["threads"])
    else:
        threads = "5"

    req = urllib2.Request("http://atlas-agis-api.cern.ch/request/service/query/get_se_services/?json&flavour=HTTP", None)
    opener = urllib2.build_opener()
    f = opener.open(req)
    res=simplejson.load(f)
    output = []
    for s in res:
        protocols = []
        if 'r' in s["aprotocols"]:
            for p in s["aprotocols"]['r']:
                if exclude:
                    skip = False
                    for e in exclude:
                        if e in p[2]:
                            skip = True
                    if skip:
                        continue
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
            if "proxy" in option:
                request_url = url + protocols[0][len(prefix):]
                test = request(request_url,option["proxy"])
                print "Test for %s: %s" % (url + protocols[0][len(prefix):], test)
                if not test.startswith("2"):
                    continue
            out = """###########
## Talk to a %s instance in %s
##
glb.locplugin[]: /usr/local/lib64/ugr/libugrlocplugin_davrucio.so %s %s %s""" % (impl, s["rc_site"], s["rc_site"], threads, url)
            if len(protocols) > 1:
                out += "\nlocplugin.%s.pfxmultiply:%s" % (s["rc_site"], pro)
            output.append(out)
    file = open('ugr.conf', 'w')
    file.write("\n\n".join(output))
    file.close()
    sys.exit(0)

def request(url, proxy):
    """ Execute a request defined by the action to test an url"""
    curl = pycurl.Curl()
    curl.setopt(pycurl.URL, url)
    curl.setopt(pycurl.CONNECTTIMEOUT, TIMEOUT)
    curl.setopt(pycurl.TIMEOUT, TIMEOUT)
    #curl.setopt(pycurl.SSL_VERIFYPEER, 1)
    #curl.setopt(pycurl.SSL_VERIFYHOST, 2)
    curl.setopt(pycurl.FOLLOWLOCATION, 1)
    curl.setopt(pycurl.WRITEFUNCTION, lambda x: None)
    if proxy:
      curl.setopt(pycurl.CAINFO, proxy)
      curl.setopt(pycurl.SSLCERT, proxy)
      curl.setopt(pycurl.CAPATH, "/etc/grid-security/certificates")

    exception = ""
    try:
        curl.perform()
        code = curl.getinfo(pycurl.HTTP_CODE)
        curl.close()
    except Exception, e:
        e = str(e)
        exception = "Exception " + e[e.find('(')+1:e.rfind(')')]
        code = 0
    return str(code)


if __name__ == "__main__":
    main(sys.argv[1:])

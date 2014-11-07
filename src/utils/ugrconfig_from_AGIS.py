#!/usr/bin/env python
##############################################################################
# Copyright (c) CERN, 2014.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at #
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS
# OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
##############################################################################
#
# NAME :        ugrconfig_from_AGIS.py
#
# DESCRIPTION : Extract info that is used to configure an HTTP federation that uses the Dynamic Federations.
#
# AUTHORS :     Ivan.Calvet@cern.ch
#
##############################################################################

import os
import sys
import argparse
import urllib2, simplejson
from   subprocess import Popen, PIPE

def main():
    parser = argparse.ArgumentParser(description='Extract info that is used to configure an HTTP federation that uses the Dynamic Federations.')
    parser.add_argument('-p', '--proxy',  required=True, help='specify your proxy path. If your proxy is registered in the X509_USER_PROXY environment variable, just use "-p grid" option. If you don\'t want to check every URL, please use "-p NO_CHECK" option.')
    parser.add_argument('-e', '--exclude', metavar='KEYWORD', nargs='*', help='exclude the paths containing these keywords (spaces separated).')
    parser.add_argument('-t', '--threads', metavar='NUMBER', type=int, default="5", help='specify the number of threads at each entry (5 by default).')
    args = parser.parse_args()

    req = urllib2.Request("http://atlas-agis-api.cern.ch/request/service/query/get_se_services/?json&flavour=HTTP", None)
    opener = urllib2.build_opener()
    f = opener.open(req)
    res = simplejson.load(f)
    output = []
    nb = 0
    for s in res:
        nb += 1
        print ">>> Entry %s / %s:" % (nb, len(res))
        protocols = []
        if 'r' in s["aprotocols"]:
            for p in s["aprotocols"]['r']:
                if args.exclude and [val for val in args.exclude if val in p[2]]:
                    continue
                if p[2].endswith("/rucio/"):
                    protocols.append(p[2][:-6])
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
            if args.proxy != "NO_CHECK":
                request_url = url + protocols[0][len(prefix):]
                test = request(request_url,args.proxy)
                print "Test of %s" % (url + protocols[0][len(prefix):])
                if test:
                    print "-> FAILED"
                    continue
            print "-> OK"
            out = """###########
## Talk to a %s instance in %s
##
glb.locplugin[]: /usr/local/lib64/ugr/libugrlocplugin_davrucio.so %s %s %s""" % (impl, s["rc_site"], s["rc_site"], args.threads, url)
            if len(protocols) > 1:
                out += "\nlocplugin.%s.pfxmultiply:%s" % (s["rc_site"], pro)
            output.append(out)
        else:
            print "-> No path to test."
    file = open('ugr.conf', 'w')
    file.write("\n\n".join(output))
    file.close()
    sys.exit(0)

def request(url, proxy):
    """ Execute a request defined by the action to test an url"""
    if proxy == "grid":
        proxy = os.environ["X509_USER_PROXY"]
        process = Popen(["davix-ls", "-P", "grid", "-k", url], stdout=PIPE, stderr=PIPE)
    else:
        process = Popen(["davix-ls", "-E", proxy, "-k", url], stdout=PIPE, stderr=PIPE)
    code = process.wait()
    return code


if __name__ == "__main__":
    main()


/* teststat
 * Initialize the connector with the given cfgfile
 * and do a stat of the given file
 *
 * by Fabrizio Furano, CERN, Nov 2011
 */



#include <iostream>
#include "../UgrConnector.hh"
#include <stdio.h>

using namespace std;


int main(int argc, char **argv) {

    if (argc != 4) {
        cout << "Usage: " << argv[0] << " <repeat count> <cfgfile> <key to stat or lfn>" << endl;
        exit(1);
    }

    UgrConnector ugr;
    UgrFileInfo *fi = 0;

    long long cnt = atoll(argv[1]);

    cout << "Initializing" << endl;
    if (ugr.init(argv[2]))
        return 1;

    cout << "Invoking stat" << endl;
    string fn = argv[3];

    for (long long i = 0; i < cnt; i++) {
        //char buf[16];
        //sprintf(buf, "%d", i);
        //string fn1 = fn + buf;
        ugr.stat(fn, &fi);
    }

    cout << "Results:" << endl;
    fi->print(cout);


}
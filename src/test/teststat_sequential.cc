
/* teststat sequential
 * Initialize the connector with the given cfgfile
 * and do a stat of the given file
 *
 * by Fabrizio Furano, CERN, Nov 2011
 *  and Devresse Adrien
 */



#include <iostream>
#include "../UgrConnector.hh"
#include <stdio.h>
#include <sys/stat.h>

using namespace std;


// execute a chain of stat sequentially ( keep state test )

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



    cout << "Invoking stat" << cnt-1 << " times." << endl;
    for (long long i = 0; i < cnt; i++) {
        ugr.stat(fn, &fi);

        ugr.stat(fn, &fi);

        if (fi->getStatStatus() == UgrFileInfo::Ok) {
                if (fi->unixflags & S_IFDIR) ugr.list(fn, &fi);
                else ugr.locate(fn, &fi);
        }

        cout << "Results:" << endl;
        fi->print(cout);
    }





}


/* LocationInfo
 * Handling of the info that is kept per each file
 *
 *
 * by Fabrizio Furano, CERN, Jul 2011
 */

#include "LocationInfo.hh"


using namespace boost;
using namespace std;

int UgrFileInfo::getBestReplicaIdx(std::string &clientlocation) {
    return 0;
}

int UgrFileInfo::waitForSomeUpdate(unique_lock<mutex> &l, int sectmout) {

    system_time const timeout = get_system_time()+posix_time::seconds(sectmout);


    // I am still skeptical on the possibility of having so many condition variables
    // Maybe this has to be taken from a pool of condvars, for the moment it stays as it is
    if (!condvar.timed_wait(l, timeout ))
        return 1; // timeout
    else
        return 0; // signal catched

    return 0;
}

int UgrFileInfo::signalSomeUpdate() {
    condvar.notify_all();

    return 0;
}



void UgrFileInfo::print(ostream &out) {
    out << "Name: " << name << endl;
    out << "Size:" << size << endl;
    for (unsigned int i = 0; i < subitems.size(); i++) {
        out << "loc: \"" << subitems[i]->location << "\" name: \"" << subitems[i]->name << "\"" << endl;
    }

}
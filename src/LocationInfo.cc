/** @file   LocationInfo.cc
 * @brief  Handling of the info that is kept per each file
 * @author Fabrizio Furano
 * @date   Jul 2011
 */

#include "LocationInfo.hh"
#include<iomanip>

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

int UgrFileInfo::waitStat(boost::unique_lock<boost::mutex> &l, int sectmout) {
    const char *fname = "UgrFileInfo::waitStat";

    // If still pending, we wait for the file object to get a notification
    // then we recheck...
    time_t timelimit = time(0)+sectmout;

    Info(SimpleDebug::kHIGHEST, fname, "Starting check-wait. Name: " << name);

    while (getStatStatus() == InProgress) {
        // Ignore the timeouts, exit only on an explicit notification
        waitForSomeUpdate(l, 1);
        // On global timeout... stop waiting
        if (time(0) > timelimit) {
            Info(SimpleDebug::kHIGH, fname, "Timeout");
            break;
        }
    }

    Info(SimpleDebug::kHIGH, fname, "Finished check-wait. Name: " << name << " Status: " << getStatStatus() <<
            " status_statinfo: " << status_statinfo << " pending_statinfo: " << pending_statinfo);

    // We are here if the plugins have finished OR in the case of timeout
    // If the stat is still marked as in progress it means that some plugin is very late.
    if ((getStatStatus() == InProgress) && (status_statinfo == NoInfo))
            status_statinfo = NotFound;

    return 0;
}

int UgrFileInfo::waitLocations(boost::unique_lock<boost::mutex> &l, int sectmout) {
const char *fname = "UgrFileInfo::waitLocations";

    // If still pending, we wait for the file object to get a notification
    // then we recheck...
    time_t timelimit = time(0)+sectmout;

    Info(SimpleDebug::kHIGHEST, fname, "Starting check-wait. Name: " << name);

    while (getLocationStatus() == InProgress) {
        // Ignore the timeouts, exit only on an explicit notification
        waitForSomeUpdate(l, 1);
        // On global timeout... stop waiting
        if (time(0) > timelimit) {
            Info(SimpleDebug::kHIGH, fname, "Timeout");
            break;
        }
    }

    Info(SimpleDebug::kHIGH, fname, "Finished check-wait. Name: " << name << " Status: " << getLocationStatus() <<
            " status_locations: " << status_locations << " pending_locations: " << pending_locations);

    // We are here if the plugins have finished OR in the case of timeout
    // If the loc is still marked as in progress it means that some plugin is very late.
    if ((getLocationStatus() == InProgress) && (status_locations == NoInfo))
            status_locations = NotFound;

    return 0;
}

int UgrFileInfo::waitItems(boost::unique_lock<boost::mutex> &l, int sectmout) {
    const char *fname = "UgrFileInfo::waitItems";

    // If still pending, we wait for the file object to get a notification
    // then we recheck...
    time_t timelimit = time(0)+sectmout;

    Info(SimpleDebug::kHIGHEST, fname, "Starting check-wait. Name: " << name);

    while (getItemsStatus() == UgrFileInfo::InProgress) {
        // Ignore the timeouts, exit only on an explicit notification
        waitForSomeUpdate(l, 2);
        // On global timeout... stop waiting
        if (time(0) > timelimit) {
            Info(SimpleDebug::kHIGH, fname, "Timeout");
            break;
        }
    }

    Info(SimpleDebug::kHIGH, fname, "Finished check-wait. Name: " << name << " Status: " << getItemsStatus()
            << " status_items: " << status_items << " pending_items: " << pending_items);

    // We are here if the plugins have finished OR in the case of timeout
    // If the loc is still marked as in progress it means that some plugin is very late.
    if ((getItemsStatus() == InProgress) && (status_items == NoInfo))
            status_items = NotFound;

    return 0;
}
   

void UgrFileInfo::print(ostream &out) {
    
    out << "Name: " << name << endl;

    if (this->status_statinfo == UgrFileInfo::NotFound) {
        out << "The file does not exist." << endl;
        return;

    }

    out << "Size:" << size << endl;
    out << "Flags:" << setbase(8) << unixflags << setbase(10) << endl;

    for (std::set<UgrFileItem>::iterator i = subitems.begin(); i != subitems.end(); i++) {
        out << "loc: \"" << i->location << "\" name: \"" << i->name << "\"" << endl;
    }

}
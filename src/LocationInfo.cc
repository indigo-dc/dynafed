/** @file   LocationInfo.cc
 * @brief  Handling of the info that is kept per each file
 * @author Fabrizio Furano
 * @date   Jul 2011
 */

#include "LocationInfo.hh"
#include "UgrMemcached.pb.h"
#include <iomanip>


using namespace boost;
using namespace std;
using namespace ugrmemcached;

int UgrFileInfo::getBestReplicaIdx(std::string &clientlocation) {
    return 0;
}

int UgrFileInfo::waitForSomeUpdate(unique_lock<mutex> &l, int sectmout) {

    system_time const timeout = get_system_time() + posix_time::seconds(sectmout);


    // I am still skeptical on the possibility of having so many condition variables
    // Maybe this has to be taken from a pool of condvars, for the moment it stays as it is
    if (!condvar.timed_wait(l, timeout))
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
    time_t timelimit = time(0) + sectmout;

    Info(SimpleDebug::kHIGHEST, fname, "Starting check-wait. Name: " << name << " Status: " << getStatStatus() <<
            " status_statinfo: " << status_statinfo << " pending_statinfo: " << pending_statinfo);

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
    time_t timelimit = time(0) + sectmout;

    Info(SimpleDebug::kHIGHEST, fname, "Starting check-wait. Name: " << name << " Status: " << getLocationStatus() <<
            " status_locations: " << status_locations << " pending_locations: " << pending_locations);

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
    time_t timelimit = time(0) + sectmout;

    Info(SimpleDebug::kHIGHEST, fname, "Starting check-wait. Name: " << name << " Status: " << getItemsStatus() <<
            " status_items: " << status_items << " pending_items: " << pending_items);

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

    for (std::set<UgrFileItem>::iterator i = subdirs.begin(); i != subdirs.end(); i++) {
        out << "subdir: \"" << "\" name: \"" << i->name << "\"" << endl;
    }

    for (std::set<UgrFileItem_replica>::iterator i = replicas.begin(); i != replicas.end(); i++) {
        out << "replica: \"" << i->location << "\" name: \"" << i->name << "\"" << endl;
    }

}


int UgrFileInfo::encodeToString(std::string &str) {
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    SerialUgrfileInfo sufi;

    sufi.set_atime(atime);
    sufi.set_ctime(ctime);
    sufi.set_group(group);
    sufi.set_mtime(mtime);
    sufi.set_owner(owner);
    sufi.set_size(size);
    sufi.set_unixflags(unixflags);

    str = sufi.SerializeAsString();


    return (str.length() > 0);
};

int UgrFileInfo::decode(void *data, int sz) {
    if (!sz) return 1;

    SerialUgrfileInfo sufi;

    sufi.ParseFromArray(data, sz);

    atime = sufi.atime();
    ctime = sufi.ctime();
    group = sufi.group();
    mtime = sufi.mtime();
    owner = sufi.owner();
    size = sufi.size();
    unixflags = sufi.unixflags();
    status_statinfo = Ok;
    dirty = false;
    touch();

    return 0;
};


/// We will like to be able to encode this info to a string, e.g. for external caching purposes

int UgrFileInfo::encodeSubitemsToString(std::string &str) {
    GOOGLE_PROTOBUF_VERIFY_VERSION;


    if (unixflags & S_IFDIR) {
        // It's a dir, save the subdirs
        SerialUgrSubdirs dirlist;
        SerialUgrSubdir* pnt;

        std::set<UgrFileItem, UgrFileItemComp>::iterator it;

        for (it = subdirs.begin();
                it != subdirs.end();
                it++) {
            pnt = dirlist.add_subdirs();

            pnt->set_name(it->name);
        }


        str = dirlist.SerializeAsString();
    } else {
        // save the replicas
        SerialUgrReplicas replist;
        SerialUgrReplica* pnt;

        std::set<UgrFileItem_replica, UgrFileItemComp>::iterator it;

        for (it = replicas.begin();
                it != replicas.end();
                it++) {
            pnt = replist.add_replicas();

            pnt->set_name(it->name);

            pnt->set_latitude(it->latitude);
            pnt->set_location(it->location);
            pnt->set_longitude(it->longitude);
        }

        str = replist.SerializeAsString();
    }

    //list.PrintDebugString();

    return (str.length() > 0);
}

/// We will like to be able to encode this info to a string, e.g. for external caching purposes

int UgrFileInfo::decodeSubitems(void *data, int sz) {
    if (!sz) return 1;



    if (unixflags & S_IFDIR) {
        // List of subdirs
        SerialUgrSubdirs dirlist;
        SerialUgrSubdir dir;

        dirlist.ParseFromArray(data, sz);
        //list.PrintDebugString();

        for (int i = 0; i < dirlist.subdirs_size(); i++) {
            UgrFileItem itr;

            dir = dirlist.subdirs(i);
            itr.name = dir.name();
            subdirs.insert(itr);

        }
        status_items = Ok;

    } else {
        // List of replicas
        SerialUgrReplicas replist;
        SerialUgrReplica rep;
        
        replist.ParseFromArray(data, sz);
        //list.PrintDebugString();

        for (int i = 0; i < replist.replicas_size(); i++) {
            UgrFileItem_replica itr;


            rep = replist.replicas(i);
            itr.name = rep.name();
            itr.latitude = rep.latitude();
            itr.longitude = rep.longitude();
            itr.location = rep.location();
            replicas.insert(itr);
        }
        
        status_locations = Ok;
        
    }



    dirtyitems = false;
    return 0;

}



/// Clean up a path, make sure it ends without a slash

void UgrFileInfo::trimpath(std::string & s) {

    while (*(s.rbegin()) == '/')
        s.erase(s.size() - 1);

    if (s.length() == 0) s = "/";


}




void UgrFileInfo::takeStat(struct stat &st) {
    const char *fname = "UgrLocPlugin_dmlite::takeStat";
    Info(SimpleDebug::kHIGHEST, fname, "Worker: stat info:" << st.st_size << " " << st.st_mode);
    
    unique_lock<mutex> l2(*this);
    size = st.st_size;
    unixflags = st.st_mode;
    if (st.st_atim.tv_sec && (st.st_atim.tv_sec > atime)) atime = st.st_atim.tv_sec;
    if (st.st_mtim.tv_sec && (st.st_mtim.tv_sec > mtime)) mtime = st.st_mtim.tv_sec;
    if (st.st_ctim.tv_sec && (st.st_ctim.tv_sec < ctime)) ctime = st.st_ctim.tv_sec;

    
    status_statinfo = UgrFileInfo::Ok;

    if ((long) st.st_nlink > CFG->GetLong("glb.maxlistitems", 2000)) {
        Info(SimpleDebug::kMEDIUM, fname, "Setting as non listable. nlink=" << st.st_nlink);
        subdirs.clear();
        status_items = UgrFileInfo::Error;
    }



    dirty = true;

}
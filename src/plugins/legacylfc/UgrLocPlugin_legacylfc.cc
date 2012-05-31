#include "UgrLocPlugin_legacylfc.hh"
#include "../../PluginLoader.hh"
#include <time.h>


using namespace boost;
using namespace std;

void lfcworker(UgrLocPlugin_legacylfc* plugin)
{
    const char *fname = "UgrLocPlugin_legacylfc::lfcworker";
    string wrktoken;

    posix_time::seconds workTime(4);

    Info(SimpleDebug::kLOW, fname, "Worker: running.");

    do {
        // Wait for an element
        system_time const timeout = get_system_time()+posix_time::seconds(1);
        unique_lock<mutex> qlck(plugin->qmtx);
        
        if (plugin->wrkqueue.size()) {
            plugin->runitem(*plugin->wrkqueue.begin());
            plugin->wrkqueue.pop_front();
        }
        else
            plugin->qcond.timed_wait(qlck, timeout);
        
    } while(1);

    
    Info(SimpleDebug::kLOW, fname, "Worker: finished");

}

void UgrLocPlugin_legacylfc::runitem(UgrFileInfo *fi) {

    // Contact the LFC, in this simple version just get all the
    // possible information
    // Cthread_init
    // startsession
    // stat
    // diropen
    // listreplicas


    {
        unique_lock<mutex> l(*fi);

        // Create a fake stat information
        fi->lastupdtime = time(0);
        fi->size = 12345;
        fi->status_statinfo = UgrFileInfo::Ok;
        fi->unixflags = 0777;

        // Create a fake list information
        for (int ii = 0; ii < 10; ii++) {
            UgrFileItem *fit = new UgrFileItem();
            fit->name = "myhost/myfilepath" + boost::lexical_cast<std::string>(ii);
            fit->location = "Gal.Coord. 2489573495.37856.34765347865.3478563487";
            fi->subitems.push_back(fit);
        }

        fi->status_locations = UgrFileInfo::Ok;



        // In this fake implementation, we notify here that this plugin
        // has finished searching for the info

        fi->notifyStatNotPending();
        fi->notifyLocationNotPending();
        fi->notifyItemsNotPending();
    }

}
// Start the async stat process
// Mark the fileinfo with one more pending stat request (by this plugin)
int UgrLocPlugin_legacylfc::do_Stat(UgrFileInfo* fi) {
    const char *fname = "UgrLocPlugin_legacylfc::do_Stat";

    // We immediately notify that this plugin is starting a search for this info
    // Depending on the plugin, the symmetric notifyNotPending() will be done
    // in a parallel thread, or inside do_waitstat
    fi->notifyStatPending();

    // Push the entry to be looked up into the worker's queue
    {
        unique_lock<mutex> qlck(qmtx);
        wrkqueue.push_back(fi);
        qcond.notify_all();
    }


    Info(SimpleDebug::kLOW, fname << name, "Stub!");
    return 0;
};

// Waits max a number of seconds for a stat task to be complete
// Exit with timeout or if this plugin has finished
// E.g. this plugin may decide that it has finished if it detected
// that all the hosts that had to asynchronously respond actually responded
//
// A trivial implementation could be a direct sync query, for instance to a DB
//
// A more complex implementation could be totally async, where, at the reception
// of any notification-end, the async task marks the sending host as done for that
// request
//
// Another implementation could just wait N seconds and take what arrived
//
// Another implementation could just take the first response that comes
// (for a stat this could be good, not so for a locate, not at all for a list)
//
// Maybe a good middle point could be to wait for at least M responses or
// N seconds at max, where M is the number of hosts that are known
//
// The result will be in the FileInfo object

int UgrLocPlugin_legacylfc::do_waitStat(UgrFileInfo *fi, int tmout) {
    const char *fname = "UgrLocPlugin_legacylfc::do_waitStat";

    unique_lock<mutex> lck(*fi);

    // If still pending, we wait for the file object to get a notification
    // then we recheck...
    time_t timelimit = time(0)+tmout;

    while (fi->getStatStatus() == UgrFileInfo::InProgress) {
        // Ignore the timeouts, exit only on an explicit notification
        fi->waitForSomeUpdate(lck, 1);
        // On global timeout... stop waiting
        if (time(0) > timelimit) break;
    }
    Info(SimpleDebug::kLOW, fname << name, "Stub!");
    return 0;
}

// Start the async location process
// In practice, trigger all the location plugins, possibly together,
// so they act concurrently

int UgrLocPlugin_legacylfc::do_Locate(UgrFileInfo *fi) {
    const char *fname = "UgrLocPlugin_legacylfc::do_waitStat";


    // We immediately notify that this plugin is starting a search for this info
    // Depending on the plugin, the symmetric notifyNotPending() will be done
    // in a parallel thread, or inside do_waitstat
    fi->notifyLocationPending();
    // Push the entry to be looked up into the worker's queue
    {
        unique_lock<mutex> qlck(qmtx);
        wrkqueue.push_back(fi);
        qcond.notify_all();
    }
    
    Info(SimpleDebug::kLOW, fname << name, "Stub!");
    return 0;
}

// Waits max a number of seconds for a locate process to be complete

int UgrLocPlugin_legacylfc::do_waitLocate(UgrFileInfo *fi, int tmout) {
    const char *fname = "UgrLocPlugin_legacylfc::do_waitStat";

    unique_lock<mutex> lck(*fi);

    // If still pending, we wait for the file object to get a notification
    // then we recheck...
    time_t timelimit = time(0)+tmout;

    while (fi->getLocationStatus() == UgrFileInfo::InProgress) {
        // Ignore the timeouts, exit only on an explicit notification
        fi->waitForSomeUpdate(lck, 1);
        // On global timeout... stop waiting
        if (time(0) > timelimit) break;
    }


    Info(SimpleDebug::kLOW, fname << name, "Stub!");
    return 0;
}

// Start the async listing process
// In practice, trigger all the location plugins, possibly together,
// so they act concurrently

int UgrLocPlugin_legacylfc::do_List(UgrFileInfo *fi) {
    const char *fname = "UgrLocPlugin_legacylfc::do_waitStat";


    // We immediately notify that this plugin is starting a search for this info
    // Depending on the plugin, the symmetric notifyNotPending() will be done
    // in a parallel thread, or inside do_waitstat
    fi->notifyItemsPending();
    // Push the entry to be looked up into the worker's queue
    {
        unique_lock<mutex> qlck(qmtx);
        wrkqueue.push_back(fi);
        qcond.notify_all();
    }

    Info(SimpleDebug::kLOW, fname << name, "Stub!");
    return 0;
}

// Waits max a number of seconds for a list process to be complete

int UgrLocPlugin_legacylfc::do_waitList(UgrFileInfo *fi, int tmout) {
    const char *fname = "UgrLocPlugin_legacylfc::do_waitStat";

    unique_lock<mutex> lck(*fi);

    // If still pending, we wait for the file object to get a notification
    // then we recheck...
    time_t timelimit = time(0)+tmout;

    while (fi->getLocationStatus() == UgrFileInfo::InProgress) {
        // Ignore the timeouts, exit only on an explicit notification
        fi->waitForSomeUpdate(lck, 2);
        // On global timeout... stop waiting
        if (time(0) > timelimit) break;
    }



    Info(SimpleDebug::kLOW, fname, "Stub!");
    return 0;
}















// ------------------------------------------------------------------------------------
// Plugin-related stuff
// ------------------------------------------------------------------------------------


// The hook function. GetLocationPluginClass must be given the name of this function
// for the plugin to be loaded

extern "C" LocationPlugin *GetLocationPlugin(GetLocationPluginArgs) {
    return (LocationPlugin *)new UgrLocPlugin_legacylfc(dbginstance, cfginstance, parms);
}

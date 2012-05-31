#include "LocationPlugin.hh"
#include "PluginLoader.hh"
#include <time.h>


using namespace boost;

void fakepluginFunc(UgrFileInfo* fi)
{
    const char *fname = "LocationPlugin::fakepluginFunc";

    boost::posix_time::seconds workTime(4);

    Info(SimpleDebug::kLOW, fname, "Worker: running on " << fi->name);

    // Pretend to do something useful...
    boost::this_thread::sleep(workTime);

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



        // In this fake implementation, we notify here that this plugin
        // has finished searching for the info

        fi->notifyStatNotPending();
        fi->notifyLocationNotPending();
        fi->notifyItemsNotPending();
    }

    Info(SimpleDebug::kLOW, fname, "Worker: finished");

}


// Start the async stat process
// Mark the fileinfo with one more pending stat request (by this plugin)

int LocationPlugin::do_Stat(UgrFileInfo* fi) {
    const char *fname = "LocationPlugin::do_Stat";

    // We immediately notify that this plugin is starting a search for this info
    // Depending on the plugin, the symmetric notifyNotPending() will be done
    // in a parallel thread, or inside do_waitstat
    fi->notifyStatPending();

    // This plugin is a fake one, that spawns a thread which populates the result after some time
    boost::thread workerThread(fakepluginFunc, fi);

    Info(SimpleDebug::kLOW, fname, "Stub!");
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

int LocationPlugin::do_waitStat(UgrFileInfo *fi, int tmout) {
    //const char *fname = "LocationPlugin::do_waitStat";

    unique_lock<mutex> lck(*fi);

    // If still pending, we wait for the file object to get a notification
    // then we recheck...
    time_t timelimit = time(0)+tmout;
    
    while (fi->getStatStatus() == UgrFileInfo::InProgress) {
        // Ignore the timeouts, exit only on an explicit notification
        if (!fi->waitForSomeUpdate(lck, 1)) break;
        // On global timeout... stop waiting
        if (time(0) > timelimit) break;
    }

    return 0;
}

// Start the async location process
// In practice, trigger all the location plugins, possibly together,
// so they act concurrently

int LocationPlugin::do_Locate(UgrFileInfo *fi) {
    const char *fname = "LocationPlugin::do_waitStat";


    // We immediately notify that this plugin is starting a search for this info
    // Depending on the plugin, the symmetric notifyNotPending() will be done
    // in a parallel thread, or inside do_waitstat
    fi->notifyLocationPending();

    // This plugin is a fake one, that spawns a thread which populates the result after some time
    boost::thread workerThread(fakepluginFunc, fi);

    Info(SimpleDebug::kLOW, fname, "Stub!");
    return 0;
}

// Waits max a number of seconds for a locate process to be complete

int LocationPlugin::do_waitLocate(UgrFileInfo *fi, int tmout) {
    const char *fname = "LocationPlugin::do_waitStat";

    unique_lock<mutex> lck(*fi);

    // If still pending, we wait for the file object to get a notification
    // then we recheck...
    time_t timelimit = time(0)+tmout;

    while (fi->getLocationStatus() == UgrFileInfo::InProgress) {
        // Ignore the timeouts, exit only on an explicit notification
        if (!fi->waitForSomeUpdate(lck, 1)) break;
        // On global timeout... stop waiting
        if (time(0) > timelimit) break;
    }


    Info(SimpleDebug::kLOW, fname, "Stub!");
    return 0;
}

// Start the async listing process
// In practice, trigger all the location plugins, possibly together,
// so they act concurrently

int LocationPlugin::do_List(UgrFileInfo *fi) {
    const char *fname = "LocationPlugin::do_waitStat";

    
    // We immediately notify that this plugin is starting a search for this info
    // Depending on the plugin, the symmetric notifyNotPending() will be done
    // in a parallel thread, or inside do_waitstat
    fi->notifyItemsPending();

    // This plugin is a fake one, that spawns a thread which populates the result after some time
    boost::thread workerThread(fakepluginFunc, fi);

    Info(SimpleDebug::kLOW, fname, "Stub!");
    return 0;
}

// Waits max a number of seconds for a list process to be complete

int LocationPlugin::do_waitList(UgrFileInfo *fi, int tmout) {
    const char *fname = "LocationPlugin::do_waitStat";

    unique_lock<mutex> lck(*fi);

    // If still pending, we wait for the file object to get a notification
    // then we recheck...
    time_t timelimit = time(0)+tmout;

    while (fi->getLocationStatus() == UgrFileInfo::InProgress) {
        // Ignore the timeouts, exit only on an explicit notification
        if (!fi->waitForSomeUpdate(lck, 2)) break;
        // On global timeout... stop waiting
        if (time(0) > timelimit) break;
    }



    Info(SimpleDebug::kLOW, fname, "Stub!");
    return 0;
}















// ------------------------------------------------------------------------------------
// Plugin-related stuff
// ------------------------------------------------------------------------------------



// The plugin functionality. This function invokes the plugin loader, looking for the
// plugin where to call the hook function

LocationPlugin *GetLocationPluginClass(char *pluginPath, GetLocationPluginArgs) {
    const char *fname = "GetLocationPluginClass_local";
    PluginLoader *myLib;
    LocationPlugin * (*ep)(GetLocationPluginArgs);

    // If we have no plugin path then return NULL
    if (!pluginPath || !strlen(pluginPath)) {
        Info(SimpleDebug::kMEDIUM, fname, "No plugin to load.");
        return NULL;
    }

    // Create a plugin object (we will throw this away without deletion because
    // the library must stay open but we never want to reference it again).
    Info(SimpleDebug::kMEDIUM, fname, "Loading plugin " << pluginPath);
    if (!(myLib = new PluginLoader(pluginPath))) {
        Info(SimpleDebug::kLOW, fname, "Failed loading plugin " << pluginPath);
        return NULL;
    }

    // Now get the entry point of the object creator
    Info(SimpleDebug::kMEDIUM, fname, "Getting entry point for plugin " << pluginPath);
    ep = (LocationPlugin * (*)(GetLocationPluginArgs))(myLib->getPlugin("GetLocationPlugin"));
    if (!ep) {
        Info(SimpleDebug::kLOW, fname, "Could not get entry point for plugin " << pluginPath);
        return NULL;
    }

    // Get the Object now
    Info(SimpleDebug::kMEDIUM, fname, "Getting class instance for plugin " << pluginPath);
    LocationPlugin *c = ep(dbginstance, cfginstance);
    if (!c)
        Info(SimpleDebug::kLOW, fname, "Could not get class instance for plugin " << pluginPath);
    return c;

}


// The hook function. GetLocationPluginClass must be given the name of this function
// for the plugin to be loaded

extern "C" LocationPlugin *GetLocationPlugin(GetLocationPluginArgs) {
    return (LocationPlugin *)new LocationPlugin(dbginstance, cfginstance);
}

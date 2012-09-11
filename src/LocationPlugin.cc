/** @file   LocationPlugin.cc
 * @brief  Base class for an UGR location plugin
 * @author Fabrizio Furano
 * @date   Oct 2011
 */
#include "LocationPlugin.hh"
#include "PluginLoader.hh"
#include <time.h>
#include <sys/stat.h>

using namespace boost;
using namespace std;

void pluginFunc(LocationPlugin *pl, int myidx) {
    const char *fname = "LocationPlugin::pluginFunc";
    Info(SimpleDebug::kHIGHEST, fname, "Worker: started");

    // Get some work to do
    while (!pl->exiting) {

    
        struct LocationPlugin::worktoken *op = pl->getOp();
        if (op && op->fi && op->wop) {

            // Run this search, including notifying the various calls
            pl->runsearch(op, myidx);

        }
    }

    Info(SimpleDebug::kHIGHEST, fname, "Worker: finished");

}

LocationPlugin::LocationPlugin(SimpleDebug *dbginstance, Config *cfginstance, std::vector<std::string> &parms) {
    SimpleDebug::Instance()->Set(dbginstance);
    CFG->Set(cfginstance);

    const char *fname = "LocationPlugin::LocationPlugin";
    nthreads = 0;
    

    if (parms.size() > 1)
        name = strdup(parms[1].c_str());
    else name = strdup("Unnamed");

    if (parms.size() > 2)
        nthreads = atoi(parms[2].c_str());

    if (nthreads < 0) {
        Error(fname, "Fixing nthreads: " << nthreads << "-->2");
        nthreads = 2;
    }

    if (nthreads > 10000) {
        Error(fname, "Fixing nthreads: " << nthreads << "-->10000")
        nthreads = 10000;
    }


    // Now get from the config any item built as:
    // locplugin.<name>.variablename
    // or
    // locplugin.<name>.arrayname[]
    // es.
    // locplugin.dmlite1.xlatepfx /dpm/cern.ch/ /
    // locplugin.http1.host[] http://exthost.y.z/path_pfx_to_strip

    std::string s = "locplugin.";
    s += name;
    s += ".xlatepfx";
    
    std::string v;
    v = CFG->GetString(s.c_str(), (char *)"");

    if (v.size() > 0) {
        vector<string> parms = tokenize(v, " ");
        if (parms.size() < 2) {
            Error(fname, "Bad xlatepfx: '" << v << "'");
        }
        else {
            xlatepfx_from = parms[0];
            xlatepfx_to = parms[1];
        }
    }

    geoPlugin = 0;

    exiting = false;
    
    
};



void LocationPlugin::stop() {
    const char *fname = "LocationPlugin::stop";

    exiting = true;

    /// Note: this tends to hang due to a known bug in boost
    //for (unsigned int i = 0; i < workers.size(); i++) {
//        LocPluginLogInfo(SimpleDebug::kLOW, fname, "Interrupting thread: " << i);
//        workers[i]->interrupt();
//    }

    for (unsigned int i = 0; i < workers.size(); i++) {

        pushOp(0, 0, wop_Nop);
    }

    for (unsigned int i = 0; i < workers.size(); i++) {
        LocPluginLogInfo(SimpleDebug::kLOW, fname, "Joining thread: " << i);
        workers[i]->join();
    }

    LocPluginLogInfo(SimpleDebug::kLOW, fname, "Deleting " << workers.size() << " threads. ");
    while (workers.size() > 0) {
        delete *workers.begin();
        workers.erase(workers.begin());
    }
}

int LocationPlugin::start() {
    const char *fname = "LocationPlugin::start";

    // Create our pool of threads
    LocPluginLogInfo(SimpleDebug::kLOW, fname, "creating " << nthreads << " threads.");
    for (int i = 0; i < nthreads; i++) {
            workers.push_back(new boost::thread(pluginFunc, this, i));
    }

    return 0;
}


LocationPlugin::~LocationPlugin() {
    
}

// Pushes a new op in the queue
void LocationPlugin::pushOp(UgrFileInfo *fi, LocationInfoHandler *handler, workOp wop) {
    const char *fname = "LocationPlugin::pushOp";

    {
        boost::lock_guard< boost::mutex > l(workmutex);

        worktoken *tk = new(worktoken);
        tk->fi = fi;
        tk->wop = wop;
        tk->handler = handler;
        workqueue.push_back(tk);
    }

    LocPluginLogInfo(SimpleDebug::kHIGHEST, fname, "pushed op:" << wop);

    workcondvar.notify_one();

}

// Gets an op from the queue, or timeout
struct LocationPlugin::worktoken *LocationPlugin::getOp() {
    struct worktoken *mytk = 0;
    const char *fname = "LocationPlugin::getOp";

    boost::unique_lock< boost::mutex > l(workmutex);

    system_time const timeout = get_system_time() + posix_time::seconds(10);

    while (!mytk) {
        // Defensive programming...
        if (workqueue.size() > 0) {
            mytk = workqueue.front();
            workqueue.pop_front();
            break;
        }

        if (!workcondvar.timed_wait(l, timeout))
            break; // timeout


    }

    if (mytk) {
        LocPluginLogInfo(SimpleDebug::kHIGHEST, fname, "got op:" << mytk->wop);
    }

    return mytk;
}

void LocationPlugin::runsearch(struct worktoken *op, int myidx) {
    const char *fname = "LocationPlugin::runsearch";

    // Pretend to do something useful...
    boost::posix_time::seconds workTime(1);
    boost::this_thread::sleep(workTime);

    LocPluginLogInfoThr(SimpleDebug::kMEDIUM, fname, "Starting op: " << op->wop << "fn: " << op->fi->name);

    // Now put the results
    {

        unique_lock<mutex> l(*(op->fi));


        // This fake plugin happens to gather more information than it's requested
        // ...this may happen, in this case the plugin writes all the info that it has
        // BUT the notification has to be only for the operation that was requested


        // Create a fake stat information
        if (op->fi->status_statinfo != UgrFileInfo::Ok) {
            op->fi->lastupdtime = time(0);
            op->fi->size = 12345;
            op->fi->status_statinfo = UgrFileInfo::Ok;
            op->fi->unixflags = 0775;
            op->fi->unixflags |= S_IFDIR;
        }
        
        // Create a fake list information
        UgrFileItem fit;
        for (int ii = 0; ii < 10; ii++) {
            fit.name = "myfilepath" + boost::lexical_cast<std::string > (ii);
            fit.location = "Gal.Coord. 2489573495.37856.34765347865.3478563487";
            op->fi->subdirs.insert(fit);
        }
        

        // We have modified the data, hence set the dirty flag
        op->fi->dirty = true;

        // Anyway the notification has to be correct, not redundant
        switch (op->wop) {

            case LocationPlugin::wop_Stat:
                op->fi->notifyStatNotPending();
                break;

            case LocationPlugin::wop_Locate:
                op->fi->status_locations = UgrFileInfo::Ok;
                op->fi->notifyLocationNotPending();
                break;

            case LocationPlugin::wop_List:
                op->fi->status_items = UgrFileInfo::Ok;
                op->fi->notifyItemsNotPending();
                break;

            default:
                break;
        }

        LocPluginLogInfoThr(SimpleDebug::kMEDIUM, fname, "Finished op: " << op->wop << "fn: " << op->fi->name);

    }
}

















// Start the async stat process
// Mark the fileinfo with one more pending stat request (by this plugin)
int LocationPlugin::do_Stat(UgrFileInfo* fi, LocationInfoHandler *handler) {
    const char *fname = "LocationPlugin::do_Stat";

    LocPluginLogInfo(SimpleDebug::kHIGHEST, fname, "Entering");

    // We immediately notify that this plugin is starting a search for this info
    // Depending on the plugin, the symmetric notifyNotPending() will be done
    // in a parallel thread, or inside do_waitstat
    fi->notifyStatPending();

    pushOp(fi, handler, wop_Stat);

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
    return 0;
}

// Start the async location process
// In practice, trigger all the location plugins, possibly together,
// so they act concurrently
int LocationPlugin::do_Locate(UgrFileInfo *fi, LocationInfoHandler *handler) {
    const char *fname = "LocationPlugin::do_Locate";

    LocPluginLogInfo(SimpleDebug::kHIGHEST, fname, "Entering");

    // We immediately notify that this plugin is starting a search for this info
    // Depending on the plugin, the symmetric notifyNotPending() will be done
    // in a parallel thread, or inside do_waitstat
    fi->notifyLocationPending();

    pushOp(fi, handler, wop_Locate);

    return 0;
}

// Waits max a number of seconds for a locate process to be complete
int LocationPlugin::do_waitLocate(UgrFileInfo *fi, int tmout) {
    return 0;
}

// Start the async listing process
// In practice, trigger all the location plugins, possibly together,
// so they act concurrently
int LocationPlugin::do_List(UgrFileInfo *fi, LocationInfoHandler *handler) {
    const char *fname = "LocationPlugin::do_List";

    LocPluginLogInfo(SimpleDebug::kHIGHEST, fname, "Entering");

    // We immediately notify that this plugin is starting a search for this info
    // Depending on the plugin, the symmetric notifyNotPending() will be done
    // in a parallel thread, or inside do_waitstat
    fi->notifyItemsPending();

    pushOp(fi, handler, wop_List);

    return 0;
}

// Waits max a number of seconds for a list process to be complete
int LocationPlugin::do_waitList(UgrFileInfo *fi, int tmout) {
    return 0;
}

// default implment, overloading should be fast 
void LocationPlugin::check_availability(PluginEndpointStatus * status, UgrFileInfo *fi){
	status->state= PLUGIN_ENDPOINT_ONLINE;
	status->latency = 0;
	status->explanation = "";
}













// ------------------------------------------------------------------------------------
// Plugin-related stuff
// ------------------------------------------------------------------------------------



// The plugin functionality. This function invokes the plugin loader, looking for the
// plugin where to call the hook function
LocationPlugin *GetLocationPluginClass(char *pluginPath, GetLocationPluginArgs) {
    const char *fname = "GetLocationPluginClass_local";
    PluginLoader *myLib = 0;
    LocationPlugin * (*ep)(GetLocationPluginArgs);

    // If we have no plugin path then return NULL
    if (!pluginPath || !strlen(pluginPath)) {
        Info(SimpleDebug::kMEDIUM, fname, "No plugin to load.");
        return NULL;
    }

    // Create a plugin object (we will throw this away without deletion because
    // the library must stay open but we never want to reference it again).
    if (!myLib) {
        Info(SimpleDebug::kMEDIUM, fname, "Loading plugin " << pluginPath);
        if (!(myLib = new PluginLoader(pluginPath))) {
            Info(SimpleDebug::kLOW, fname, "Failed loading plugin " << pluginPath);
            return NULL;
        }
    } else {
        Info(SimpleDebug::kMEDIUM, fname, "Plugin " << pluginPath << "already loaded.");
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
    LocationPlugin *c = ep(dbginstance, cfginstance, parms);
    if (!c)
        Info(SimpleDebug::kLOW, fname, "Could not get class instance for plugin " << pluginPath);
    return c;

}




/// The plugin hook function. GetLocationPluginClass must be given the name of this function
/// for the plugin to be loaded
extern "C" LocationPlugin *GetLocationPlugin(GetLocationPluginArgs) {
    return (LocationPlugin *)new LocationPlugin(dbginstance, cfginstance, parms);
}

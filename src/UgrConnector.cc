/** @file   UgrConnector.cc
 * @brief  Base class that gives the functionalities of a dynamic, protocol-agnostic redirector
 * @author Fabrizio Furano
 * @date   Jul 2011
 */


#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>

#include "SimpleDebug.hh"
#include "PluginLoader.hh"
#include <string>
#include "UgrConnector.hh"
#include "LocationInfo.hh"
#include "LocationInfoHandler.hh"



using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::system;


#define EXECUTE_ON_AVAILABLE(myfunc) \
    do{ \
    for (unsigned int i = 0; i < locPlugins.size(); i++) { \
        if (locPlugins[i].) \
            locPlugins[i]->myfunc; \
    }  \
    } while(0)



// ------------------------------------------------------------------------------------
// Plugin-related stuff
// ------------------------------------------------------------------------------------


// Invoked by a thread, gives life to the object

void UgrConnector::tick(int parm) {

    const char *fname = "UgrConnector::tick";
    Info(SimpleDebug::kLOW, fname, "Ticker started");

    //ticker->detach();

    while (!ticker->interruption_requested()) {
        Info(SimpleDebug::kHIGHEST, fname, "Tick.");
        time_t timenow = time(0);

        sleep(ticktime);

        // Tick the location handler, caches, etc
        locHandler.tick();

        // Tick the plugins
        for (unsigned int i = 0; i < locPlugins.size(); i++) {
            locPlugins[i]->Tick(timenow);
        }
    }

    Info(SimpleDebug::kLOW, fname, "Ticker exiting");
}

UgrConnector::~UgrConnector() {
    const char *fname = "UgrConnector::~UgrConnector";

    Info(SimpleDebug::kLOW, fname, "Stopping ticker.");
    if (ticker) {
        Info(SimpleDebug::kLOW, fname, "Joining ticker");
        ticker->interrupt();
        ticker->join();
        delete ticker;
        ticker = 0;
        Info(SimpleDebug::kLOW, fname, "Joined.");
    }

    Info(SimpleDebug::kLOW, fname, "Destroying plugins");
    int cnt = locPlugins.size();

    for (int i = 0; i < cnt; i++)
        locPlugins[i]->stop();

    for (int i = 0; i < cnt; i++)
        delete locPlugins[i];

    Info(SimpleDebug::kLOW, fname, "Exiting.");

}

int UgrConnector::init(char *cfgfile) {
    const char *fname = "UgrConnector::init";
    {
        boost::lock_guard<boost::mutex> l(mtx);
        if (initdone) return -1;

        // Process the config file
        Info(SimpleDebug::kLOW, "MsgProd_Init_cfgfile", "Starting. Config: " << cfgfile);

        if (!cfgfile || !strlen(cfgfile)) {
            Error(fname, "No config file given." << cfgfile << endl);
            return 1;
        }

        if (CFG->ProcessFile(cfgfile)) {
            Error(fname, "Error processing config file." << cfgfile << endl;);
            return 1;
        }

        DebugSetLevel(CFG->GetLong("glb.debug", 1));
        bool debug_stderr = CFG->GetBool("glb.log_stderr", true);
        long debuglevel = CFG->GetLong("glb.debug", 1);

        DebugSetLevel(debuglevel);
        SimpleDebug::Instance()->SetStderrPrint(debug_stderr);

        // setup plugin directory
        plugin_dir = CFG->GetString("glb.plugin_dir", (char *) UGR_PLUGIN_DIR_DEFAULT);
        try {
            if (is_directory(plugin_dir)) {
                Info(SimpleDebug::kMEDIUM, fname, "Define Ugr plugin directory to: " << plugin_dir);
            } else {
                throw filesystem_error("ugr plugin path is not a directory ", plugin_dir, error_code(ENOTDIR, get_generic_category()));
            }
        } catch (filesystem_error & e) {
            Error(fname, "Invalid plugin directory" << plugin_dir << ", error " << e.what());
        }

        // Get the tick pace from the config
        ticktime = CFG->GetLong("glb.tick", 10);

        // Load a GeoPlugin, if specified
        string s = CFG->GetString("glb.geoplugin", (char *) "");
        geoPlugin = 0;
        if (s != "") {
            vector<string> parms = tokenize(s, " ");

            path plugin_path(parms[0].c_str()); // if not abs path -> load from plugin dir
            if (!plugin_path.has_root_directory()) {
                plugin_path = plugin_dir;
                plugin_path /= parms[0];
            }

            Info(SimpleDebug::kLOW, fname, "Attempting to load the global Geo plugin " << plugin_path.string());
            geoPlugin = (GeoPlugin *) GetGeoPluginClass((char *) plugin_path.string().c_str(),
                    SimpleDebug::Instance(),
                    Config::GetInstance(),
                    parms);

            if (!geoPlugin) {
                Error(fname, "Error loading Geo plugin " << s << endl;);
                return 1;
            }


        }



        // Init the extcache, as now we have the cfg parameters
        extCache.Init();
        this->locHandler.Init(&extCache);




        // Cycle through the location plugins that have to be loaded
        char buf[1024];
        int i = 0;
        locPlugins.clear();
        do {
            CFG->ArrayGetString("glb.locplugin", buf, i);
            if (buf[0]) {
                vector<string> parms = tokenize(buf, " ");
                // Get the entry point for the plugin that implements the product-oriented technicalities of the calls
                // An empty string does not load any plugin, just keeps the default behavior
                path plugin_path(parms[0].c_str()); // if not abs path -> load from plugin dir
                if (!plugin_path.has_root_directory()) {
                    plugin_path = plugin_dir;
                    plugin_path /= parms[0];
                }
                Info(SimpleDebug::kLOW, fname, "Attempting to load location plugin " << plugin_path.string());
                LocationPlugin *prod = (LocationPlugin *) GetLocationPluginClass((char *) plugin_path.string().c_str(),
                        SimpleDebug::Instance(),
                        Config::GetInstance(),
                        parms);
                if (prod) {
                    prod->setUgrCallback(this);
                    prod->setGeoPlugin(geoPlugin);
                    prod->setID(locPlugins.size());
                    locPlugins.push_back(prod);

                }
            }
            i++;
        } while (buf[0]);

        Info(SimpleDebug::kLOW, fname, "Loaded " << locPlugins.size() << " location plugins." << cfgfile);

        if (!locPlugins.size()) {
            vector<string> parms;

            parms.push_back("static_locplugin");
            parms.push_back("Unnamed");
            parms.push_back("1");

            Info(SimpleDebug::kLOW, fname, "No location plugins available. Using the default one.");
            LocationPlugin *prod = new LocationPlugin(SimpleDebug::Instance(), CFG, parms);
            if (prod) locPlugins.push_back(prod);
        }

        if (!locPlugins.size())
            Info(SimpleDebug::kLOW, fname, "Still no location plugins available. A disaster.");


        n2n_pfx = CFG->GetString("glb.n2n_pfx", (char *) "");
        n2n_newpfx = CFG->GetString("glb.n2n_newpfx", (char *) "");
        UgrFileInfo::trimpath(n2n_pfx);
        UgrFileInfo::trimpath(n2n_newpfx);
        Info(SimpleDebug::kLOW, fname, "N2N pfx: '" << n2n_pfx << "' newpfx: '" << n2n_newpfx << "'");


        Info(SimpleDebug::kHIGH, fname, "Starting the plugins.");
        for (unsigned int i = 0; i < locPlugins.size(); i++) {
            if (locPlugins[i]->start(&extCache))
                Error(fname, "Could not start plugin " << i);
        }

        Info(SimpleDebug::kLOW, fname, locPlugins.size() << " plugins started.");


        // Start the ticker
        ticker = new boost::thread(boost::bind(&UgrConnector::tick, this, 0));


        initdone = true;

    }

    Info(SimpleDebug::kLOW, fname, "Initialization complete.");

    return 0;
}

bool UgrConnector::isEndpointOK(int pluginID) {
    if ((pluginID > (int) locPlugins.size()) || (pluginID < 0)) return false;

    return locPlugins[pluginID]->isOK();
}

void UgrConnector::do_n2n(std::string &path) {
    if ((n2n_pfx.size() == 0) || (path.find(n2n_pfx) == 0)) {

        if ((n2n_newpfx.size() > 0) || (n2n_pfx.size() > 0)) {

            path = n2n_newpfx + path.substr(n2n_pfx.size());

            // Avoid double slashes at the beginning. This is well spent CPU time, even if it may hide a bad configuration.
            if (path.substr(0, 2) == "//")
                path.erase(0, 1);

        }
    }

}

int UgrConnector::do_Stat(UgrFileInfo *fi) {

    // Ask all the non slave plugins that are online
    for (unsigned int i = 0; i < locPlugins.size(); i++) {
        if ( (!locPlugins[i]->isSlave()) && (locPlugins[i]->isOK())) locPlugins[i]->do_Stat(fi, &locHandler);
    }

    return 0;
}

int UgrConnector::do_waitStat(UgrFileInfo *fi, int tmout) {

    if (fi->getStatStatus() != UgrFileInfo::InProgress) return 0;

    Info(SimpleDebug::kHIGH, "UgrConnector::do_waitStat", "Going to wait for " << fi->name);
    {
        unique_lock<mutex> lck(*fi);

        // If still pending, we wait for the file object to get a notification
        // then we recheck...
        return fi->waitStat(lck, tmout);
    }

    return 0;
}

int UgrConnector::stat(string &lfn, UgrFileInfo **nfo) {
    const char *fname = "UgrConnector::stat";

    UgrFileInfo::trimpath(lfn);
    do_n2n(lfn);

    Info(SimpleDebug::kMEDIUM, fname, "Stating " << lfn);

    // See if the info is in cache
    // If not in memory create an object and trigger a search on it
    UgrFileInfo *fi = locHandler.getFileInfoOrCreateNewOne(lfn);
    {
        boost::lock_guard<UgrFileInfo > l(*fi);
        if (fi->getStatStatus() == UgrFileInfo::NoInfo)
            do_Stat(fi);
    }

    // wait for the search to finish by looking at the pending object
    do_waitStat(fi, CFG->GetLong("glb.waittimeout", 30));

    // If the status is noinfo, we can mark it as not found
    {
        boost::lock_guard<UgrFileInfo > l(*fi);
        if (fi->getStatStatus() == UgrFileInfo::NoInfo)
            fi->status_statinfo = UgrFileInfo::NotFound;
        else fi->status_statinfo = UgrFileInfo::Ok;
    }

    *nfo = fi;

    // Touch the item anyway, it has been referenced
    fi->touch();

    // Send, if needed, to the external cache
    this->locHandler.putFileInfoToCache(fi);

    Info(SimpleDebug::kMEDIUM, fname, "Stat-ed " << lfn << " sz:" << fi->size << " fl:" << fi->unixflags << " Status: " << fi->getStatStatus() <<
            " status_statinfo: " << fi->status_statinfo << " pending_statinfo: " << fi->pending_statinfo);
    return 0;
}

void UgrConnector::statSubdirs(UgrFileInfo *fi) {
    const char *fname = "UgrConnector::statSubdirs";

    boost::lock_guard<UgrFileInfo > l(*fi);

    // if it's not a dir then exit
    if (!(fi->unixflags & S_IFDIR)) {
        Info(SimpleDebug::kMEDIUM, fname, "Try to sub-stat a file that is not a directory !! " << fi->name);
        return;
    }

    Info(SimpleDebug::kMEDIUM, fname, "Stat-ing all the subdirs of " << fi->name);

    // Cycle through all the subdirs (fi is locked)
    for (std::set<UgrFileItem>::iterator i = fi->subdirs.begin();
            i != fi->subdirs.end();
            i++) {

        std::string cname = fi->name;
        cname += "/";
        cname += i->name;

        UgrFileInfo *fi2 = locHandler.getFileInfoOrCreateNewOne(cname);
        {
            boost::lock_guard<UgrFileInfo > l(*fi2);
            if (fi2->getStatStatus() == UgrFileInfo::NoInfo)
                do_Stat(fi2);
        }

    }


}

int UgrConnector::do_Locate(UgrFileInfo *fi) {

    // Ask all the non slave plugins that are online
    for (unsigned int i = 0; i < locPlugins.size(); i++) {
        if ( (!locPlugins[i]->isSlave()) && (locPlugins[i]->isOK()) ) locPlugins[i]->do_Locate(fi, &locHandler);
    }

    return 0;
}

int UgrConnector::do_waitLocate(UgrFileInfo *fi, int tmout) {

    if (fi->getLocationStatus() != UgrFileInfo::InProgress) return 0;

    Info(SimpleDebug::kHIGH, "UgrConnector::do_waitLocate", "Going to wait for " << fi->name);
    {
        unique_lock<mutex> lck(*fi);

        // If still pending, we wait for the file object to get a notification
        // then we recheck...
        return fi->waitLocations(lck, tmout);
    }

    return 0;
}





int UgrConnector::do_checkreplica(UgrFileInfo *fi, std::string rep) {

    if (!fi) return -1;
    
    // Propagate the request to the slave checker plugins. This is an async operation, as we
    // suppose that there already is a thread that is waiting for this result
    
    // Ask all the slave plugins that are online
    for (unsigned int i = 0; i < locPlugins.size(); i++) {
        if ( (locPlugins[i]->isSlave()) && (locPlugins[i]->isOK()) ) locPlugins[i]->do_CheckReplica(fi, rep, &locHandler);
    }

    // Touch the item anyway, it has been referenced
    fi->touch();

    Info(SimpleDebug::kLOW, "UgrConnector::do_checkreplica", "Checking " << fi->name << "rep:" << rep << " Status: " << fi->getLocationStatus() <<
            " status_locations: " << fi->status_locations << " pending_locations: " << fi->pending_locations);

    return 0;
}



int UgrConnector::locate(string &lfn, UgrFileInfo **nfo) {

    UgrFileInfo::trimpath(lfn);
    do_n2n(lfn);

    Info(SimpleDebug::kMEDIUM, "UgrConnector::locate", "Locating " << lfn);

    // See if the info is in cache
    // If not in memory create an object and trigger a search on it
    UgrFileInfo *fi = locHandler.getFileInfoOrCreateNewOne(lfn, true, true);

    {
        boost::lock_guard<UgrFileInfo > l(*fi);
        if (fi->getLocationStatus() == UgrFileInfo::NoInfo)
            do_Locate(fi);
    }

    // wait for the search to finish by looking at the pending object
    do_waitLocate(fi, CFG->GetLong("glb.waittimeout", 30));


    // If the status is noinfo, we can mark it as not found
    {
        boost::lock_guard<UgrFileInfo > l(*fi);
        if (fi->getLocationStatus() == UgrFileInfo::NoInfo)
            fi->status_locations = UgrFileInfo::NotFound;
        else fi->status_locations = UgrFileInfo::Ok;
    }

    *nfo = fi;

    // Touch the item anyway, it has been referenced
    fi->touch();

    // Send, if needed, to the external cache
    this->locHandler.putSubitemsToCache(fi);

    Info(SimpleDebug::kLOW, "UgrConnector::locate", "Located " << lfn << "repls:" << fi->replicas.size() << " Status: " << fi->getLocationStatus() <<
            " status_locations: " << fi->status_locations << " pending_locations: " << fi->pending_locations);

    return 0;
}

int UgrConnector::do_List(UgrFileInfo *fi) {


    // Ask all the non slave plugins that are online
    for (unsigned int i = 0; i < locPlugins.size(); i++) {
        if ( (!locPlugins[i]->isSlave()) && (locPlugins[i]->isOK()) ) locPlugins[i]->do_List(fi, &locHandler);
    }


    return 0;
}

int UgrConnector::do_waitList(UgrFileInfo *fi, int tmout) {

    if (fi->getItemsStatus() != UgrFileInfo::InProgress) return 0;

    Info(SimpleDebug::kHIGH, "UgrConnector::do_waitList", "Going to wait for " << fi->name);
    {
        unique_lock<mutex> lck(*fi);

        // If still pending, we wait for the file object to get a notification
        // then we recheck...
        return fi->waitItems(lck, tmout);
    }


    return 0;
}

int UgrConnector::list(string &lfn, UgrFileInfo **nfo, int nitemswait) {

    UgrFileInfo::trimpath(lfn);
    do_n2n(lfn);

    Info(SimpleDebug::kMEDIUM, "UgrConnector::list", "Listing " << lfn);

    // See if the info is in cache
    // If not in memory create an object and trigger a search on it
    UgrFileInfo *fi = locHandler.getFileInfoOrCreateNewOne(lfn, true, true);

    {
        boost::lock_guard<UgrFileInfo > l(*fi);
        if (fi->getItemsStatus() == UgrFileInfo::NoInfo)
            do_List(fi);
    }

    // wait for the search to finish by looking at the pending object
    do_waitList(fi, CFG->GetLong("glb.waittimeout", 30));



    // If the status is noinfo, we can mark it as not found
    {
        boost::lock_guard<UgrFileInfo > l(*fi);
        if (fi->getItemsStatus() == UgrFileInfo::NoInfo)
            fi->status_items = UgrFileInfo::NotFound;
        else
            if (fi->status_items != UgrFileInfo::Error)
            fi->status_items = UgrFileInfo::Ok;
    }

    // Stat all the childs in parallel, eventually
    if (CFG->GetBool("glb.statsubdirs", false))
        statSubdirs(fi);

    *nfo = fi;

    // Touch the item anyway, it has been referenced
    fi->touch();

    // Send, if needed, to the external cache
    this->locHandler.putSubitemsToCache(fi);

    Info(SimpleDebug::kLOW, "UgrConnector::list", "Listed " << lfn << "items:" << fi->subdirs.size() << " Status: " << fi->getItemsStatus() <<
            " status_items: " << fi->status_items << " pending_items: " << fi->pending_items);

    return 0;
};

std::set<UgrFileItem_replica, UgrFileItemGeoComp> UgrConnector::getGeoSortedReplicas(std::string clientip, UgrFileInfo *nfo) {


    float ltt = 0.0, lng = 0.0;

    if (geoPlugin) geoPlugin->getAddrLocation(clientip, ltt, lng);

    UgrFileItemGeoComp cmp(ltt, lng);
    Info(SimpleDebug::kLOW, "UgrConnector::getGeoSortedReplicas", nfo->name << " " << clientip << " " << ltt << " " << lng);
    std::set<UgrFileItem_replica, UgrFileItemGeoComp> newset(cmp);

    if (nfo) {
        for (std::set<UgrFileItem_replica>::iterator i = nfo->replicas.begin(); i != nfo->replicas.end(); ++i)
            newset.insert(*i);
    }

    return newset;


}

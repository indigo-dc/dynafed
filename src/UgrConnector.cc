/** @file   UgrConnector.cc
 * @brief  Base class that gives the functionalities of a dynamic, protocol-agnostic redirector
 * @author Fabrizio Furano
 * @date   Jul 2011
 */


#include <iostream>
#include "SimpleDebug.hh"
#include "PluginLoader.hh"
#include <string>
#include "UgrConnector.hh"
#include "LocationInfo.hh"
#include "LocationInfoHandler.hh"

#include <sys/types.h>
#include <sys/stat.h>

using namespace std;


/// Clean up a path

void trimpath(std::string &s) {


    if (*(s.rbegin()) == '/')
        s.erase(s.size() - 1);


}

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
        sleep(ticktime);
        locHandler.tick();
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
    if (initdone) return -1;

    const char *fname = "UgrConnector::init";
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
    long debuglevel = CFG->GetLong("glb.debug", 1);

    DebugSetLevel(debuglevel);

    // Get the tick pace from the config
    ticktime = CFG->GetLong("glb.tick", 10);


    // Load a GeoPlugin, if specified
    string s = CFG->GetString("glb.geoplugin", (char *) "");
    geoPlugin = 0;
    if (s != "") {
        vector<string> parms = tokenize(s, " ");

        Info(SimpleDebug::kLOW, fname, "Attempting to load the global Geo plugin " << s);
        geoPlugin = (GeoPlugin *) GetGeoPluginClass((char *) parms[0].c_str(),
                SimpleDebug::Instance(),
                Config::GetInstance(),
                parms);

        if (!geoPlugin) {
            Error(fname, "Error loading Geo plugin " << s << endl;);
            return 1;
        }


    }

    // Cycle through the location plugins that have to be loaded
    char buf[1024];
    int i = 0;

    do {
        CFG->ArrayGetString("glb.locplugin", buf, i);
        if (buf[0]) {
            vector<string> parms = tokenize(buf, " ");
            // Get the entry point for the plugin that implements the product-oriented technicalities of the calls
            // An empty string does not load any plugin, just keeps the default behavior
            Info(SimpleDebug::kLOW, fname, "Attempting to load location plugin " << buf);
            LocationPlugin *prod = (LocationPlugin *) GetLocationPluginClass((char *) parms[0].c_str(),
                    SimpleDebug::Instance(),
                    Config::GetInstance(),
                    parms);
            if (prod) {
                prod->setGeoPlugin(geoPlugin);
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


    ticker = new boost::thread(boost::bind(&UgrConnector::tick, this, 0));

    Info(SimpleDebug::kHIGH, fname, "Starting the plugins.");
    for (unsigned int i = 0; i < locPlugins.size(); i++) {
        if (locPlugins[i]->start())
            Error(fname, "Could not start plugin " << i);
    }
    Info(SimpleDebug::kLOW, fname, locPlugins.size() << " plugins started.");


    n2n_pfx = CFG->GetString("glb.n2n_pfx", (char *) "");
    n2n_newpfx = CFG->GetString("glb.n2n_newpfx", (char *) "");
    trimpath(n2n_pfx);
    trimpath(n2n_newpfx);
    Info(SimpleDebug::kLOW, fname, "N2N pfx: '" << n2n_pfx << "' newpfx: '" << n2n_newpfx << "'");

    // Init the extcache
    this->locHandler.Init();
    
    initdone = true;
    return 0;
}

void UgrConnector::do_n2n(std::string &path) {
    if ((n2n_pfx.size() == 0) || (path.find(n2n_pfx) == 0)) {

        if ((n2n_newpfx.size() > 0) || (n2n_pfx.size() > 0))
            path = n2n_newpfx + path.substr(n2n_pfx.size());

    }
}

int UgrConnector::do_Stat(UgrFileInfo *fi) {

    for (unsigned int i = 0; i < locPlugins.size(); i++)
        locPlugins[i]->do_Stat(fi, &locHandler);

    return 0;
}

int UgrConnector::do_waitStat(UgrFileInfo *fi, int tmout) {

    if (fi->getStatStatus() != UgrFileInfo::InProgress) return 0;

    for (unsigned int i = 0; i < locPlugins.size(); i++)
        locPlugins[i]->do_waitStat(fi, tmout);

    return 0;
}

int UgrConnector::stat(string &lfn, UgrFileInfo **nfo) {
    const char *fname = "UgrConnector::stat";

    trimpath(lfn);
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
    do_waitStat(fi);

    // If the status is noinfo, we can mark it as not found
    {
        boost::lock_guard<UgrFileInfo > l(*fi);
        if (fi->getStatStatus() == UgrFileInfo::NoInfo)
            fi->status_statinfo = UgrFileInfo::NotFound;
        else fi->status_statinfo = UgrFileInfo::Ok;
    }

    *nfo = fi;

    // Send, if needed, to the external cache
    this->locHandler.putFileInfoToCache(fi);

    Info(SimpleDebug::kLOW, fname, "Stat-ed " << lfn << "sz:" << fi->size << " fl:" << fi->unixflags << " Status: " << fi->getStatStatus() <<
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

    Info(SimpleDebug::kMEDIUM, fname, "Stat-ing all the subitems of " << fi->name);

    // Cycle through all the subdirs (fi is locked)
    for (std::set<UgrFileItem>::iterator i = fi->subitems.begin();
            i != fi->subitems.end();
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



    for (unsigned int i = 0; i < locPlugins.size(); i++)
        locPlugins[i]->do_Locate(fi, &locHandler);

    return 0;
}

int UgrConnector::do_waitLocate(UgrFileInfo *fi, int tmout) {

    if (fi->getLocationStatus() != UgrFileInfo::InProgress) return 0;

    for (unsigned int i = 0; i < locPlugins.size(); i++)
        locPlugins[i]->do_waitLocate(fi, tmout);

    return 0;
}

int UgrConnector::locate(string &lfn, UgrFileInfo **nfo) {

    trimpath(lfn);
    do_n2n(lfn);

    Info(SimpleDebug::kMEDIUM, "UgrConnector::locate", "Locating " << lfn);

    // See if the info is in cache
    // If not in memory create an object and trigger a search on it
    UgrFileInfo *fi = locHandler.getFileInfoOrCreateNewOne(lfn);

    {
        boost::lock_guard<UgrFileInfo > l(*fi);
        if (fi->getLocationStatus() == UgrFileInfo::NoInfo)
            do_Locate(fi);
    }

    // wait for the search to finish by looking at the pending object
    do_waitLocate(fi);


    // If the status is noinfo, we can mark it as not found
    {
        boost::lock_guard<UgrFileInfo > l(*fi);
        if (fi->getLocationStatus() == UgrFileInfo::NoInfo)
            fi->status_locations = UgrFileInfo::NotFound;
        else fi->status_locations = UgrFileInfo::Ok;
    }

    *nfo = fi;

    // Send, if needed, to the external cache
    this->locHandler.putSubitemsToCache(fi);

    Info(SimpleDebug::kLOW, "UgrConnector::locate", "Located " << lfn << "repls:" << fi->subitems.size() << " Status: " << fi->getLocationStatus() <<
            " status_statinfo: " << fi->status_locations << " pending_statinfo: " << fi->pending_locations);

    return 0;
}

int UgrConnector::do_List(UgrFileInfo *fi) {
    for (unsigned int i = 0; i < locPlugins.size(); i++)
        locPlugins[i]->do_List(fi, &locHandler);

    return 0;
}

int UgrConnector::do_waitList(UgrFileInfo *fi, int tmout) {

    if (fi->getItemsStatus() != UgrFileInfo::InProgress) return 0;

    for (unsigned int i = 0; i < locPlugins.size(); i++)
        locPlugins[i]->do_waitList(fi, tmout);

    return 0;
}

int UgrConnector::list(string &lfn, UgrFileInfo **nfo, int nitemswait) {

    trimpath(lfn);
    do_n2n(lfn);

    Info(SimpleDebug::kMEDIUM, "UgrConnector::list", "Listing " << lfn);

    // See if the info is in cache
    // If not in memory create an object and trigger a search on it
    UgrFileInfo *fi = locHandler.getFileInfoOrCreateNewOne(lfn);

    {
        boost::lock_guard<UgrFileInfo > l(*fi);
        if (fi->getItemsStatus() == UgrFileInfo::NoInfo)
            do_List(fi);
    }

    // wait for the search to finish by looking at the pending object
    do_waitList(fi);



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

    // Send, if needed, to the external cache
    this->locHandler.putSubitemsToCache(fi);

    Info(SimpleDebug::kLOW, "UgrConnector::list", "Listed " << lfn << "items:" << fi->subitems.size() << " Status: " << fi->getItemsStatus() <<
            " status_items: " << fi->status_items << " pending_items: " << fi->pending_items);

    return 0;
};

std::set<UgrFileItem, UgrFileItemGeoComp> UgrConnector::getGeoSortedReplicas(std::string clientip, UgrFileInfo *nfo) {
    

        float ltt = 0.0, lng = 0.0;

        if (geoPlugin) geoPlugin->getAddrLocation(clientip, ltt, lng);

        UgrFileItemGeoComp cmp(ltt, lng);
        Info(SimpleDebug::kLOW, "UgrConnector::getGeoSortedReplicas", nfo->name << " " << clientip << " " << ltt << " " << lng);
        std::set<UgrFileItem, UgrFileItemGeoComp> newset(cmp);

        if (nfo) {
            for (std::set<UgrFileItem>::iterator i = nfo->subitems.begin(); i != nfo->subitems.end(); ++i)
                newset.insert((UgrFileItem &)(*i));
        }

        return newset;

    
}

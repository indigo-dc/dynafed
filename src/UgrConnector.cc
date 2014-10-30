/** @file   UgrConnector.cc
 * @brief  Base class that gives the functionalities of a dynamic, protocol-agnostic redirector
 * @author Fabrizio Furano
 * @date   Jul 2011
 */


#include <iostream>
#include <typeinfo>
#include <sys/types.h>
#include <sys/stat.h>

#include "SimpleDebug.hh"
#include "PluginLoader.hh"
#include <string>
#include "UgrConnector.hh"
#include "LocationInfo.hh"
#include "LocationInfoHandler.hh"



using namespace boost;
using namespace boost::filesystem;
using namespace boost::system;


bool replicas_is_offline(UgrConnector * c,  const UgrFileItem_replica & r);

// mocking object
std::function<bool (UgrConnector*, const UgrFileItem_replica&)> replicasStatusObj(replicas_is_offline);


// ------------------------------------------------------------------------------------
// Plugin-related stuff
// ------------------------------------------------------------------------------------

template<class T>
void ugr_load_plugin(UgrConnector & c,
                       const std::string & fname,
                       const boost::filesystem::path & plugin_dir,
                       const std::string & key_list_config,
                       std::vector<T*> & v_plugin){
    char buf[1024];
    int i = 0;
    v_plugin.clear();
    do {
        CFG->ArrayGetString(key_list_config.c_str(), buf, i);
        if (buf[0]) {
            T* filter = NULL;
            std::vector<std::string> parms = tokenize(buf, " ");
           path plugin_path(parms[0].c_str()); // if not abs path -> load from plugin dir
           // Get the entry point for the plugin that implements the product-oriented technicalities of the calls
           // An empty string does not load any plugin, just keeps the default behavior

           if (!plugin_path.has_root_directory()) {
               plugin_path = plugin_dir;
               plugin_path /= parms[0];
           }
            try{

                Info(UgrLogger::Lvl1, fname, "Attempting to load plugin "<< typeid(T).name() << plugin_path.string());
                PluginInterface *prod = static_cast<PluginInterface*>(GetPluginInterfaceClass((char *) plugin_path.string().c_str(),
                        c,
                        parms));
                filter = dynamic_cast<T*>(prod);
            }catch(...){
                // silent exception
                filter = NULL;
            }
            if (filter) {
                filter->setID(v_plugin.size());
                v_plugin.push_back(filter);
            }else{
                Error(fname, "Impossible to load plugin " << plugin_path << " of type " << typeid(T).name() << std::endl;);
            }
        }
        i++;
    } while (buf[0]);
}


template<class T>
void ugr_unload_plugin(std::vector<T*> & v_plugin){
    for(typename std::vector<T*>::iterator it = v_plugin.begin(); it != v_plugin.end(); ++it){
        delete *it;
    }
}


// Invoked by a thread, gives life to the object

void UgrConnector::tick(int parm) {

    const char *fname = "UgrConnector::tick";
    Info(UgrLogger::Lvl1, fname, "Ticker started");

    //ticker->detach();

    while (!ticker->interruption_requested()) {
        Info(UgrLogger::Lvl4, fname, "Tick.");
        time_t timenow = time(0);

        sleep(ticktime);

        // Tick the location handler, caches, etc
        locHandler.tick();

        // Tick the plugins
        for (unsigned int i = 0; i < locPlugins.size(); i++) {
            locPlugins[i]->Tick(timenow);
        }
    }

    Info(UgrLogger::Lvl1, fname, "Ticker exiting");
}

UgrConnector::~UgrConnector() {
    const char *fname = "UgrConnector::~UgrConnector";

    Info(UgrLogger::Lvl1, fname, "Stopping ticker.");
    if (ticker) {
        Info(UgrLogger::Lvl1, fname, "Joining ticker");
        ticker->interrupt();
        ticker->join();
        delete ticker;
        ticker = 0;
        Info(UgrLogger::Lvl1, fname, "Joined.");
    }

    Info(UgrLogger::Lvl1, fname, "Destroying plugins");
    int cnt = locPlugins.size();

    for (int i = 0; i < cnt; i++)
        locPlugins[i]->stop();

    ugr_unload_plugin<LocationPlugin>(locPlugins);
    ugr_unload_plugin<FilterPlugin>(filterPlugins);

    Info(UgrLogger::Lvl1, fname, "Exiting.");

}

Config & UgrConnector::getConfig() const{
    return *Config::GetInstance();
}

UgrLogger & UgrConnector::getLogger() const{
    return *UgrLogger::get();
}

int UgrConnector::init(char *cfgfile) {
    const char *fname = "UgrConnector::init";
    {
        boost::lock_guard<boost::mutex> l(mtx);
        if (initdone) return -1;

        // Process the config file
        Info(UgrLogger::Lvl1, "MsgProd_Init_cfgfile", "Starting. Config: " << cfgfile);

        if (!cfgfile || !strlen(cfgfile)) {
            Error(fname, "No config file given." << cfgfile << std::endl);
            return 1;
        }

        if (CFG->ProcessFile(cfgfile)) {
            Error(fname, "Error processing config file." << cfgfile << std::endl;);
            return 1;
        }

        bool debug_stderr = CFG->GetBool("glb.log_stderr", true);
        long debuglevel = CFG->GetLong("glb.debug", 1);
        DebugSetLevel(debuglevel);
        UgrLogger::get()->SetStderrPrint(debug_stderr);
	
	// Now enable the logging of the components that have been explicitely requested
	int i = 0;
	do {
	  char buf[1024];
	  CFG->ArrayGetString("glb.debug.components", buf, i);
	  if (!buf[0]) break;
	  UgrLogger::get()->setLogged(buf, true);
	  ++i;
	} while (1);

        // setup plugin directory
        plugin_dir = CFG->GetString("glb.plugin_dir", (char *) UGR_PLUGIN_DIR_DEFAULT);
        try {
            if (is_directory(plugin_dir)) {
                Info(UgrLogger::Lvl2, fname, "Define Ugr plugin directory to: " << plugin_dir);
            } else {
                throw filesystem_error("ugr plugin path is not a directory ", plugin_dir, error_code(ENOTDIR, get_generic_category()));
            }
        } catch (filesystem_error & e) {
            Error(fname, "Invalid plugin directory" << plugin_dir << ", error " << e.what());
        }

        // Get the tick pace from the config
        ticktime = CFG->GetLong("glb.tick", 10);

        // Init the extcache, as now we have the cfg parameters
        extCache.Init();
        this->locHandler.Init(&extCache);


        ugr_load_plugin<LocationPlugin>(*this, fname, plugin_dir,
                                               "glb.locplugin", locPlugins);

        Info(UgrLogger::Lvl1, fname, "Loaded " << locPlugins.size() << " location plugins." << cfgfile);

        if (!locPlugins.size()) {
            std::vector<std::string> parms;

            parms.push_back("static_locplugin");
            parms.push_back("Unnamed");
            parms.push_back("1");

            Info(UgrLogger::Lvl1, fname, "No location plugins available. Using the default one.");
            LocationPlugin *prod = new LocationPlugin(*this, parms);
            if (prod) locPlugins.push_back(prod);
        }

        if (!locPlugins.size())
            Info(UgrLogger::Lvl1, fname, "Still no location plugins available. A disaster.");

        // load Filter plugins
        ugr_load_plugin<FilterPlugin>(*this, fname, plugin_dir,
                                               "glb.filterplugin", filterPlugins);


        n2n_pfx = CFG->GetString("glb.n2n_pfx", (char *) "");
        n2n_newpfx = CFG->GetString("glb.n2n_newpfx", (char *) "");
        UgrFileInfo::trimpath(n2n_pfx);
        UgrFileInfo::trimpath(n2n_newpfx);
        Info(UgrLogger::Lvl1, fname, "N2N pfx: '" << n2n_pfx << "' newpfx: '" << n2n_newpfx << "'");


        Info(UgrLogger::Lvl3, fname, "Starting the plugins.");
        for (unsigned int i = 0; i < locPlugins.size(); i++) {
            if (locPlugins[i]->start(&extCache))
                Error(fname, "Could not start plugin " << i);
        }

        Info(UgrLogger::Lvl1, fname, locPlugins.size() << " plugins started.");


        // Start the ticker
        ticker = new boost::thread(boost::bind(&UgrConnector::tick, this, 0));


        initdone = true;

    }

    Info(UgrLogger::Lvl1, fname, "Initialization complete.");

    return 0;
}

bool UgrConnector::isEndpointOK(int pluginID) {
    const size_t id = static_cast<size_t>(pluginID);

    if ( id >= locPlugins.size()){
        Info(UgrLogger::Lvl1, "isEndpointOK", "Invalid plugin ID BUG !");
        return false;
    }

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
        if ( (!locPlugins[i]->isSlave()) && (locPlugins[i]->isOK())
             && (locPlugins[i]->getFlag(LocationPlugin::Readable))){
            locPlugins[i]->do_Stat(fi, &locHandler);
        }
    }

    return 0;
}

int UgrConnector::do_waitStat(UgrFileInfo *fi, int tmout) {

    if (fi->getStatStatus() != UgrFileInfo::InProgress) return 0;

    Info(UgrLogger::Lvl3, "UgrConnector::do_waitStat", "Going to wait for " << fi->name);
    {
        unique_lock<mutex> lck(*fi);

        // If still pending, we wait for the file object to get a notification
        // then we recheck...
        return fi->waitStat(lck, tmout);
    }

    return 0;
}

int UgrConnector::stat(std::string &lfn, UgrFileInfo **nfo) {
    const char *fname = "UgrConnector::stat";

    UgrFileInfo::trimpath(lfn);
    do_n2n(lfn);

    Info(UgrLogger::Lvl2, fname, "Stating " << lfn);

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
        if ((fi->getStatStatus() == UgrFileInfo::NoInfo) ||
          (fi->getStatStatus() == UgrFileInfo::InProgress))
            fi->status_statinfo = UgrFileInfo::NotFound;

        // We don't set it to ok if it was in progress after a timeout
        //else fi->status_statinfo = UgrFileInfo::Ok;
    }

    *nfo = fi;

    // Touch the item anyway, it has been referenced
    fi->touch();

    // Send, if needed, to the external cache
    this->locHandler.putFileInfoToCache(fi);

    Info(UgrLogger::Lvl2, fname, "Stat-ed " << lfn << " sz:" << fi->size << " fl:" << fi->unixflags << " Status: " << fi->getStatStatus() <<
            " status_statinfo: " << fi->status_statinfo << " pending_statinfo: " << fi->pending_statinfo);
    return 0;
}


int UgrConnector::findNewLocation(const std::string & new_lfn, const UgrClientInfo & client, UgrReplicaVec & new_locations){
    const char *fname = "UgrConnector::findNewLocation";
    std::string l_lfn(new_lfn);
    std::shared_ptr<NewLoctationHandler> response_handler= std::make_shared<NewLoctationHandler>();

    UgrFileInfo::trimpath(l_lfn);
    do_n2n(l_lfn);

    Info(UgrLogger::Lvl2, fname,  "Find new location for " << l_lfn);

    // Ask all the non slave plugins that are online
    for (auto it = locPlugins.begin(); it < locPlugins.end(); ++it) {
        if ( (!(*it)->isSlave()) && ((*it)->isOK())
             && (*it)->getFlag(LocationPlugin::Writable)){

            response_handler->addWorker(1);
            (*it)->async_findNewLocation(l_lfn, response_handler);
        }
    }


    if(response_handler->wait(CFG->GetLong("glb.waittimeout", 30)) == false){
         Info(UgrLogger::Lvl2, fname, " Timeout triggered during findNewLocation for " << l_lfn);
    }

    new_locations.clear();
    new_locations = response_handler->takeAll();
    Info(UgrLogger::Lvl2, fname, new_locations.size() << " NewLocations found for " << l_lfn);

    // sort all answer geographically
    filter(new_locations, client);

    Info(UgrLogger::Lvl2, fname, new_locations.size() << " new locations founds");
    return 0;

}

bool replicas_is_offline(UgrConnector * c,  const UgrFileItem_replica & r){
    if (c->isEndpointOK(r.pluginID)) {
        Info(UgrLogger::Lvl3, "UgrConnector::filter", "not a replica offline" << r.name << " ");
        return false;
    }
    
    Info(UgrLogger::Lvl3, "UgrConnector::filter", "Skipping offline replica: " << r.name << " " << r.location << " " << r.latitude << " " << r.longitude << " id:" << r.pluginID << " ");
    return true;
}


void filter_offline_replica(UgrConnector & c, UgrReplicaVec & replicas){

    // remove from the list the dead endpoints
    // Filter out the replicas that belong to dead endpoints
    replicas.erase( std::remove_if(replicas.begin(), replicas.end(), boost::bind(replicasStatusObj, &c, _1)),
            replicas.end() );

}

///
/// Apply configured filters on the replica list
int UgrConnector::filter(UgrReplicaVec & replicas){

    
    for(std::vector<FilterPlugin*>::iterator it = filterPlugins.begin(); it != filterPlugins.end(); ++it){
        (*it)->filterReplicaList(replicas);
    }

    filter_offline_replica(*this, replicas);
    
    return 0;
}

int UgrConnector::filter(UgrReplicaVec & replicas, const UgrClientInfo & cli_info){
    // apply generic filters
    filter(replicas);

    // applys all filters with cli_info
    for(std::vector<FilterPlugin*>::iterator it = filterPlugins.begin(); it != filterPlugins.end(); ++it){
        (*it)->filterReplicaList(replicas, cli_info);
    }
    return 0;
}

void UgrConnector::statSubdirs(UgrFileInfo *fi) {
    const char *fname = "UgrConnector::statSubdirs";

    boost::lock_guard<UgrFileInfo > l(*fi);

    // if it's not a dir then exit
    if (!(fi->unixflags & S_IFDIR)) {
        Info(UgrLogger::Lvl2, fname, "Try to sub-stat a file that is not a directory !! " << fi->name);
        return;
    }

    Info(UgrLogger::Lvl2, fname, "Stat-ing all the subdirs of " << fi->name);

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
        if ( (!locPlugins[i]->isSlave()) && (locPlugins[i]->isOK())
             &&  (locPlugins[i]->getFlag(LocationPlugin::Readable))){
            locPlugins[i]->do_Locate(fi, &locHandler);
        }
    }

    return 0;
}

int UgrConnector::do_waitLocate(UgrFileInfo *fi, int tmout) {

    if (fi->getLocationStatus() != UgrFileInfo::InProgress) return 0;

    Info(UgrLogger::Lvl3, "UgrConnector::do_waitLocate", "Going to wait for " << fi->name);
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

    Info(UgrLogger::Lvl1, "UgrConnector::do_checkreplica", "Checking " << fi->name << "rep:" << rep << " Status: " << fi->getLocationStatus() <<
            " status_locations: " << fi->status_locations << " pending_locations: " << fi->pending_locations);

    return 0;
}



int UgrConnector::locate(std::string &lfn, UgrFileInfo **nfo) {

    UgrFileInfo::trimpath(lfn);
    do_n2n(lfn);

    Info(UgrLogger::Lvl2, "UgrConnector::locate", "Locating " << lfn);

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

    Info(UgrLogger::Lvl1, "UgrConnector::locate", "Located " << lfn << " repls:" << fi->replicas.size() << " Status: " << fi->getLocationStatus() <<
            " status_locations: " << fi->status_locations << " pending_locations: " << fi->pending_locations);

    return 0;
}

int UgrConnector::do_List(UgrFileInfo *fi) {


    // Ask all the non slave plugins that are online
    for (unsigned int i = 0; i < locPlugins.size(); i++) {
        if ( (!locPlugins[i]->isSlave()) && (locPlugins[i]->isOK())
             && (locPlugins[i]->getFlag(LocationPlugin::Listable))){
            locPlugins[i]->do_List(fi, &locHandler);
        }
    }


    return 0;
}

int UgrConnector::do_waitList(UgrFileInfo *fi, int tmout) {

    if (fi->getItemsStatus() != UgrFileInfo::InProgress) return 0;

    Info(UgrLogger::Lvl3, "UgrConnector::do_waitList", "Going to wait for " << fi->name);
    {
        unique_lock<mutex> lck(*fi);

        // If still pending, we wait for the file object to get a notification
        // then we recheck...
        return fi->waitItems(lck, tmout);
    }


    return 0;
}

int UgrConnector::list(std::string &lfn, UgrFileInfo **nfo, int nitemswait) {

    UgrFileInfo::trimpath(lfn);
    do_n2n(lfn);

    Info(UgrLogger::Lvl2, "UgrConnector::list", "Listing " << lfn);

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

    Info(UgrLogger::Lvl1, "UgrConnector::list", "Listed " << lfn << "items:" << fi->subdirs.size() << " Status: " << fi->getItemsStatus() <<
            " status_items: " << fi->status_items << " pending_items: " << fi->pending_items);

    return 0;
}

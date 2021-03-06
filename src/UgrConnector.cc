/*
 *  Copyright (c) CERN 2013
 *
 *  Copyright (c) Members of the EMI Collaboration. 2011-2013
 *  See  http://www.eu-emi.eu/partners for details on the copyright
 *  holders.
 *
 *  Licensed under the Apache License, Version 2.0
 *  See the LICENSE file for further information
 * 
 */


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

#include <string>
#include "UgrConnector.hh"
#include "LocationInfo.hh"
#include "LocationInfoHandler.hh"
#include "UgrPluginLoader.hh"
#include "UgrAuthorization.hh"
#include "LocationPlugin.hh"
#include <dlfcn.h>



using namespace boost;
using namespace boost::filesystem;
using namespace boost::system;


bool replicas_is_offline(UgrConnector * c,  const UgrFileItem_replica & r);

// mocking object
std::function<bool (UgrConnector*, const UgrFileItem_replica&)> replicasStatusObj(replicas_is_offline);



// Invoked by a thread, gives life to the object

void UgrConnector::tick(int parm) {

    const char *fname = "UgrConnector::tick";
    Info(UgrLogger::Lvl1, fname, "Ticker started");
    time_t timesummary = 0;

    //ticker->detach();

    while (!ticker->interruption_requested()) {
        Info(UgrLogger::Lvl4, fname, "Tick.");
        time_t timenow = time(0);

        sleep(ticktime);

        // Tick the location handler, caches, etc
        locHandler.tick();

        // Tick the plugins
	int nonline =0;
	int noffline = 0;
	std::string off;
        for (unsigned int i = 0; i < locPlugins.size(); i++) {
            locPlugins[i]->Tick(timenow);
	    if (locPlugins[i]->isOK()) {
	      nonline++;
	    }
	    else {
	      noffline++;
	      off += locPlugins[i]->get_Name();
	      off += ", ";
	    }
        }
        
        // Print a summary of the plugin status
        if (time(0) - timesummary > 60) {
	  Info(UgrLogger::Lvl1, fname, "Plugins status. Online:" << nonline << " Offline:" << noffline);
	  if (noffline > 0) {
	    Info(UgrLogger::Lvl1, fname, " Offline plugins:" << off);
	  }
	  
	  timesummary = time(0);
	}
	
	// Prepare a summary of the plugin statuses, for monitoring purposes
	std::string statuses;
        unsigned int cnt = locPlugins.size();
	for (unsigned int i = 0; i < cnt; i++) {
          statuses.append(locPlugins[i]->get_Name());
          statuses.append("%%");
          statuses.append(locPlugins[i]->get_Name());
          statuses.append("%%");
          locPlugins[i]->appendMonString(statuses);
          if (i < cnt-1) statuses.append("&&");
        }
	Info(UgrLogger::Lvl4, fname, " Plugin mon info:" << statuses);
        
        extCache.putMoninfo(statuses);
    }

    Info(UgrLogger::Lvl1, fname, "Ticker exiting");
}


UgrConnector::UgrConnector(): ticker(0), ticktime(10), initdone(false) {
    const char *fname = "UgrConnector::ctor";
    ugrlogmask = UgrLogger::get()->getMask(ugrlogname);
    Info(UgrLogger::Lvl1, fname, "Ctor " << UGR_VERSION_MAJOR <<"." << UGR_VERSION_MINOR << "." << UGR_VERSION_PATCH);
    Info(UgrLogger::Lvl3, fname, "LibUgrConnector path:" << getUgrLibPath());
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

UgrConfig & UgrConnector::getConfig() const{
    return *UgrConfig::GetInstance();
}

UgrLogger & UgrConnector::getLogger() const{
    return *UgrLogger::get();
}


void UgrConnector::applyHooksNewReplica(UgrFileItem_replica & rep){
    for( auto it = filterPlugins.begin(); it != filterPlugins.end(); ++it){
        (*it)->hookNewReplica(rep);
    }
}

static boost::filesystem::path getPluginDirectory(){
     const char *fname = "UgrConnector::init::getPluginDirectory";
     boost::filesystem::path plugin_dir(getUgrLibPath());
     plugin_dir = plugin_dir.parent_path();
     plugin_dir /= "ugr";

     try {
         if (is_directory(plugin_dir)) {
             Info(UgrLogger::Lvl2, fname, "Define Ugr plugin directory to: " << plugin_dir);
         } else {
             throw filesystem_error("ugr plugin path is not a directory ", plugin_dir, error_code(ENOTDIR, generic_category()));
         }
     } catch (filesystem_error & e) {
         Error(fname, "Invalid plugin directory" << plugin_dir << ", error " << e.what());
     }
    return plugin_dir;
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

        if (UgrCFG->ProcessFile(cfgfile)) {
            Error(fname, "Error processing config file." << cfgfile << std::endl);
            return 1;
        }

        bool debug_stderr = UgrCFG->GetBool("glb.log_stderr", true);
        long debuglevel = UgrCFG->GetLong("glb.debug", 1);
        DebugSetLevel(debuglevel);
        UgrLogger::get()->SetStderrPrint(debug_stderr);
        
        // Now enable the logging of the components that have been explicitely requested
        int i = 0;
        do {
          char buf[1024];
          UgrCFG->ArrayGetString("glb.debug.components", buf, i);
          if (!buf[0]) break;
          UgrLogger::get()->setLogged(buf, true);
          ++i;
        } while (1);
        
        // setup plugin directory
        plugin_dir = getPluginDirectory();

        // Get the tick pace from the config
        ticktime = UgrCFG->GetLong("glb.tick", 10);

        // Mini sanity check on the cache parameters
        if (UgrCFG->GetLong("infohandler.itemttl", 1) > UgrCFG->GetLong("infohandler.itemmaxttl", 1)) {
					Error(fname, "Fatal misconfiguration: infohandler.itemttl (" << UgrCFG->GetLong("infohandler.itemttl", 1) <<
					") should always be smaller than infohandler.itemmaxttl (" << UgrCFG->GetLong("infohandler.itemmaxttl", 1) << ")" << std::endl);
          return 1;
        }
        if (UgrCFG->GetLong("infohandler.itemttl", 1) <= UgrCFG->GetLong("infohandler.itemttl_negative", 1)) {
					Error(fname, "Fatal misconfiguration: infohandler.itemttl_negative (" << UgrCFG->GetLong("infohandler.itemttl_negative", 1) <<
					") should always be smaller than infohandler.itemttl (" << UgrCFG->GetLong("infohandler.itemttl", 1) << ")" << std::endl);
          return 1;
        }
        
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


        // load authorization plugins
        ugr_load_plugin<UgrAuthorizationPlugin>(*this, fname, plugin_dir,
                                               "glb.authorizationplugin", authorizationPlugins);
        
        // An instance of the default authorization plugins is always added to the list, passing no parameters
        {
            std::vector<std::string> pp;
            UgrAuthorizationPlugin *p = new UgrAuthorizationPlugin(*this, pp);
            authorizationPlugins.push_back(p);
        }
        
        
        // Populate vector of prefixes
        size_t p1=0, p2=0;
        std::string pfx_str = UgrCFG->GetString("glb.n2n_pfx", (char *) "");
        // Split on space character and populate vector
        while ( (p2=pfx_str.find_first_of(" ", p1)) != std::string::npos ) {
          n2n_pfx_v.push_back(pfx_str.substr(p1, p2-p1));
          UgrFileInfo::trimpath(n2n_pfx_v.back());
          p1=p2+1;
        }   
        n2n_pfx_v.push_back(pfx_str.substr(p1));
        UgrFileInfo::trimpath(n2n_pfx_v.back());
        // Must have the longest paths first
        sort(n2n_pfx_v.begin(), n2n_pfx_v.end(), std::greater<std::string>());
        
        n2n_newpfx = UgrCFG->GetString("glb.n2n_newpfx", (char *) "");
        UgrFileInfo::trimpath(n2n_newpfx);
        Info(UgrLogger::Lvl1, fname, "N2N prefixes: '" << pfx_str << "' newpfx: '" << n2n_newpfx << "'");


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


bool UgrConnector::canEndpointDoChecksum(int pluginID) {
  const size_t id = static_cast<size_t>(pluginID);
  
  if ( id >= locPlugins.size()){
    Info(UgrLogger::Lvl1, "canEndpointDoChecksum", "Invalid plugin ID BUG !");
    return false;
  }
  
  
  return locPlugins[pluginID]->canDoChecksum();
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
  for (auto p = n2n_pfx_v.begin(); p != n2n_pfx_v.end(); ++p) {
    
    if ((p->size() == 0) || (path.find(*p) == 0)) {
      
      if ((n2n_newpfx.size() > 0) || (p->size() > 0)) {
        
        path = n2n_newpfx + path.substr(p->size());
        
        // Avoid double slashes at the beginning. This is well spent CPU time, even if it may hide a bad configuration.
        if (path.substr(0, 2) == "//")
          path.erase(0, 1);
        
        break;
      }
    }
  }
  
  // Make sure that no spurious queries enter Ugr
  int pos = path.find('&');
  if (pos != std::string::npos)
    path.erase(pos, std::string::npos);
  
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

int UgrConnector::stat(std::string &lfn, const UgrClientInfo &client, UgrFileInfo **nfo) {
    const char *fname = "UgrConnector::stat";
    std::string l_lfn(lfn);
    UgrFileInfo::trimpath(l_lfn);
    do_n2n(l_lfn);

    Info(UgrLogger::Lvl2, fname, "Stating " << l_lfn);

    // See if the info is in cache
    // If not in memory create an object and trigger a search on it
    UgrFileInfo *fi = locHandler.getFileInfoOrCreateNewOne(*this, l_lfn);
    {
        boost::lock_guard<UgrFileInfo > l(*fi);
        if (fi->getStatStatus() == UgrFileInfo::NoInfo)
            do_Stat(fi);
    }

    // wait for the search to finish by looking at the pending object
    do_waitStat(fi, UgrCFG->GetLong("glb.waittimeout", 30));

    bool addtoparent = false;
    
    // If the status is noinfo, we can mark it as not found
    {
        boost::lock_guard<UgrFileInfo > l(*fi);
        if ((fi->getStatStatus() == UgrFileInfo::NoInfo) ||
          (fi->getStatStatus() == UgrFileInfo::InProgress))
            fi->status_statinfo = UgrFileInfo::NotFound;
        
        // stat finished and aquired info, now attempt to update the subdir set of new entry's parent, should increase dynamicity of listing
        else
          addtoparent = true;
          
        // We don't set it to ok if it was in progress after a timeout
        //else fi->status_statinfo = UgrFileInfo::Ok;
        
        // Touch the item anyway, it has been referenced
        fi->touch();
    
    }

    if ( addtoparent && UgrCFG->GetBool("glb.addchildtoparentonstat", true) )
      this->locHandler.addChildToParentSubitem(*this, l_lfn, false, true);

    *nfo = fi;

    // Send, if needed, to the external cache
    this->locHandler.putFileInfoToCache(fi);

    Info(UgrLogger::Lvl2, fname, "Stat-ed '" << l_lfn << "' addr: " << fi << " sz:" << fi->size << " fl:" << fi->unixflags << " Status: " << fi->getStatStatus() <<
            " status_statinfo: " << fi->status_statinfo << " pending_statinfo: " << fi->pending_statinfo);
    return 0;
}


static bool predUnAvailableReplica(const UgrFileItem_replica & rep){
    return (rep.status != UgrFileItem_replica::Available);
}

UgrCode UgrConnector::remove(const std::string &lfn, const UgrClientInfo &client, UgrReplicaVec &replicas_to_delete){
    const char *fname = "UgrConnector::remove";
    std::string l_lfn(lfn);
    std::shared_ptr<DeleteReplicaHandler> response_handler= std::make_shared<DeleteReplicaHandler>();

    UgrFileInfo::trimpath(l_lfn);
    do_n2n(l_lfn);

    Info(UgrLogger::Lvl2, fname,  "Delete all replicas of " << l_lfn);


    // Ask all the non slave plugins that are online and writable
    for (auto it = locPlugins.begin(); it < locPlugins.end(); ++it) {
        if ( (!(*it)->isSlave()) && ((*it)->isOK())
             && (*it)->getFlag(LocationPlugin::Writable)){
            (*it)->async_deleteReplica(l_lfn, response_handler);
        }
    }

    if(response_handler->wait(UgrCFG->GetLong("glb.waittimeout", 30)) == false){
         Info(UgrLogger::Lvl2, fname, "Timeout triggered during deleteAll for " << l_lfn);
    }

    replicas_to_delete = response_handler->takeAll();

    // check if no answer: then no resource has been deleted
    if(replicas_to_delete.size() ==0){
            return UgrCode(UgrCode::FileNotFound, "Resource not existing");
    }


    // check permission if denied, the user has a right problem somewhere, report it
    for(auto it = replicas_to_delete.begin(); it < replicas_to_delete.end(); ++it){
        if((*it).status == UgrFileItem_replica::PermissionDenied){
            return UgrCode(UgrCode::PermissionDenied, "Impossible to suppress the resource, permission denied");
        }
    }

    // remove all replicas that have been that are inconsistents or already deleted
    size_t deleted_number = replicas_to_delete.size();
    replicas_to_delete.erase(std::remove_if(replicas_to_delete.begin(), replicas_to_delete.end(), &predUnAvailableReplica), replicas_to_delete.end());
    deleted_number -= replicas_to_delete.size();

    // apply filters
    filterAndSortReplicaList(replicas_to_delete, client);

    locHandler.wipeInfoOnLfn(*this, l_lfn);
    
    Info(UgrLogger::Lvl2, fname, "Deleted "<< deleted_number << " replicas, " << replicas_to_delete.size() << " to delete");

    return UgrCode();
}


UgrCode UgrConnector::removeDir(const std::string &lfn, const UgrClientInfo &client, UgrReplicaVec &replicas_to_delete){
    const char *fname = "UgrConnector::removeDir";
    std::string l_lfn(lfn);
    std::shared_ptr<DeleteReplicaHandler> response_handler= std::make_shared<DeleteReplicaHandler>();

    UgrFileInfo::trimpath(l_lfn);
    do_n2n(l_lfn);

    Info(UgrLogger::Lvl2, fname,  "Delete all replicas of " << l_lfn);


    // Ask all the non slave plugins that are online and writable
    for (auto it = locPlugins.begin(); it < locPlugins.end(); ++it) {
        if ( (!(*it)->isSlave()) && ((*it)->isOK())
             && (*it)->getFlag(LocationPlugin::Writable)){
            (*it)->async_deleteDir(l_lfn, response_handler);
        }
    }

    if(response_handler->wait(UgrCFG->GetLong("glb.waittimeout", 30)) == false){
         Info(UgrLogger::Lvl2, fname, "Timeout triggered during async_deleteDir for " << l_lfn);
    }

    replicas_to_delete = response_handler->takeAll();

    // check if no answer: then no resource has been deleted
    if(replicas_to_delete.size() ==0){
            return UgrCode(UgrCode::FileNotFound, "Resource does not exist or cannot be found");
    }

    // check permission if denied, the user has a right problem somewhere, report it
    for(auto it = replicas_to_delete.begin(); it < replicas_to_delete.end(); ++it){
        if((*it).status == UgrFileItem_replica::PermissionDenied){
            return UgrCode(UgrCode::PermissionDenied, "Impossible to remove the resource, permission denied");
        }
    }

    // remove all replicas that have been that are inconsistents or already deleted
    size_t deleted_number = replicas_to_delete.size();
    replicas_to_delete.erase(std::remove_if(replicas_to_delete.begin(), replicas_to_delete.end(), &predUnAvailableReplica), replicas_to_delete.end());
    deleted_number -= replicas_to_delete.size();

    // apply filters
    filterAndSortReplicaList(replicas_to_delete, client);
   
    locHandler.wipeInfoOnLfn(*this, l_lfn);
    
    Info(UgrLogger::Lvl2, fname, "Deleted "<< deleted_number << " replicas, " << replicas_to_delete.size() << " to delete");

    return UgrCode();
}








std::vector<std::string> splitPath(const std::string& path) throw()
{
  std::vector<std::string> components;
  size_t s, e;
  std::string comp;
  
  if (!path.empty() && path[0] == '/')
    components.push_back("/");
  
  s = path.find_first_not_of('/');
  while (s != std::string::npos) {
    e = path.find('/', s);
    if (e != std::string::npos) {
      comp = path.substr(s, e - s);
      if (comp.length() > 0)
        components.push_back(comp);
      s = path.find_first_not_of('/', e);
    }
    else {
      comp = path.substr(s);
      if (comp.length() > 0)
        components.push_back(comp);
      s = e;
    }
  }
  
  return components;
}



std::string joinPath(const std::vector<std::string>& components) throw()
{
  std::vector<std::string>::const_iterator i;
  std::string path;
  
  for (i = components.begin(); i != components.end(); ++i) {
    if (*i != "/")
      path += *i + "/";
    else
      path += "/";
    
  }
  
  // Remove the slash at the end
  if (!path.empty())
    path.erase(--path.end());
  
  return path;
}













UgrCode UgrConnector::makeDir(const std::string & lfn, const UgrClientInfo & client) {
  // TODO: ugrconnector::mkdir always succeeds and inserts all the non-existing parent dirs into the cache
  // from LCGDM-2373
  const char *fname = "UgrConnector::makeDir";
  std::string l_lfn(lfn);
  
  UgrFileInfo::trimpath(l_lfn);
  //do_n2n(l_lfn);
  
  Info(UgrLogger::Lvl2, fname, "Make (Fake) all the parent directories for '" << l_lfn << "'");
  
  std::vector<std::string> components = splitPath(l_lfn);

  
  UgrFileItem precitm;
  
  // Make sure that all the parent dirs exist
  while ( components.size() ) {
    UgrFileInfo *nfo = NULL;
    bool dosend = false;
    
    std::string ppath = joinPath(components);
    // Here we can only stat the parent, to guess whether it exists or not
    stat(ppath, client, &nfo);
    {
      boost::lock_guard<UgrFileInfo > l(*nfo);
      
      if (nfo->status_statinfo == UgrFileInfo::NotFound) {
        // No parent means that we have to fake its existence
        Info(UgrLogger::Lvl2, fname, "Can't stat parent: '" << ppath << "' ... we are going to fake its temporary existence");
        
        nfo->unixflags = 0777 | S_IFDIR;
        nfo->size = 0;
        nfo->status_statinfo = UgrFileInfo::Ok;
        nfo->status_items = UgrFileInfo::Ok;
        
        if (precitm.name.size() > 0)
          nfo->subdirs.insert(precitm);
        
        dosend = true;
        
        
        // The current (fake) dir will be a subdir of its parent
        precitm.name = components.back();
        components.pop_back();
      
      }
      else {
        // We met a parent directory that does exist         
        if (precitm.name.size() > 0)
            nfo->subdirs.insert(precitm);
        
        dosend = true;

        break;
      }
    }
    
    if (nfo && dosend) {
      // Send, if needed, to the external cache. This is not really the best thing, no better ideas by now
      this->locHandler.putFileInfoToCache(nfo);
      
      // Send, if needed, to the external cache
      this->locHandler.putSubitemsToCache(nfo);
    }
    
    
    
  }
  
  
  Info(UgrLogger::Lvl3, fname, "Successfully created parent directories for '" << l_lfn << "'");
  return UgrCode();
  
}

UgrCode UgrConnector::findNewLocation(const std::string & new_lfn, off64_t filesz, const UgrClientInfo & client, UgrReplicaVec & new_locations){
    const char *fname = "UgrConnector::findNewLocation";
    std::string l_lfn(new_lfn);
    std::shared_ptr<NewLocationHandler> response_handler= std::make_shared<NewLocationHandler>();

    response_handler->filesize = filesz;
    response_handler->s3uploadID = client.s3uploadid;
    response_handler->nchunks = client.nchunks;
    
    UgrFileInfo::trimpath(l_lfn);
    do_n2n(l_lfn);

    // We need to stat the file to make sure we know nothing about it... sigh
    UgrFileInfo* fi = NULL;
    stat(l_lfn, client, &fi);
        
    // check if ovewrite
    if(UgrCFG->GetBool("glb.allow_overwrite", true) == false){
        
        if(fi && fi->status_items !=  UgrFileInfo::NotFound){
            return UgrCode(UgrCode::OverwriteNotAllowed, "Ovewrite existing resource is not allowed");
        }
    }

    Info(UgrLogger::Lvl2, fname,  "Find new location for " << l_lfn);
    
    // Make sure that the entry we have in the cache will not contain
    // old data, e.g. a NotFound
    fi->setToNoInfo();
    
    // Ask all the non slave plugins that are online
    // Limit the search to one plugin if requested so...
    
    for (auto it = locPlugins.begin(); it < locPlugins.end(); ++it) {
        if ( (!(*it)->isSlave()) && ((*it)->isOK())
             && (*it)->getFlag(LocationPlugin::Writable)) {
          
          // If the client requested a search through a specific plugin...
          if (client.s3uploadpluginid >= 0) {
            Info(UgrLogger::Lvl2, fname,  "Find new location for '" << l_lfn << "' restricting to pluginid " << client.s3uploadpluginid);
            
            if (client.s3uploadpluginid == (*it)->getID())
              (*it)->async_findNewLocation(l_lfn, response_handler);
          }
          else // otherwise do it through all the plugins
            (*it)->async_findNewLocation(l_lfn, response_handler);
          
        }
    }


    if(response_handler->wait(UgrCFG->GetLong("glb.waittimeout", 30)) == false){
         Info(UgrLogger::Lvl2, fname, "Timeout triggered during findNewLocation for " << l_lfn);
    }

    new_locations.clear();
    new_locations = response_handler->takeAll();
    Info(UgrLogger::Lvl2, fname, new_locations.size() << " NewLocations found for " << l_lfn);


    // apply hooks now
    if (client.s3uploadpluginid < 0)
      for(auto it = new_locations.begin(); it < new_locations.end(); ++it){
        applyHooksNewReplica(*it);
      }

    // sort geographically
    if (client.s3uploadpluginid < 0)
      filterAndSortReplicaList(new_locations, client);
    
    // attempt to update the subdir set of new entry's parent, should increase dynamicity of listing
    if ( UgrCFG->GetBool("glb.addchildtoparentonput", true) )
      this->locHandler.addChildToParentSubitem(*this, l_lfn, true, true);
    
    Info(UgrLogger::Lvl2, fname, new_locations.size() << " new locations found");
    return UgrCode();

}

UgrCode UgrConnector::mkDirMinusPonSiteFN(const std::string & sitefn) {
  
  const char *fname = "UgrConnector::mkDirMinusPonSiteFN";
  std::shared_ptr<HandlerTraits> response_handler= std::make_shared<HandlerTraits>();
  
  
  
  // Ask all the non slave plugins that are online and writable
  for (auto it = locPlugins.begin(); it < locPlugins.end(); ++it) {
    if ( (!(*it)->isSlave()) && ((*it)->isOK())
      && (*it)->getFlag(LocationPlugin::Writable)){
      (*it)->async_mkDirMinusPonSiteFN(sitefn, response_handler);
      }
  }
  
  if(response_handler->wait(UgrCFG->GetLong("glb.waittimeout", 30)) == false){
    Error(fname, "Timeout creating remote parent directories for " << sitefn );
  }
  
  
  Info(UgrLogger::Lvl2, fname,  "Exiting. sitefn: '" << sitefn << "'");
  
  return UgrCode();
  
}


bool replicas_is_offline(UgrConnector * c,  const UgrFileItem_replica & r){
    if (c->isEndpointOK(r.pluginID)) {
        Info(UgrLogger::Lvl3, "UgrConnector::replicas_is_offline", "Replica not offline" << r.name << " ");
        return false;
    }
    
    Info(UgrLogger::Lvl3, "UgrConnector::replicas_is_offline", "Offline replica: " << r.name << " " << r.location << " " << r.latitude << " " << r.longitude << " id:" << r.pluginID << " ");
    return true;
}


void filter_offline_replica(UgrConnector & c, UgrReplicaVec & replicas){

    // remove from the list the dead endpoints
    // Filter out the replicas that belong to dead endpoints
    replicas.erase( std::remove_if(replicas.begin(), replicas.end(), boost::bind(replicasStatusObj, &c, _1)),
            replicas.end() );

}


int UgrConnector::filterAndSortReplicaList(UgrReplicaVec & replicas, const UgrClientInfo & cli_info){

    // applys all filters with cli_info
    for(std::vector<FilterPlugin*>::iterator it = filterPlugins.begin(); it != filterPlugins.end(); ++it){
        (*it)->applyFilterOnReplicaList(replicas, cli_info);
    }

    filter_offline_replica(*this, replicas);

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

        UgrFileInfo *fi2 = locHandler.getFileInfoOrCreateNewOne(*this, cname);
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



int UgrConnector::locate(std::string &lfn, const UgrClientInfo &client, UgrFileInfo **nfo) {
  
    UgrFileInfo::trimpath(lfn);
    
    std::string l_lfn(lfn);
    do_n2n(l_lfn);

    Info(UgrLogger::Lvl2, "UgrConnector::locate", "Locating " << l_lfn);

    // See if the info is in cache
    // If not in memory create an object and trigger a search on it
    UgrFileInfo *fi = locHandler.getFileInfoOrCreateNewOne(*this, l_lfn, true, true);

    {
        boost::lock_guard<UgrFileInfo > l(*fi);
        if (fi->getLocationStatus() == UgrFileInfo::NoInfo)
            do_Locate(fi);
    }

    // wait for the search to finish by looking at the pending object
    do_waitLocate(fi, UgrCFG->GetLong("glb.waittimeout", 30));


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

    Info(UgrLogger::Lvl1, "UgrConnector::locate", "Located " << l_lfn << " repls:" << fi->replicas.size() << " Status: " << fi->getLocationStatus() <<
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

int UgrConnector::list(std::string &lfn, const UgrClientInfo &client, UgrFileInfo **nfo, int nitemswait) {

    UgrFileInfo::trimpath(lfn);
    
    std::string l_lfn(lfn);
    do_n2n(l_lfn);

    Info(UgrLogger::Lvl2, "UgrConnector::list", "Listing " << l_lfn);

    // See if the info is in cache
    // If not in memory create an object and trigger a search on it
    UgrFileInfo *fi = locHandler.getFileInfoOrCreateNewOne(*this, l_lfn, true, true);

    {
        boost::lock_guard<UgrFileInfo > l(*fi);
        if (fi->getItemsStatus() == UgrFileInfo::NoInfo)
            do_List(fi);
    }

    // wait for the search to finish by looking at the pending object
    do_waitList(fi, UgrCFG->GetLong("glb.waittimeout", 30));



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
    if (UgrCFG->GetBool("glb.statsubdirs", false))
        statSubdirs(fi);

    *nfo = fi;

    // Touch the item anyway, it has been referenced
    fi->touch();

    // Send, if needed, to the external cache
    this->locHandler.putSubitemsToCache(fi);

    Info(UgrLogger::Lvl1, "UgrConnector::list", "Listed " << l_lfn << " items:" << fi->subdirs.size() << " Status: " << fi->getItemsStatus() <<
            " status_items: " << fi->status_items << " pending_items: " << fi->pending_items);

    return 0;
}





int UgrConnector::checkperm(const char *fname,
                          const std::string &clientName,
                          const std::string &remoteAddress,
                          const std::vector<std::string> &fqans,
                          const std::vector< std::pair<std::string, std::string> > &keys,
                          char *reqresource, char reqmode) {
    
    bool ok = false;
    
    // If one of the auth plugins accepts, then it's accepted
    for (unsigned int i = 0; i < authorizationPlugins.size(); i++) {
        if (authorizationPlugins[i]->isallowed(fname,
                          clientName,
                          remoteAddress,
                          fqans,
                          keys,
                          reqresource, reqmode)) {
            ok = true;
            break;
        }
    }
    
    if ( !ok ) {
        return 1;
    }
    
    return 0;
}




const std::string & getUgrLibPath(){
// path of the ugr shared object
static std::unique_ptr<std::string> lib_path;
static boost::mutex mu_lib_path;

if(lib_path.get() == NULL){
    boost::lock_guard<boost::mutex> l(mu_lib_path);
    if(lib_path.get() == NULL){
        Dl_info shared_lib_infos;

        // do an address resolution on a local function
        // get this resolution to determine davix shared library path at runtime
        if( dladdr((void*) &getUgrLibPath, &shared_lib_infos) != 0){
            lib_path = std::unique_ptr<std::string>(new std::string(shared_lib_infos.dli_fname));
        }
    }
}

return *lib_path;
}

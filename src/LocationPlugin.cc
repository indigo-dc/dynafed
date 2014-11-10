/** @file   LocationPlugin.cc
 * @brief  Base class for an UGR location plugin
 * @author Fabrizio Furano
 * @date   Oct 2011
 */
#include "LocationPlugin.hh"
#include "UgrConnector.hh"
#include "PluginLoader.hh"
#include "UgrMemcached.pb.h"
#include "ExtCacheHandler.hh"
#include <time.h>
#include <sys/stat.h>



const std::string location_config_prefix = "locplugin.";

void pluginFunc(LocationPlugin *pl, int myidx) {
    const char *fname = "LocationPlugin::pluginFunc";
    Info(UgrLogger::Lvl4, fname, "Worker: started");

    // Get some work to do
    while (!pl->exiting) {


        struct LocationPlugin::worktoken *op = pl->getOp();

        if(op && op->operation){
            op->operation();
            continue;
        }

        // Check if a ping has to be performed
        if (op && (op->wop == LocationPlugin::wop_Check)) {

            // If it was already running, ignore the req
            // If not, set it to running and process the check
            if (!pl->availInfo.setCheckRunning(true)) continue;

            pl->do_Check(myidx);
            pl->availInfo.setCheckRunning(false);

            continue;
        }

        if (op && op->fi && op->wop) {

            // Run this search, including notifying the various calls
            pl->runsearch(op, myidx);

        }
    }

    Info(UgrLogger::Lvl4, fname, "Worker: finished");

}


// Sort Container by descending len function
bool sortStringsByDescLen(const std::string &a, const std::string &b) { return a.size() > b.size(); }


LocationPlugin::LocationPlugin(UgrConnector & c, std::vector<std::string> &parms) :
    PluginInterface(c, parms),
    name(),
    workers(),
    availInfo(),
    extCache(NULL),
    exiting(false)
{
    CFG->Set(&c.getConfig());

    const char *fname = "LocationPlugin::LocationPlugin";
    nthreads = 0;
    extCache = 0;

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
        Error(fname, "Fixing nthreads: " << nthreads << "-->10000");
        nthreads = 10000;
    }

    
    // Private logging... refer to the component name locplugin.<name>
    pluglogname = "locplugin.";
    pluglogname += name;
    pluglogmask = UgrLogger::get()->getMask(pluglogname);
    
    
    // Now get from the config any item built as:
    // locplugin.<name>.variablename
    // or
    // locplugin.<name>.arrayname[]
    // es.
    // locplugin.dmlite1.xlatepfx /dpm/cern.ch/ /
    // locplugin.http1.host[] http://exthost.y.z/path_pfx_to_strip

    // Get the xlatepfx
    std::string pfx = "locplugin.";
    pfx += name;

    std::string s = pfx;
    s += ".xlatepfx";

    std::string v;
    v = CFG->GetString(s.c_str(), (char *) "");

    if (v.size() > 0) {


        std::vector<std::string> parms = tokenize(v, " ");
        if (parms.size() < 2) {
            Error(fname, "Bad xlatepfx: '" << v << "'");
        } else {
            unsigned int i;
            for (i = 0; i < parms.size() - 1; i++)
                xlatepfx_from.push_back(parms[i]);
	    
            xlatepfx_to = parms[parms.size()-1];
	    UgrFileInfo::trimpath(xlatepfx_to);
	    
            for (i = 0; i < parms.size() - 1; i++) {
		UgrFileInfo::trimpath(xlatepfx_from[i]);
                Info(UgrLogger::Lvl1, fname, name << " Translating prefixes '" << xlatepfx_from[i] << "' -> '" << xlatepfx_to << "'");
            }
        }
    }
    
    // Very important... the xlatepfx_from vector must be sorted by descending string length
    std::sort(xlatepfx_from.begin(), xlatepfx_from.end(), sortStringsByDescLen);

    // Now get the content of pfxmultiply
    pfx = "locplugin.";
    pfx += name;

    s = pfx;
    s += ".pfxmultiply";

    v = CFG->GetString(s.c_str(), (char *) "");

    if (v.size() > 0) {


        std::vector<std::string> parms = tokenize(v, " ");
        if (parms.size() < 2) {
            Error(fname, "Bad pfxmultiply: '" << v << "'");
        } else {
            unsigned int i;
            for (i = 0; i < parms.size(); i++)
                pfxmultiply.push_back(parms[i]);
            
            for (i = 0; i < pfxmultiply.size(); i++) {
		UgrFileInfo::trimpath(pfxmultiply[i]);
                Info(UgrLogger::Lvl1, fname, name << " Multiplying prefixes '" << pfxmultiply[i]);
            }
        }
    }
    
    // Very important... the pfxmultiply vector must be sorted by descending string length
    std::sort(pfxmultiply.begin(), pfxmultiply.end(), sortStringsByDescLen);
    
    // get state checker
    availInfo.state_checking = CFG->GetBool(pfx + ".status_checking", true);
    Info(UgrLogger::Lvl1, fname, " State checker : " << ((availInfo.state_checking) ? "ENABLED" : "DISABLED"));

    availInfo.time_interval_ms = CFG->GetLong(pfx + ".status_checker_frequency", 5000);
    Info(UgrLogger::Lvl1, fname, " State checker frequency : " << availInfo.time_interval_ms);

    // get maximum latency
    availInfo.max_latency_ms = CFG->GetLong(pfx + ".max_latency", 10000);
    Info(UgrLogger::Lvl1, fname, " Maximum Endpoint latency " << availInfo.max_latency_ms << "ms");

    // get slave status
    slave = CFG->GetBool(pfx + ".slave", false);
    Info(UgrLogger::Lvl1, fname, " Slave : " << ((slave) ? "true" : "false"));

    // get replica xlator
    replicaXlator = CFG->GetBool(pfx + ".replicaxlator", false);
    Info(UgrLogger::Lvl1, fname, " ReplicaXlator : " << ((replicaXlator) ? "true" : "false"));

    // get readable / writable / listable capability
    setFlag(Readable, CFG->GetBool(pfx + ".readable", true));
    setFlag(Listable, CFG->GetBool(pfx + ".listable", true));
    setFlag(Writable, CFG->GetBool(pfx + ".writable", false));
    Info(UgrLogger::Lvl1, fname, "configured as " <<  (getFlag(Readable)?"Readable":"")
                    << " " << (getFlag(Writable)?"Writable":"")
                    << " " << (getFlag(Listable)?"Listable":""));

    exiting = false;


};

void LocationPlugin::stop() {
    const char *fname = "LocationPlugin::stop";

    exiting = true;
    availInfo.state_checking = false;

    /// Note: this tends to hang due to a known bug in boost
    //for (unsigned int i = 0; i < workers.size(); i++) {
    //        LocPluginLogInfo(UgrLogger::Lvl1, fname, "Interrupting thread: " << i);
    //        workers[i]->interrupt();
    //    }

    for (unsigned int i = 0; i < workers.size(); i++) {

        pushOp(0, 0);
    }

    for (unsigned int i = 0; i < workers.size(); i++) {
        LocPluginLogInfo(UgrLogger::Lvl1, fname, "Joining thread: " << i);
        workers[i]->join();
    }

    LocPluginLogInfo(UgrLogger::Lvl1, fname, "Deleting " << workers.size() << " threads. ");
    while (workers.size() > 0) {
        delete *workers.begin();
        workers.erase(workers.begin());
    }
}

int LocationPlugin::start(ExtCacheHandler *c) {
    const char *fname = "LocationPlugin::start";

    extCache = c;

    // Create our pool of threads
    LocPluginLogInfo(UgrLogger::Lvl1, fname, "creating " << nthreads << " threads.");
    for (int i = 0; i < nthreads; i++) {
        workers.push_back(new boost::thread(pluginFunc, this, i));
    }

    return 0;
}

LocationPlugin::~LocationPlugin() {

}


// implement new location finder
// default behavior : notify completion and quit
int LocationPlugin::run_findNewLocation(const std::string & new_lfn, std::shared_ptr<NewLocationHandler> handler){
    const char *fname = "LocationPlugin::do_findNewLocation";

    LocPluginLogInfo(UgrLogger::Lvl4, fname,  get_Name() << " : No findNewLocation support for this plugin, default behavior");
    // do nothing
    return 1;
}


int LocationPlugin::run_deleteReplica(const std::string & lfn, std::shared_ptr<DeleteReplicaHandler> handler){
    const char *fname = "LocationPlugin::run_deleteReplica";

    LocPluginLogInfo(UgrLogger::Lvl4, fname,  get_Name() << " : No deleteReplica support for this plugin, default behavior");
    // do nothing
    return 1;
}


// Pushes a new op in the queue

void LocationPlugin::pushOp(UgrFileInfo *fi, LocationInfoHandler *handler, workOp wop, char *newpfx) {
    const char *fname = "LocationPlugin::pushOp";

    {
        boost::lock_guard< boost::mutex > l(workmutex);

        worktoken *tk = new(worktoken);
        tk->fi = fi;
        tk->wop = wop;
        tk->handler = handler;
	if (newpfx) tk->altpfx = newpfx;
        workqueue.push_back(tk);
    }

    LocPluginLogInfo(UgrLogger::Lvl4, fname, "pushed op:" << wop << " " << (fi ? fi->name : "") << " newpfx:" << newpfx);

    workcondvar.notify_one();

}

void LocationPlugin::pushOp(const std::function<void ()> & operation){
    const char *fname = "LocationPlugin::pushOp";

    {
        boost::lock_guard< boost::mutex > l(workmutex);

        worktoken *tk = new(worktoken);
        tk->wop = wop_Nop;
        tk->operation = operation;
        workqueue.push_back(tk);
    }

    LocPluginLogInfo(UgrLogger::Lvl4, fname, "push generic task");

    workcondvar.notify_one();

}

void LocationPlugin::pushRepCheckOp(UgrFileInfo *fi, LocationInfoHandler *handler, std::string &rep) {
    const char *fname = "LocationPlugin::pushRepCheckOp";

    {
        boost::lock_guard< boost::mutex > l(workmutex);

        worktoken *tk = new(worktoken);
        tk->fi = fi;
        tk->wop = wop_CheckReplica;
        tk->handler = handler;
        tk->repl = rep;
        workqueue.push_back(tk);
    }

    LocPluginLogInfo(UgrLogger::Lvl4, fname, "pushed op: wop_CheckReplica " << rep);

    workcondvar.notify_one();

}


/// Invokes a full round of CheckReplica towards other slave plugins

void LocationPlugin::req_checkreplica(UgrFileInfo *fi, std::string &repl) {

    getConn().do_checkreplica(fi, repl);
}



// Gets an op from the queue, or timeout

struct LocationPlugin::worktoken *LocationPlugin::getOp() {
    using namespace boost;
    struct worktoken *mytk = 0;
    const char *fname = "LocationPlugin::getOp";

    unique_lock< mutex > l(workmutex);

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
        LocPluginLogInfo(UgrLogger::Lvl4, fname, "got op:" << mytk->wop);
    }

    return mytk;
}

void LocationPlugin::runsearch(struct worktoken *op, int myidx) {
    using namespace boost;
    const char *fname = "LocationPlugin::runsearch";

    // Pretend to do something useful...
    posix_time::seconds workTime(1);
    boost::this_thread::sleep(workTime);

    LocPluginLogInfoThr(UgrLogger::Lvl2, fname, "Starting op: " << op->wop << "fn: " << op->fi->name);

    // Now put the results
    if (op) {

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

    }

    LocPluginLogInfoThr(UgrLogger::Lvl2, fname, "Finished op: " << op->wop << "fn: " << op->fi->name);
}



void LocationPlugin::run_Check(int myidx){

}




// Start the async stat process
// Mark the fileinfo with one more pending stat request (by this plugin)

int LocationPlugin::do_Stat(UgrFileInfo* fi, LocationInfoHandler *handler) {
  const char *fname = "LocationPlugin::do_Stat";
  
  LocPluginLogInfo(UgrLogger::Lvl4, fname, "Entering");
  
  
  // We may have to multiply this query, to search inside multiple prefixes for this plugin
  if (pfxmultiply.size() < 2) {
    // We immediately notify that this plugin is starting a search for this info
    // Depending on the plugin, the symmetric notifyNotPending() will be done
    // in a parallel thread, or inside do_waitstat
    fi->notifyStatPending();
    pushOp(fi, handler, wop_Stat);
  }
  else {
    
    // For every "pfxmultiply" item, we submit it with the query as suggested substitution
    // and submit the query
    for (unsigned int i = 0; i < pfxmultiply.size(); i++) {
      // We immediately notify that this plugin is starting a search for this info
      // Depending on the plugin, the symmetric notifyNotPending() will be done
      // in a parallel thread, or inside do_waitstat
      fi->notifyStatPending();      
      pushOp(fi, handler, wop_Stat, (char *)pfxmultiply[i].c_str());
    }
  }
  
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
  
  LocPluginLogInfo(UgrLogger::Lvl4, fname, "Entering");
  
  
  // We may have to multiply this query, to search inside multiple prefixes for this plugin
  if (pfxmultiply.size() < 2) {
    // We immediately notify that this plugin is starting a search for this info
    // Depending on the plugin, the symmetric notifyNotPending() will be done
    // in a parallel thread, or inside do_waitstat
    fi->notifyLocationPending();
    pushOp(fi, handler, wop_Locate);
  }
  else {
    
    // For every "pfxmultiply" item, we submit it with the query as suggested substitution
    // and submit the query
    for (unsigned int i = 0; i < pfxmultiply.size(); i++) {
      // We immediately notify that this plugin is starting a search for this info
      // Depending on the plugin, the symmetric notifyNotPending() will be done
      // in a parallel thread, or inside do_waitstat
      fi->notifyLocationPending();      
      pushOp(fi, handler, wop_Locate, (char *)pfxmultiply[i].c_str());
    }
  }
  
  
  
  
  
  return 0;
}

int LocationPlugin::do_CheckReplica(UgrFileInfo *fi, std::string &rep, LocationInfoHandler *handler) {
    const char *fname = "LocationPlugin::do_CheckReplica";

    LocPluginLogInfo(UgrLogger::Lvl4, fname, "Entering");

    // We immediately notify that this plugin is starting a search for this info
    // Depending on the plugin, the symmetric notifyNotPending() will be done
    // in a parallel thread, or inside do_waitstat
    fi->notifyLocationPending();

    pushRepCheckOp(fi, handler, rep);

    return 0;
}



void executor_findNewLocation(LocationPlugin* p, std::string new_lfn, std::shared_ptr<NewLocationHandler>  handler){
    p->run_findNewLocation(new_lfn, handler);
    handler->decWorker();
}

int LocationPlugin::async_findNewLocation(const std::string &new_lfn, const std::shared_ptr<NewLocationHandler> & handler){
    // run find new location
    // follow pattern setting up by Fab to extend it to fully asynchronous behavior in future with thread launch
    // handler is the completion handler of the operation

    handler->incWorker();
    pushOp(std::bind(&executor_findNewLocation, this, new_lfn, handler));
    return 0;
}


void executor_deleteReplica(LocationPlugin* p, std::string new_lfn, std::shared_ptr<DeleteReplicaHandler>  handler){
    p->run_deleteReplica(new_lfn, handler);
    handler->decWorker();
}

int LocationPlugin::async_deleteReplica(const std::string &lfn, const std::shared_ptr<DeleteReplicaHandler> & handler){
    handler->incWorker();
    pushOp(std::bind(&executor_deleteReplica, this, lfn, handler));
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
  
  LocPluginLogInfo(UgrLogger::Lvl4, fname, "Entering");
  
  
  
  
  // We may have to multiply this query, to search inside multiple prefixes for this plugin
  if (pfxmultiply.size() < 2) {
    // We immediately notify that this plugin is starting a search for this info
    // Depending on the plugin, the symmetric notifyNotPending() will be done
    // in a parallel thread, or inside do_waitstat
    fi->notifyItemsPending();
    pushOp(fi, handler, wop_List);
  }
  else {
    
    // For every "pfxmultiply" item, we submit it with the query as suggested substitution
    // and submit the query
    for (unsigned int i = 0; i < pfxmultiply.size(); i++) {
      // We immediately notify that this plugin is starting a search for this info
      // Depending on the plugin, the symmetric notifyNotPending() will be done
      // in a parallel thread, or inside do_waitstat
      fi->notifyItemsPending();      
      pushOp(fi, handler, wop_List, (char *)pfxmultiply[i].c_str());
    }
  }
  
  return 0;
}


bool LocationPlugin::doParentQueryCheck(std::string & from, struct worktoken *wtk, int myidx){
    const char* fname = "LocationPlugin::doParentQueryCheck";
    bool doitemsnotify = false;
    // Loop through the xlatepfx alternatives
    for( std::vector<std::string>::iterator it = xlatepfx_from.begin(); it < xlatepfx_from.end(); it++){
      
	// IF we are querying for a substring of any of the xlatepfx
	//  AND this substring is followed by '/'
        if( it->size() > from.size()
           && it->compare(0, from.size(), from) == 0
           && ( it->at(from.size()) == '/' || from.size() == 1)) {
	    // if query on parent translated dir
            
            switch(wtk->wop){
                case LocationPlugin::wop_List :{
		  
		    UgrFileItem item;
		    const size_t pos = it->find('/', from.size()+1);
		    
		    if (from.size() == 1) {
		      // Particular case, we are querying the root dir
		      item.name = it->substr(from.size(), (pos == std::string::npos)?std::string::npos: pos - from.size());
		    }
		    else {
		      item.name = it->substr(from.size()+1, (pos == std::string::npos)?std::string::npos: pos - from.size()-1);
		    }
		    

                    wtk->fi->setPluginID(getID());

                    LocPluginLogInfoThr(UgrLogger::Lvl4, fname, "Worker: Inserting prefix item  " << item.name);
                    wtk->fi->subdirs.insert(item);

                    wtk->fi->status_items = UgrFileInfo::Ok;
		    doitemsnotify = true;
		    continue;
                }
                case LocationPlugin::wop_Stat:{
                    wtk->fi->setPluginID(getID());
                    struct stat st = {};
                    st.st_nlink = 1;
                    st.st_mode |= S_IFDIR;
		    st.st_mode |= ACCESSPERMS;
                    wtk->fi->takeStat(st);

                    LocPluginLogInfoThr(UgrLogger::Lvl4, fname, "Notify End Stat");
                    wtk->fi->notifyStatNotPending();
                    return true;
                }
                default:
                    // no ops
                    break;
            }

        }

    }

    if (doitemsnotify) {
      LocPluginLogInfoThr(UgrLogger::Lvl4, fname, "Notify End Listdir");
      wtk->fi->notifyItemsNotPending();
      return true;
    }

    return false;
}

// Waits max a number of seconds for a list process to be complete

int LocationPlugin::do_waitList(UgrFileInfo *fi, int tmout) {
    return 0;
}



// default name xlation
// Return 0 if the prefix was found (and hence translated)

int LocationPlugin::doNameXlation(std::string &from, std::string &to, workOp op, std::string &altpfx) {
    const char *fname = "LocationPlugin::doNameXlation";
    int r = 1;
    size_t i;
    const size_t xtlate_size = xlatepfx_from.size();

    if(xtlate_size == 0){ // no translation required
      to = from;
      r = 0;
    }
    else {
      for (i = 0; i < xtlate_size; i++) {
	if ((xlatepfx_from[i].size() > 0) &&
	  ((from.size() == 0) || (from.compare(0, xlatepfx_from[i].length(), xlatepfx_from[i]) == 0))) {
	  
	  if (from.size() == 0)
	    to = xlatepfx_to;
	  else
	    to = xlatepfx_to + from.substr(xlatepfx_from[i].length());
	  
	  r = 0;
	break;
	
	  }
      }
      
      if (r) to = from;
    }

    LocPluginLogInfo(UgrLogger::Lvl3, fname, "xlated pfx: " << from << "->" << to);

    // If r is nonzero then a xlatepfx translation was specified, AND no matching prefix was found
    if (r) {
      LocPluginLogInfo(UgrLogger::Lvl3, fname, "No match on xlated pfx: " << from);
      return r;
    }
    
    // We are here if the prefix xlation for the query succeeded, and the path has been translated
    // (or there was no xlation to apply)
    // We may have a xlation suggestion coming from the pfxmultiply, in the format of a bare prefix
    // to add to this query. An usage of this is to prepend the names of spacetokens
    // where to multiply to query to
    if (altpfx.size() > 1) {
      to.insert(0, altpfx);
    }
    
    LocPluginLogInfo(UgrLogger::Lvl3, fname, "xlated pfx: " << from << "->" << to);
    
    return 0;
}






/// Gives life to the object

int LocationPlugin::Tick(time_t timenow) {
    PluginEndpointStatus st;

    // If the status is dirty and the info is valid, write to the extcache
    if (extCache && !availInfo.isExpired(timenow) && availInfo.isDirty()) {

        availInfo.setDirty(false);
        availInfo.getStatus(st);
        extCache->putEndpointStatus(&st, name);
    }

    // If the curr status is expired, if possible get from the extcache one
    // that is not expired
    if (availInfo.isExpired(timenow)) {
        if (extCache && !extCache->getEndpointStatus(&st, name)) {
            availInfo.setStatus(st, false, (char *) name.c_str());
        }
    }

    // If the info is still expired then trigger a refresh towards the endpoint
    // The refresh will write updated info into the extcache
    if (availInfo.isExpired(timenow)) {
        pushOp(0, 0, wop_Check);
    }

    return 0;
}

void LocationPlugin::do_Check(int myidx){
    if(availInfo.state_checking)
        run_Check(myidx);
}

const std::string & LocationPlugin::getConfigPrefix(){
    return location_config_prefix;
}

PluginAvailabilityInfo::PluginAvailabilityInfo(int interval_ms, int latency_ms) {
    isCheckRunning = false;
    status_dirty = false;
    time_interval_ms = interval_ms;
    max_latency_ms = latency_ms;
    lastchange = 0;
}

bool PluginAvailabilityInfo::getCheckRunning() {
    boost::unique_lock< boost::mutex > l(workmutex);
    return isCheckRunning;
}

bool PluginAvailabilityInfo::setCheckRunning(bool b) {
    boost::unique_lock< boost::mutex > l(workmutex);
    bool r = isCheckRunning;

    // Return false if the status was what we are setting
    if (r == b) return false;

    isCheckRunning = b;

    // Return true if we changed the status
    return true;
}

bool PluginAvailabilityInfo::isExpired(time_t timenow) {
    return (timenow - this->status.lastcheck)*1000 > time_interval_ms;
}

void PluginAvailabilityInfo::getStatus(PluginEndpointStatus &st) {
    boost::unique_lock< boost::mutex > l(workmutex);
    st = status;
}

void PluginAvailabilityInfo::setStatus(PluginEndpointStatus &st, bool setdirty, char *logname) {

    // Set state, log the status change
    // A status change in an endpoint is logged at level kLOW
    // A status setting is logged at level kHIGHEST
    short lvl = UgrLogger::Lvl3;
    const char *online = "ONLINE";
    const char *offline = "OFFLINE";
    const char *s = online;
    bool reject = true;

    {
        boost::unique_lock< boost::mutex > l(workmutex);

        // Reject the status if it is older than the one that we already have
        if (st.lastcheck > status.lastcheck) {
            if (st.state != PLUGIN_ENDPOINT_ONLINE) s = offline;
	    
	    
            if (st.state != status.state) {
	      lvl = UgrLogger::Lvl1;
	      if (status.state != PLUGIN_ENDPOINT_UNKNOWN) lastchange = st.lastcheck;
	    }
            status = st;
            if (setdirty) status_dirty = true;
            reject = false;
        }
    }

    if (reject) {
        Info(UgrLogger::Lvl4, "PluginAvailabilityInfo::setStatus",
                " Status of " << logname <<
                " checked: " << s << " was rejected.");
    } else {
        Info(lvl, "PluginAvailabilityInfo::setStatus",
                " Status of " << logname <<
                " checked: " << s << ", HTTP code: " << st.errcode <<
                " latency: " << st.latency_ms << "ms" <<
                " desc: " << st.explanation);
    }

}



/// We will like to be able to encode this info to a string, e.g. for external caching purposes

int PluginEndpointStatus::encodeToString(std::string &str) {
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    ugrmemcached::SerialEndpointStatus ses;

    ses.set_errcode(errcode);
    ses.set_lastcheck(lastcheck);
    ses.set_latency_ms(latency_ms);
    ses.set_state(state);

    str = ses.SerializeAsString();

    return (str.length() > 0);
}

/// We will like to be able to encode this info to a string, e.g. for external caching purposes

int PluginEndpointStatus::decode(void *data, int sz) {
    if (!sz) return 1;

    ugrmemcached::SerialEndpointStatus ses;

    ses.ParseFromArray(data, sz);

    errcode = ses.errcode();
    lastcheck = ses.lastcheck();
    latency_ms = ses.latency_ms();
    state = (PluginEndpointState) ses.state();

    return 0;
}


// ------------------------------------------------------------------------------------
// Plugin-related stuff
// ------------------------------------------------------------------------------------



// The plugin functionality. This function invokes the plugin loader, looking for the
// plugin where to call the hook function

PluginInterface *GetPluginInterfaceClass(char *pluginPath, GetPluginInterfaceArgs) {
    const char *fname = "GetPluginInterfaceClass_local";
    PluginLoader *myLib = 0;
    PluginInterface * (*ep)(GetPluginInterfaceArgs);

    // If we have no plugin path then return NULL
    if (!pluginPath || !strlen(pluginPath)) {
        Info(UgrLogger::Lvl2, fname, "No plugin to load.");
        return NULL;
    }

    // Create a plugin object (we will throw this away without deletion because
    // the library must stay open but we never want to reference it again).
    if (!myLib) {
        Info(UgrLogger::Lvl2, fname, "Loading plugin " << pluginPath);
        if (!(myLib = new PluginLoader(pluginPath))) {
            Info(UgrLogger::Lvl1, fname, "Failed loading plugin " << pluginPath);
            return NULL;
        }
    } else {
        Info(UgrLogger::Lvl2, fname, "Plugin " << pluginPath << "already loaded.");
    }

    // Now get the entry point of the object creator
    Info(UgrLogger::Lvl2, fname, "Getting entry point for plugin " << pluginPath);
    ep = (PluginInterface * (*)(GetPluginInterfaceArgs))(myLib->getPlugin("GetPluginInterface"));
    if (!ep) {
        Info(UgrLogger::Lvl1, fname, "Could not get entry point for plugin " << pluginPath);
        return NULL;
    }

    // Get the Object now
    Info(UgrLogger::Lvl2, fname, "Getting class instance for plugin " << pluginPath);
    PluginInterface *p = ep(c, parms);
    if (!p)
        Info(UgrLogger::Lvl1, fname, "Could not get class instance for plugin " << pluginPath);
    return p;

}




/// The plugin hook function. GetPluginInterfaceClass must be given the name of this function
/// for the plugin to be loaded

extern "C" PluginInterface *GetPluginInterface(GetPluginInterfaceArgs) {
    return (PluginInterface *)new LocationPlugin(c, parms);
}

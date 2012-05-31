#ifndef LOCATIONPLUGIN_HH
#define LOCATIONPLUGIN_HH

/* LocationPlugin
 * Base class for a plugin which gathers info about files from some source
 * If not properly subclassed, this acts as a default fake plugin, putting test data
 * as responses
 *
 *
 * by Fabrizio Furano, CERN, Oct 2011
 */

#include "Config.hh"
#include "SimpleDebug.hh"
#include "LocationInfo.hh"

#include <string>
#include <vector>
#include <map>
#include <boost/thread.hpp>


#define LocPluginLogInfo(l, n, c) Info(SimpleDebug::kMEDIUM, fname, "LocPlugin: " << this->name << " " << c);
#define LocPluginLogErr(n, c) Info(fname, "LocPlugin: " << this->name << " " << c);

class LocationPlugin {

    friend void pluginFunc(LocationPlugin *pl);

protected:
    // The name assigned to this plugin from the creation
    char *name;

    // We keep a thread pool and a synchronized work queue, in order to always guarantee
    // pure non blocking behaviour
    std::vector< boost::thread * > workers;

    enum workOp {
        wop_Nop = 0,
        wop_Stat,
        wop_Locate,
        wop_List
    };
    struct worktoken {
        UgrFileInfo *fi;
        workOp wop;
    };
    std::deque< struct worktoken *> workqueue;
    boost::condition_variable workcondvar;
    boost::mutex workmutex;

    void pushOp(UgrFileInfo *fi, workOp wop);
    struct worktoken *getOp();

    virtual void runsearch(struct worktoken *wtk);
    

public:

   LocationPlugin(SimpleDebug *dbginstance, Config *cfginstance, std::vector<std::string> &parms);
   virtual ~LocationPlugin();


   // Calls that characterize the behevior of the plugin
   // In general:
   //  do_XXX triggers the start of an async task that gathers a specific kind of info
   //   it's not serialized, and it has to return immediately, exposing a non-blocking behavior
   //  do_waitXXX waits for the completion of the task that was triggered by the corresponding
   //   invokation of do_XXX. This is a blocking function, that must honour the timeout value that
   //   is given
      
   // The async stat process will put (asynchronously) the required info directly in the data fields of
   // the given instance of UgrFileInfo. Access to this data struct has to be serialized, since it's
   // a shared thing

   // Start the async stat process
   virtual int do_Stat(UgrFileInfo *fi);
   // Waits max a number of seconds for a locate process to be complete
   virtual int do_waitStat(UgrFileInfo *fi, int tmout=5);

   // Start the async location process
   virtual int do_Locate(UgrFileInfo *fi);
   // Waits max a number of seconds for a locate process to be complete
   virtual int do_waitLocate(UgrFileInfo *fi, int tmout=5);

   // Start the async listing process
   virtual int do_List(UgrFileInfo *fi);
   // Waits max a number of seconds for a list process to be complete
   virtual int do_waitList(UgrFileInfo *fi, int tmout=5);

};



// ------------------------------------------------------------------------------------
// Plugin-related stuff
// ------------------------------------------------------------------------------------

// The set of args that have to be passed to the plugin hook function
#define GetLocationPluginArgs SimpleDebug *dbginstance, Config *cfginstance, std::vector<std::string> &parms

// The plugin functionality. This function invokes the plugin loader, looking for the
// plugin where to call the hook function
LocationPlugin *GetLocationPluginClass(char *pluginPath, GetLocationPluginArgs);



#endif


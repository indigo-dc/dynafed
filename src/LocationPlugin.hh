/** @file   LocationPlugin.hh
 * @brief  Base class for an UGR location plugin
 * @author Fabrizio Furano
 * @date   Oct 2011
 */
#ifndef LOCATIONPLUGIN_HH
#define LOCATIONPLUGIN_HH


#include "Config.hh"
#include "SimpleDebug.hh"
#include "LocationInfoHandler.hh"
#include "PluginInterface.hh"

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <bitset>
#include <boost/thread.hpp>

class LocationInfoHandler;
class UgrConnector;

#define LocPluginLogInfo(lvl, where, what) do {                                											\
	if (UgrLogger::get()->getLevel() >= lvl && UgrLogger::get()->isLogged(pluglogmask)) 	\
	{    																	\
		std::ostringstream outs;                                   			\
		outs << "UGR " << pluglogname << " " << where << " " << __func__ << " : " << what;                      			\
		UgrLogger::get()->log((UgrLogger::Level)lvl, outs.str());    				\
	}                                                             			\
}while(0)

#define LocPluginLogInfoThr(lvl, where, what) do {                                											\
	if (UgrLogger::get()->getLevel() >= lvl && UgrLogger::get()->isLogged(pluglogmask)) 	\
	{    																	\
		std::ostringstream outs;                                   			\
		outs << "UGR " << pluglogname << "[" << myidx << "] " << where << " " << __func__ << " : " << what;                      			\
		UgrLogger::get()->log((UgrLogger::Level)lvl, outs.str());    				\
	}                                                             			\
}while(0)

#define LocPluginLogErr(where, what) do {                                											\
		std::ostringstream outs;                                   			\
		outs << "UGR " << pluglogname << "[" << myidx << "] " << where << " !! " << __func__ << " : " << what;                      			\
		UgrLogger::get()->log((UgrLogger::Level)0, outs.str());    				\
}while(0)


enum PluginEndpointState {
    PLUGIN_ENDPOINT_UNKNOWN = 0,
    PLUGIN_ENDPOINT_ONLINE,
    PLUGIN_ENDPOINT_OFFLINE,
    PLUGIN_ENDPOINT_TEMPORARY_OFFLINE,
    PLUGIN_ENDPOINT_NOT_EXIST,
    PLUGIN_ENDPOINT_OVERLOADED,
    PLUGIN_ENDPOINT_ERROR_AUTH,
    PLUGIN_ENDPOINT_ERROR_OTHER,
};

/// contains information about the availability of the plugin endpoint

class PluginEndpointStatus {
public:
    /// current status of the plugin's endpoint
    PluginEndpointState state;
    /// average latency in ms
    int latency_ms;
    /// error code (e.g. the http one)
    int errcode;
    /// string description
    std::string explanation;
    /// when the status was checked last
    time_t lastcheck;

    /// We will like to be able to encode this info to a string, e.g. for external caching purposes
    int encodeToString(std::string &str);

    /// We will like to be able to encode this info to a string, e.g. for external caching purposes
    int decode(void *data, int sz);

    PluginEndpointStatus() {
        lastcheck = 0;
        state = PLUGIN_ENDPOINT_UNKNOWN;
        latency_ms = 0;
        errcode = 0;
    };
};

class PluginAvailabilityInfo {
public:
    PluginAvailabilityInfo(int interval_ms = 5000, int latency_ms = 5000);


    /// Do we do any status checking at all?
    bool state_checking;
    /// How often the check has to be made
    int time_interval_ms;
    /// The maximum allowed latency
    int max_latency_ms;

    virtual bool isOK() {
        // A plugin can be used if it's ONLINE since 10 checks
        return ((status.state <= PLUGIN_ENDPOINT_ONLINE) &&
	  (time(0)-lastchange > time_interval_ms/100));
    }

    bool getCheckRunning();
    bool setCheckRunning(bool b);

    bool isExpired(time_t timenow);

    /// Gets the current status for this plugin/endpoint
    virtual void getStatus(PluginEndpointStatus &st);

    /// Sets the current status for this plugin/endpoint, set dirty
    virtual void setStatus(PluginEndpointStatus &st, bool setdirty, char *logname);

    virtual bool isDirty() {
        return status_dirty;
    }

    virtual void setDirty(bool d) {
        boost::unique_lock< boost::mutex > l(workmutex);
        status_dirty = d;
    }
private:
    boost::mutex workmutex;
    bool isCheckRunning;


    /// The current status
    PluginEndpointStatus status;
    bool status_dirty;
    
    /// when the status changed last time
    time_t lastchange;


};

/** LocationPlugin
 * Base class for a plugin which gathers info about files from some source. No assumption
 * is made about what source is this.
 * This base implementation acts as a default fake plugin, that puts test data
 * as responses. Very useful for testing.
 * 
 */
class LocationPlugin : public PluginInterface {
private:
    std::bitset<64> flags;
    int nthreads;
    /// Easy way to get threaded life
    friend void pluginFunc(LocationPlugin *pl, int myidx);
    friend void executor_findNewLocation(LocationPlugin* p, std::string new_lfn, std::shared_ptr<NewLocationHandler>  handler);
    friend void executor_deleteReplica(LocationPlugin* p, std::string new_lfn, std::shared_ptr<DeleteReplicaHandler>  handler);

public:

    enum workOp {
        wop_Nop = 0,
        wop_Stat,
        wop_Locate,
        wop_List,
        wop_Check,
        wop_CheckReplica
    };

    enum Flag{
        Readable=0,
        Writable=1,
        Listable=2
    };
    
    /// The description of an operation to be done asynchronously
    struct worktoken {
        /// FileInfo that refers to the file
        UgrFileInfo *fi;
	/// Operation to perform to gather info on the item
        workOp wop;
	
        LocationInfoHandler *handler;
	
	/// Alternative replica name to check for the given file
        std::string repl;
	/// Alternative prefix to locally prepend to the string being searched
        std::string altpfx;

       /// alternative execution task
       ///  if operation is valid, execute only operation
       std::function<void (void)> operation;
    };

protected:

    /// The name assigned to this plugin from the creation
    std::string name;

    /// We keep a private thread pool and a synchronized work queue, in order to provide
    /// pure non blocking behaviour
    std::vector< boost::thread * > workers;

    /// Online/offline, etc
    PluginAvailabilityInfo availInfo;

    // Ext cache
    ExtCacheHandler *extCache;


    // Workaround for a bug in boost, where interrupt() hangs
    bool exiting;

    /// Tells us if this plugin is invoked directly by UgrConnector (slave=false) or by another plugin (slave=true)
    bool slave;
    /// Tells us if this plugin demands the validation of the replicas to the slave plugins
    bool replicaXlator;

    /// Queue of the pending operations
    std::deque< struct worktoken *> workqueue;
    /// Condvar for synchronizing the queue
    boost::condition_variable workcondvar;
    /// Mutex for protecting the queue
    boost::mutex workmutex;

    UgrLogger::bitmask pluglogmask;
    UgrLogger::component pluglogname;
    
    /// Push into the queue a new op to be performed, relative to an instance of UgrFileInfo
    void pushOp(UgrFileInfo *fi, LocationInfoHandler *handler, workOp wop = wop_Nop, char *newpfx = 0);
    /// Push generic operation to be performed
    void pushOp(const std::function<void (void)> & operation);

    void pushRepCheckOp(UgrFileInfo *fi, LocationInfoHandler *handler, std::string &rep);
    /// Gets the next op to perform
    struct worktoken *getOp();

    /// The method that performs the operation
    /// This has to be overridden in the true plugins
    virtual void runsearch(struct worktoken *wtk, int myidx);

    /// Implement the Checker at the plugin level
    ///
    virtual void run_Check(int myidx);

    /// execute a findNewLocation operation, need to be implemented by the plugin
    /// @param fi UgrFileInfo instance to populate
    /// @param handler the location info handler to write into
    virtual int run_findNewLocation(const std::string & new_lfn, std::shared_ptr<NewLocationHandler> handler);


    /// execute a deleteReplica operation, this need to be implemented by the plugin
    ///
    virtual int run_deleteReplica(const std::string & lfn, std::shared_ptr<DeleteReplicaHandler> handler);



    // The simple, default global name translation
    std::vector<std::string> xlatepfx_from;
    std::string xlatepfx_to;
    
    // The prefix multiplier, to look for files in multiple dirs at once
    std::vector<std::string> pfxmultiply;

    /// Applies the plugin-specific name translation. This may depend also on the operation being requested.
    virtual int doNameXlation(std::string &from, std::string &to, workOp op, std::string &altpfx);

    /// Add parent directory if needed
    bool doParentQueryCheck(std::string & from, struct worktoken *wtk, int myidx);

    /// Invokes a full round of CheckReplica towards other slave plugins
    virtual void req_checkreplica(UgrFileInfo *fi, std::string &repl);

public:

    LocationPlugin(UgrConnector & c, std::vector<std::string> &parms);
    virtual ~LocationPlugin();

    virtual void stop();
    virtual int start(ExtCacheHandler *c);

    /// Gives life to the object
    virtual int Tick(time_t timenow);

    virtual int isOK() {
        return availInfo.isOK();
    }

    /// Tells us if this plugin is invoked directly by UgrConnector (slave=false) or by another plugin (slave=true)

    bool isSlave() {
        return slave;
    };

    /// Tells us if this plugin demands the validation of the replicas to the slave plugins

    bool isReplicaXlator() {
        return replicaXlator;
    }

    bool getFlag(Flag flag_type){
        return flags[static_cast<size_t>(flag_type)];
    }

    void setFlag(Flag flag_type, bool value){
        flags[static_cast<size_t>(flag_type)] = value;
    }

    ///
    /// return the plugin name ( id )
    ///

    virtual const std::string & get_Name() {
        return name;
    }


    // Calls that characterize the behavior of the plugin
    // In general:
    //  do_XXX triggers the start of an async task that gathers a specific kind of info
    //   it's not serialized, and it has to return immediately, exposing a non-blocking behavior
    //  do_waitXXX waits for the completion of the task that was triggered by the corresponding
    //   invokation of do_XXX. This is a blocking function, that must honour the timeout value that
    //   is given

    // The async stat process will put (asynchronously) the required info directly in the data fields of
    // the given instance of UgrFileInfo. Access to this data struct has to be properly protected, since it's
    // a shared thing

    /// Start the async stat process
    /// @param fi UgrFileInfo instance to populate
    /// @param handler the location info handler to write into
    virtual int do_Stat(UgrFileInfo *fi, LocationInfoHandler *handler);
    /// Waits max a number of seconds for a stat process to be complete
    /// @param fi UgrFileInfo instance to wait for
    /// @param tmout Timeout for waiting
    virtual int do_waitStat(UgrFileInfo *fi, int tmout = 5);

    /// Start the async location process
    /// @param fi UgrFileInfo instance to populate
    /// @param handler the location info handler to write into
    virtual int do_Locate(UgrFileInfo *fi, LocationInfoHandler *handler);
    /// Waits max a number of seconds for a locate process to be complete
    /// @param fi UgrFileInfo instance to wait for
    /// @param tmout Timeout for waiting
    virtual int do_waitLocate(UgrFileInfo *fi, int tmout = 5);

    /// Start the async listing process
    /// @param fi UgrFileInfo instance to populate
    /// @param handler the location info handler to write into
    virtual int do_List(UgrFileInfo *fi, LocationInfoHandler *handler);
    /// Waits max a number of seconds for a list process to be complete
    /// @param fi UgrFileInfo instance to wait for
    /// @param tmout Timeout for waiting
    virtual int do_waitList(UgrFileInfo *fi, int tmout = 5);

    ///
    /// \brief async_findNewLocation
    /// \param new_lfn
    /// \param handler
    /// \return 0 if success, negative if plugin error
    ///
    /// Execute a findNewLocation query on this plugin asynchronously. this call run_findNewLocation internally
    ///
    int async_findNewLocation(const std::string & new_lfn, const std::shared_ptr<NewLocationHandler> & handler);

    ///
    /// \brief async_deleteReplica
    /// \param lfn
    /// \param handler
    /// \return 0 if success, negative if plugin error
    ///
    /// Execute a delete operation on this plugin asynchronously. this call run_deleteReplica internally
    ///
    int async_deleteReplica(const std::string & lfn, const std::shared_ptr<DeleteReplicaHandler> & handler);


    /// Asynchronously check if this plugin knows about the given replica
    /// Eventually add the replica
    /// @param fi UgrFileInfo instance to populate
    /// @param rep the replica to check
    /// @param handler the location info handler to write into
    virtual int do_CheckReplica(UgrFileInfo *fi, std::string &rep, LocationInfoHandler *handler);


    /// Asynchronously check the plugin Status
    void do_Check(int myidx);

public:
    static const std::string & getConfigPrefix();

};



// ------------------------------------------------------------------------------------
// Plugin-related stuff
// ------------------------------------------------------------------------------------

/// The set of args that have to be passed to the plugin hook function
#define GetPluginInterfaceArgs UgrConnector & c, std::vector<std::string> &parms

/// The plugin functionality. This function invokes the plugin loader, looking for the
/// plugin where to call the hook function
PluginInterface *GetPluginInterfaceClass(char *pluginPath, GetPluginInterfaceArgs);



#endif


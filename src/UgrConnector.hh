/** @file   UgrConnector.hh
 * @brief  Base class that gives the functionalities of a dynamic, protocol-agnostic redirector
 * @author Fabrizio Furano
 * @date   Jul 2011
 */

#ifndef UGRCONNECTOR_HH
#define UGRCONNECTOR_HH


#include <boost/filesystem.hpp>

#include "SimpleDebug.hh"
#include "Config.hh"
#include "LocationInfo.hh"
#include "LocationInfoHandler.hh"
#include "HostsInfoHandler.hh"
#include "LocationPlugin.hh"
#include "ExtCacheHandler.hh"


#include <string>

class LocationPlugin;
class ExtCacheHandler;

/// The main class that allows to interact with the system
class UgrConnector{
private:
    /// ugr plugin directory path
    boost::filesystem::path plugin_dir;

    /// The thread that ticks
    boost::thread *ticker;

    /// A mutex that protects the initialization... sigh
    boost::mutex mtx;
protected:

    /// This holds in memory at least the entries that are being processed
    /// Eventually it may grow or demand a more scalable caching to an external entity
    /// This has to be mutex-protected
    LocationInfoHandler locHandler;

    /// This handles the information that we have about a host that participates to the thing
    /// Having no info about a host is technically allowed
    /// At app level, this will probably be forbidden
    /// This has to be mutex-protected
    HostsInfoHandler hostHandler;

    /// The location plugins that we have loaded
    /// E.g. SimpleHTTP, MultiHTTP, SEMsg, WhateverDB, WhateverMessaging
    /// Each plugin is able to modify the info in LocHandler and HostHandler, asynchronously
    /// When a location process is started, all the plugins are triggered in parallel
    std::vector<LocationPlugin *> locPlugins;

    /// The filter plugins that we have loaded
    /// Each filter plugin is applied in order by the operation filter()
    std::vector<FilterPlugin *> filterPlugins;

    /// The main instance of the cache handler
    /// The eventual responsibility of destroying it is of this class
    ExtCacheHandler extCache;
    
    /// Info needed for a simple implementation of n2n functionalities
    /// Prefix to substitute and new prefix
    std::string n2n_pfx, n2n_newpfx;
    void do_n2n(std::string &path);

    /// Start the async stat process
    /// In practice, trigger all the location plugins, possibly together,
    /// so they act concurrently
    int do_Stat(UgrFileInfo *fi);
    /// Waits max a number of seconds for a stat process to be complete
    int do_waitStat(UgrFileInfo *fi, int tmout = 30);

    /// Start the async location process
    /// In practice, trigger all the location plugins, possibly together,
    /// so they act concurrently
    int do_Locate(UgrFileInfo *fi);
    /// Waits max a number of seconds for a locate process to be complete
    int do_waitLocate(UgrFileInfo *fi, int tmout = 30);

    /// Start the async listing process
    /// In practice, trigger all the location plugins, possibly together,
    /// so they act concurrently
    int do_List(UgrFileInfo *fi);
    /// Waits max a number of seconds for a list process to be complete
    int do_waitList(UgrFileInfo *fi, int tmout = 30);

   
    /// Invoked by the ticker thread, gives life to the object
    virtual void tick(int parm);

    /// Helper func that starts a parallel stat task for all the subdirs of the given dir
    /// This reduces the latencies in processing a directory listing
    void statSubdirs(UgrFileInfo *fi);

    unsigned int ticktime;
    bool initdone;
public:

    UgrConnector() : ticker(0), ticktime(10), initdone(false) {
        const char *fname = "UgrConnector::ctor";
        Info(SimpleDebug::kLOW, fname, "Ctor");
    };

    virtual ~UgrConnector();

    Config & getConfig() const;

    SimpleDebug & getLogger() const;

    /// To be called after the ctor to initialize the object.
    /// @param cfgfile Path to the config file to be loaded
    int init(char *cfgfile = 0);
    bool resetinit() { initdone = false; return true; }
    
    // Tells us if the endpoint represented by the given pluginID is up or down
    bool isEndpointOK(int pluginID);

    /// Returns a pointer to the item with the list of the locations of the given lfn (ls).
    /// This could be a list of replicas
    /// Waits for some time that at least nitemswait items have arrived, ev returns TIMEOUT
    /// If the search process terminates and there are no (more) items, returns OK
    /// The nfo instance is returned in locked state, only with the purpose
    /// of copying the values out. The caller
    /// must release it as soon as possible
    /// @param lfn  The unique key to the file, typically its logical file name
    /// @param nfo  Gets a pointer to the updated instance of the UgrfileInfo related to lfn
    virtual int locate(std::string &lfn, UgrFileInfo **nfo);

    /// Returns a pointer to the item with the list of the content of the given lfn (ls).
    /// Waits for some time that at least nitemswait items have arrived, ev returns TIMEOUT
    /// If the search process terminates and there are no (more) items, returns OK
    /// The nfo instance is returned in locked state, only with the purpose
    /// of copying the values out. The caller
    /// must release it as soon as possible
    /// @param lfn  The unique key to the file, typically its logical file name
    /// @param nfo  Gets a pointer to the updated instance of the UgrfileInfo related to lfn
    /// @param nitemswait Wait for at least N items to have arrived. 0 to wait for the whole set
    virtual int list(std::string &lfn, UgrFileInfo **nfo, int nitemswait = 0);

    /// Returns a pointer to the item, after having made sure that
    /// its stat information was populated. Eventually populate it before returning.
    /// The nfo instance is returned in locked state, only with the purpose
    /// of copying the values out. The caller
    /// must release it as soon as possible
    /// @param lfn  The unique key to the file, typically its logical file name
    /// @param nfo  Gets a pointer to the updated instance of the UgrfileInfo related to lfn
    virtual int stat(std::string &lfn, UgrFileInfo **nfo);

    /// Start an async process that finds the endpoint that has the given replica
    /// There is no wait primitive associated to this, as the normal do_waitLocate will do
    int do_checkreplica(UgrFileInfo *fi, std::string rep);

    ///
    /// Apply configured filters on the replica list
    int filter(std::deque<UgrFileItem_replica> & replica);
    int filter(std::deque<UgrFileItem_replica> & replica, const UgrClientInfo & cli_info);
    
protected:
    // non copyable
    UgrConnector(const UgrConnector & u);
    UgrConnector & operator=(const UgrConnector & u);
};


#endif

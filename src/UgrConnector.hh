/** @file   UgrConnector.hh
 * @brief  Base class that gives the functionalities of a dynamic, protocol-agnostic redirector
 * @author Fabrizio Furano
 * @date   Jul 2011
 */

#ifndef UGRCONNECTOR_HH
#define UGRCONNECTOR_HH



#include "SimpleDebug.hh"
#include "Config.hh"
#include "LocationInfo.hh"
#include "LocationInfoHandler.hh"
#include "HostsInfoHandler.hh"
#include "LocationPlugin.hh"
#include "GeoPlugin.hh"


#include <string>



/// The main class that allows to interact with the system
class UgrConnector {
private:
    /// The thread that ticks
    boost::thread *ticker;

    /// A mutex that protects the initialization... sigh
    boost::mutex mtx;
protected:

    /// This is the currently loaded instance of a GeoPlugin, i.e. an object
    /// that gives GPS coordinates to file replicas
    GeoPlugin *geoPlugin;

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

    UgrConnector() : ticker(0), geoPlugin(0), ticktime(10), initdone(false) {
        const char *fname = "UgrConnector::ctor";
        Info(SimpleDebug::kLOW, fname, "Ctor");
    };

    virtual ~UgrConnector();

    /// To be called after the ctor to initialize the object.
    /// @param cfgfile Path to the config file to be loaded
    int init(char *cfgfile = 0);
    bool resetinit() { initdone = false; }

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

    /// Return the replica set sorted by increasing distance to the client IP given
    std::set<UgrFileItem_replica, UgrFileItemGeoComp> getGeoSortedReplicas(std::string clientip, UgrFileInfo *nfo);
};


#endif

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


/** @file   UgrConnector.hh
 * @brief  Base class that gives the functionalities of a dynamic, protocol-agnostic redirector
 * @author Fabrizio Furano
 * @date   Jul 2011
 */

#ifndef UGRCONNECTOR_HH
#define UGRCONNECTOR_HH

#include <string>
#include <functional>
#include <boost/filesystem.hpp>


#include "SimpleDebug.hh"
#include "Config.h"
#include "UgrConfig.hh"
#include "LocationInfo.hh"
#include "LocationInfoHandler.hh"
#include "HostsInfoHandler.hh"
#include "LocationPlugin.hh"
#include "ExtCacheHandler.hh"


class UgrAuthorization;
class UgrAuthorizationPlugin;

/// return the path of the UgrConnector shared library
const std::string & getUgrLibPath();

    
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
    /// Each filter plugin is applied in order by the operation filterAndSortReplicaList()
    std::vector<FilterPlugin *> filterPlugins;
    
    /// The authorization plugins that we have loaded
    /// All of them are applied looking for one that grants access
    std::vector<UgrAuthorizationPlugin *> authorizationPlugins;

    /// The main instance of the cache handler
    /// The eventual responsibility of destroying it is of this class
    ExtCacheHandler extCache;
    
    /// Info needed for a simple implementation of n2n functionalities
    /// Prefix to substitute and new prefix
    std::string n2n_newpfx;
    std::vector<std::string> n2n_pfx_v;
    
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

    UgrConnector();

    virtual ~UgrConnector();

    UgrConfig & getConfig() const;

    UgrLogger & getLogger() const;

    /// To be called after the ctor to initialize the object.
    /// @param cfgfile Path to the config file to be loaded
    int init(char *cfgfile = 0);
    bool resetinit() { initdone = false; return true; }
    
    // Tells us if the endpoint represented by the given pluginID is up or down
    bool isEndpointOK(int pluginID);
    
    // Tells us if the endpoint is able to calculate a checksum if a client
    // is redirected to it
    bool canEndpointDoChecksum(int pluginID);
    
    /// Returns a pointer to the item with the list of the locations of the given lfn (ls).
    /// This could be a list of replicas
    /// Waits for some time that at least nitemswait items have arrived, ev returns TIMEOUT
    /// If the search process terminates and there are no (more) items, returns OK
    /// The nfo instance is returned in locked state, only with the purpose
    /// of copying the values out. The caller
    /// must release it as soon as possible
    /// @param lfn  The unique key to the file, typically its logical file name
    /// @param client The credentials of the client to be eventually authorized
    /// @param nfo  Gets a pointer to the updated instance of the UgrfileInfo related to lfn
    virtual int locate(std::string &lfn, const UgrClientInfo & client, UgrFileInfo **nfo);

    /// Returns a pointer to the item with the list of the content of the given lfn (ls).
    /// Waits for some time that at least nitemswait items have arrived, ev returns TIMEOUT
    /// If the search process terminates and there are no (more) items, returns OK
    /// The nfo instance is returned in locked state, only with the purpose
    /// of copying the values out. The caller
    /// must release it as soon as possible
    /// @param lfn  The unique key to the file, typically its logical file name
    /// @param client The credentials of the client to be eventually authorized
    /// @param nfo  Gets a pointer to the updated instance of the UgrfileInfo related to lfn
    /// @param nitemswait Wait for at least N items to have arrived. 0 to wait for the whole set
    virtual int list(std::string &lfn, const UgrClientInfo & client, UgrFileInfo **nfo, int nitemswait = 0);

    /// Returns a pointer to the item, after having made sure that
    /// its stat information was populated. Eventually populate it before returning.
    /// The nfo instance is returned in locked state, only with the purpose
    /// of copying the values out. The caller
    /// must release it as soon as possible
    /// @param lfn  The unique key to the file, typically its logical file name
    /// @param client The credentials of the client to be eventually authorized
    /// @param nfo  Gets a pointer to the updated instance of the UgrfileInfo related to lfn
    virtual int stat(std::string &lfn, const UgrClientInfo & client, UgrFileInfo **nfo);


    /// Remove All replicates of a resource at the given location
    /// This function tries first to delete all replicate in a sychronous manner.
    /// in case of the impossibility to delete some of the known replicas, these are added
    /// to the replicas_to_delete vector
    ///
    /// @param lfn  The unique key to the file, typically its logical file name
    /// @param client The credentials of the client to be eventually authorized
    /// @param replicas_to_delete
    virtual UgrCode remove(const std::string & lfn, const UgrClientInfo & client, UgrReplicaVec & replicas_to_delete);


    /// Remove All replicates of a resource at the given location
    /// This function tries first to delete all replicate in a sychronous manner.
    /// in case of the impossibility to delete some of the known replicas, these are added
    /// to the replicas_to_delete vector
    ///
    /// @param lfn  The unique key to the directory, typically its logical file name
    /// @param client The credentials of the client to be eventually authorized
    /// @param replicas_to_delete
    virtual UgrCode removeDir(const std::string & lfn, const UgrClientInfo & client, UgrReplicaVec & replicas_to_delete);

    /// Create a directory in the federated namespace
    /// For subtle technical reasons, the right way for this func to work
    /// is just to insert a dir into the cached namespace
    ///
    /// @param lfn  The unique key to the directory, typically its logical file name
    /// @param client The credentials of the client to be eventually authorized
    /// @param replicas_to_delete
    virtual UgrCode makeDir(const std::string & lfn, const UgrClientInfo & client);

    /// Return a list of locations for a new resource
    /// The location is selected based on client credential, client location, endpoints configuration, file size
    /// @param new_lfn : lfn of the new resource
    /// @param client: client informations
    /// @param new_locations: vector of possible replicas
    virtual UgrCode findNewLocation(const std::string & new_lfn, off64_t filesz, const UgrClientInfo & client, UgrReplicaVec & new_locations);

    
    
    /// Create a subdirectory and all its parents on all the plugins that
    /// match the given sitefn (usually just one)
    /// NOTE: this function takes a site-filename, NOT a logical federated filename
    ///
    /// @param sitefn : the url of a remote replica
    /// @param client: client informations
    /// @param new_locations: vector of possible replicas
    virtual UgrCode mkDirMinusPonSiteFN(const std::string & sitefn);
    
    
        
    /// Checks the permissions to do a operation
    /// Returns 0 if accepted
    /// @param fname the original function name that was invoked (for pretty logging purposes)
    /// @param clientName Authentication info about the client asking for access
    /// @param remoteAddress Authentication info about the client asking for access
    /// @param fqans Authentication info about the client asking for access
    /// @param reqresource the path or filename in question
    /// @param reqmode r, w, l ... the mode
    int checkperm(const char *fname,
                          const std::string &clientName,
                          const std::string &remoteAddress,
                          const std::vector<std::string> &fqans,
                          const std::vector< std::pair<std::string, std::string> > &keys,
                          char *reqresource, char reqmode);

    /// Start an async process that finds the endpoint that has the given replica
    /// There is no wait primitive associated to this, as the normal do_waitLocate will do
    int do_checkreplica(UgrFileInfo *fi, std::string rep);


    ///
    /// Use the FilterPlugin stack to filter and sort a list of Replicas based on arbitrary criteria
    ///
    /// The criterias can be geographical position (FilterPlugin: GeoPlugin), loopDetection (FilterPlugin: noLoop),
    /// replicas Status (Default: Checker)
    int filterAndSortReplicaList(UgrReplicaVec & replica, const UgrClientInfo & cli_info);

    /// Utility to filter replicas on systems without enough free space
    void filterFull(UgrReplicaVec &);

    //
    // internal usage only
    void applyHooksNewReplica(UgrFileItem_replica & rep);
    
protected:
    // non copyable
    UgrConnector(const UgrConnector & u);
    UgrConnector & operator=(const UgrConnector & u);
};


#endif

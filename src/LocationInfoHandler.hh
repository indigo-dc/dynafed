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

/** @file   LocationInfoHandler.hh
 * @brief  Handling of the info that is kept per each file
 * @author Fabrizio Furano
 * @date   Oct 2011
 */

#ifndef LOCATIONINFOHANDLER_HH
#define LOCATIONINFOHANDLER_HH

#include "UgrConfig.hh"
#include "ExtCacheHandler.hh"
#include "LocationInfo.hh"

#include <string>
#include <map>


#ifdef HAVE_ATOMIC
#include <atomic>

typedef std::atomic<int> IntAtomic;
#endif

#ifdef HAVE_CSTDATOMIC
#include <cstdatomic>

typedef atomic<int> IntAtomic;
#endif

#include <boost/thread.hpp>
#include <boost/bimap.hpp>



/// This class acts like a repository of file locations/information that has to
/// be gathered and synchronized on the fly. Given the intrinsic nature of async buffer,
/// it may act as a first level cache, thus implementing a insert/purge mechanism.
/// It may also interface with a secondary (big) cache, thus adding a retrieve mechanism
/// These behaviors are implemented in the Tick() method, that has to be called at a
/// regular pace, to give life to this object

class LocationInfoHandler : public boost::recursive_mutex {
private:
    /// Counter for implementing a LRU buffer
    unsigned long long lrutick;
    /// Max number of items allowed
    unsigned long maxitems;
    /// Max life for an item that was not recently accessed
    unsigned int maxttl;
    /// Max life for an item that even if it was recently accessed
    unsigned int maxmaxttl;
    /// Max life for a NEGATIVE item (e.g. a not found) that was not recently accessed
    unsigned int maxttl_negative;

    /// A simple implementation of an lru queue, based on a bimap
    typedef boost::bimap< time_t, std::string > lrudatarepo;
    /// A simple implementation of an lru queue, based on a bimap
    typedef lrudatarepo::value_type lrudataitem;
    /// A simple implementation of an lru queue, based on a bimap
    lrudatarepo lrudata;

    /// The information repo itself. Basically a map key->UgrFileInfo*
    /// where the key is generally a LFN. For us it's just a string key, no assumption is made on it.
    std::map< std::string, UgrFileInfo * > data;


    /// An external cache
    ExtCacheHandler *extcache;

    /// Purge an item from the buffer, to make space
    int purgeLRUitem();

    /// Purge the old items from the buffer
    void purgeExpired();



public:

    LocationInfoHandler() : lrutick(0) {
        extcache = 0;
    };

    void Init(ExtCacheHandler *cache) {
        // Get the max capacity from the config
        maxitems = UgrCFG->GetLong("infohandler.maxitems", 1000000);
        // Get the lifetime of an entry after the last reference
        maxttl = UgrCFG->GetLong("infohandler.itemttl", 3600);
        // Get the maximum allowed lifetime of an entry
        maxmaxttl = UgrCFG->GetLong("infohandler.itemmaxttl", 86400);
        maxttl_negative = UgrCFG->GetLong("infohandler.itemttl_negative", 10);
       
        if (UgrCFG->GetBool("infohandler.useextcache", true)) {
            Info(UgrLogger::Lvl1, "LocationInfoHandler::Init", "Setting the ExtCacheHandler");
            extcache = cache;
        }

    }


    //
    // Helper primitives to operate on the list of file locations
    //

    /// Get a pointer to a FileInfo, or create a new one, marked as pending
    UgrFileInfo *getFileInfoOrCreateNewOne(UgrConnector & context, std::string &lfn, bool docachelookup=true, bool docachesubitemslookup=false);

    /// Get a pointer to a FileInfo if it exists in either 1st of 2nd level cache, then update its subitems list with child item
    int addChildToParentSubitem(UgrConnector& context, std::string &lfn, bool checkExtCache, bool recursive);

    /// Do your best to wipe the information about an lfn
    /// This cannot influence the 1st level of any other process running Ugr
    /// yet we can propagate the wiped entry to the 2nd level
    int wipeInfoOnLfn(UgrConnector& context, std::string &lfn);
		
    // Ext Cache in/out
    int getFileInfoFromCache(UgrFileInfo *fi);
    int getSubitemsFromCache(UgrFileInfo *fi);
    int putFileInfoToCache(UgrFileInfo *fi);
    int putSubitemsToCache(UgrFileInfo *fi);

    /// Gives life to this obj
    void tick();
};

///
/// \brief The HandlerTraits class
///
/// Traits for asynchronous completion handler
///
class HandlerTraits{
public:
    HandlerTraits(): counter_(0){}


    // add N worker for completion
    inline void addWorker(int n){
        counter_ += n;
    }

    inline void incWorker(){
        ++counter_;
    }


    // remove one worker from the completion
    inline void decWorker(){
        --counter_;
        if(counter_.load() <=0){
            sig_.notify_one();
        }
    }

    // wait for the completion handler
    inline bool wait(unsigned int duration){
        using namespace boost;
        if(counter_.load() > 0){

            system_time const deadline = get_system_time() + posix_time::seconds(duration);

            unique_lock<mutex> l(mut_sig_);
            return sig_.timed_wait(l, deadline);
        }
        return true;
    }

private:
    IntAtomic counter_;
    boost::condition_variable sig_;
    boost::mutex mut_sig_;

};


///
/// @brief ReplicasHandler class
///
/// Handler for operations on a list of replicas
///
class ReplicasHandler : public HandlerTraits, public boost::noncopyable{
public:
    // If a plugin understands the content of this field, it may
    // decide to insert enough items to be used for a multipart upload
    std::string s3uploadID;
    off_t filesize;
    int64_t nchunks;
    
    inline void addReplica(const std::string & name, int pluginID){
        UgrFileItem_replica r;
        r.pluginID = pluginID;
        r.name = name;
        
        {
            boost::lock_guard<boost::mutex> l(mu_);
            new_locations_vec_.push_back(std::move(r));
        }
    }

    inline void addReplica(const std::string & name, const std::string & alturl, int pluginID){
      UgrFileItem_replica r;
      r.pluginID = pluginID;
      r.name = name;
      r.alternativeUrl = alturl;
      {
        boost::lock_guard<boost::mutex> l(mu_);
        new_locations_vec_.push_back(std::move(r));
      }
    }
    
    inline void addReplica(const UgrFileItem_replica & rep, int pluginID){
            boost::lock_guard<boost::mutex> l(mu_);
            new_locations_vec_.push_back(rep);
            new_locations_vec_.back().pluginID = pluginID;
    }

    inline UgrReplicaVec takeAll(){
        UgrReplicaVec res;
        {
            boost::lock_guard<boost::mutex> l(mu_);
            res.swap(new_locations_vec_);
        }
        return res;
    }

    inline int size() {
        boost::lock_guard<boost::mutex> l(mu_);
        return new_locations_vec_.size();
    }
      
private:
    boost::mutex mu_;
    UgrReplicaVec new_locations_vec_;

};


///
/// \brief NewLocationHandler
///
///  Event Handler for a request about new location query (findNewLocation)
typedef ReplicasHandler NewLocationHandler;

///
/// \brief DeleteHandler
///
///  Even Handler for a request about delete replicats
typedef ReplicasHandler DeleteReplicaHandler;

#endif


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

/** @file   LocationInfo.hh
 * @brief  Handling of the info that is kept per each file
 * @author Fabrizio Furano
 * @date   Jul 2011
 */
#ifndef LOCATIONINFO_HH
#define LOCATIONINFO_HH


#include "Config.h"
#include "UgrConfig.hh"

#include <string>
#include <set>
#include <boost/thread.hpp>

#include "SimpleDebug.hh"
#include <boost/date_time/posix_time/posix_time.hpp>
#include <sys/stat.h>

#include <iostream>


class UgrConnector;


///
/// @brief The UgrClientInfo, container about client information
////
class UgrClientInfo{
public:
    UgrClientInfo(): s3uploadpluginid(-1) { };
    UgrClientInfo(const std::string & ip1) : ip(ip1), s3uploadpluginid(-1) {  }
    UgrClientInfo(const std::string & ip1, std::vector<std::string> & grps) : groups(grps),  ip(ip1), s3uploadpluginid(-1) { }

    std::vector<std::string> groups;
    std::string ip;
    
    // The client may have an upload ID coming from S3
    std::string s3uploadid;
    // The client may want to restrict the search to just one plugin
    int s3uploadpluginid;
};

/// The information that we have about a subitem, e.g. an item in a directory listing,
/// or an item in a list of replicas. Maybe a file or directory
/// If the owner is a directory then this is a (relative) item of its listing
/// If the owner is a file, this is the info about one of its replicas (full path)
/// This can be used to build the full name of a file and fetch its info separately
class UgrFileItem {
public:

    UgrFileItem() {}
    UgrFileItem(const UgrFileItem & origin) :
        name(origin.name),
        location(origin.location){}
    
    // The item's name
    std::string name;

    // Some info about the location, e.g. galactic coordinates
    std::string location;
    
};

class UgrFileItem_replica: public UgrFileItem {
public: 


    enum Status{
        Available=0,
        Deleted,
        PermissionDenied,
    };

    UgrFileItem_replica(): UgrFileItem(), status(Available), latitude(0.0), longitude(0.0), tempDistance(0.0) {
        pluginID = -1;
    };

    /// Some info about the location, e.g. galactic coordinates
    std::string location;

    /// Status Information
    Status status;
    
    /// Some info about the geographical location
    float latitude;
    float longitude;
    
    /// The index of the plugin that inserted this replica
    short pluginID;
    
    /// To help saving memory while doing geo distance calculations
    float tempDistance;
    
    /// Alternative URL to reach the replica. In the case of an S3 PUT this will be a signed URL
    /// where to send a POST request
    std::string alternativeUrl;
};


/// Vector of Replicas
typedef std::deque<UgrFileItem_replica> UgrReplicaVec;




/// Instances of UgrFileItem may be kept in a quasi-sorted way.
/// This is the compare functor that keeps them sorted by name
class UgrFileItemComp {
public:
	virtual ~UgrFileItemComp(){};
	
    virtual bool operator()(UgrFileItem s1, UgrFileItem s2) {
        if (s1.name < s2.name)
            return true;
        else
            return false;
    }
};




/// Defines the information that is kept about a file
/// Note that this object is lockable. It's supposed to be locked (if necessary)
/// from outside its own code only
class UgrFileInfo : public boost::mutex {
protected:
    /// Threads waiting for result about this file will wait and synchronize here
    boost::condition_variable condvar;
public:

    /// Ctor
    /// @param lfn The string key that univocally identifies a file or dir
    UgrFileInfo(UgrConnector & c, std::string &lfn);
    /// global content
    UgrConnector & context;

    /// Indicates that this entry was modified since the last time it was
    /// pushed to an eventual external caching thing
    /// This flag has to be set to true by any plugin that modifies
    /// the content of the object
    /// The purpose of this is to be able to implement write-through caching
    bool dirty;
    bool dirtyitems;
    
    /// Indicates that this entry cannot be purged from the 1st level cache
    /// because it is in temporary use.
    /// The typical usage of this is between opendir/closedir
    int pinned;
    
    void pin() { pinned++; };
    void unpin() { if (pinned > 0) pinned--; };
    int ispinned() { return pinned; };
    
    /// The filename this record refers to (the lfn)
    std::string name;


    enum InfoStatus {
        Error = -2,
        NoInfo,
        Ok, // 0
        NotFound,
        InProgress
    };

    /// Status of this object with respect to the pending stat operations
    /// carried on by plugins
    /// If a request is in progress, the responsibility of
    /// performing it is of the plugin(s) that are treating it.
    InfoStatus status_statinfo;

    /// Status of this object with respect to the pending locate operations
    /// carried on by plugins
    /// If a request is in progress, the responsibility of
    /// performing it is of the plugin(s) that are treating it.
    InfoStatus status_locations;

    /// Status of this object with respect to the pending list operations
    /// carried on by plugins
    /// If a request is in progress, the responsibility of
    /// performing it is of the plugin(s) that are treating it.
    InfoStatus status_items;

    /// Count of the plugins that are gathering stat info
    /// for an instance
    int pending_statinfo;

    /// Count of the plugins that are gathering locate info
    /// for an instance
    int pending_locations;

    /// Count of the plugins that are gathering list info
    /// for an instance
    int pending_items;

    /// Tells if the stat information is ready
    InfoStatus getStatStatus() {
        // To stat successfully a file we just need one plugin to answer positively
        // Hence, if we have the stat info here, we just return Ok, regardless
        // of how many plugins are still active
        if (!status_statinfo) return Ok;

        // If we have no stat info, then the file was not found or it's early to tell
        // Hence the info is inprogress if there are plugins that are still active
        if (pending_statinfo > 0) return InProgress;


        return status_statinfo;
    }

    /// Tells if the locate information is ready
    InfoStatus getLocationStatus() {
        // In the case of a pending op that tries to find all the locations, we need to
        // get the response from all the plugins
        // Hence, this info is inprogress if there are still plugins that are active on it
        //
        // As an addition, we also add the possibility of waiting for less plugins, in order to
        // be a bit fuzzy
      if (pending_locations > UgrCFG->GetLong("glb.waitlesslocations", 0)) return InProgress;

        if (status_locations == Ok) return Ok;
        return status_locations;
    }

    /// Tells if the list information is ready
    InfoStatus getItemsStatus() {
        if (pending_items > 0) return InProgress;

        if (status_items == Ok) return Ok;
        return status_items;
    }

    /// Builds a summary of the status
    /// If some info part is pending then the whole thing is pending
    InfoStatus getInfoStatus() {
        if ((pending_statinfo > 0) ||
                (pending_locations > 0) ||
                (pending_items > 0))
            return InProgress;

        if ((status_statinfo == Ok) ||
                (status_locations == Ok) ||
                (status_items == Ok))
            return Ok;

        if ((status_statinfo == NotFound) ||
                (status_locations == NotFound) ||
                (status_items == NotFound))
            return NotFound;

        return NoInfo;
    };

    /// Called by plugins when they start a search, to add 1 to the pending counter
    void notifyStatPending() {
        if (pending_statinfo >= 0) {
            // Set the file status to pending with respect to the stat op
            pending_statinfo++;
        }
    }

    /// Called by plugins when they end a search, to subtract 1 from the pending counter
    /// and wake up the clients that are waiting for something to happen to a file info
    void notifyStatNotPending() {
        const char *fname = "UgrFileInfo::notifyStatNotPending";
        if (pending_statinfo > 0) {
            // Decrease the pending count with respect to the stat op
            pending_statinfo--;
        } else
            Error(fname, "The fileinfo seemed not to be pending?!?");

        signalSomeUpdate();
    }


    // Called by plugins when they start a search, to add 1 to the pending counter
    void notifyLocationPending() {
        if (pending_locations >= 0) {
            // Set the file status to pending with respect to the stat op
            pending_locations++;
        }
    }

    /// Called by plugins when they end a search, to subtract 1 from the pending counter
    /// and wake up the clients that are waiting for something to happen to a file info
    void notifyLocationNotPending() {
        const char *fname = "UgrFileInfo::notifyLocationNotPending";
        if (pending_locations > 0) {
            // Decrease the pending count with respect to the stat op
            pending_locations--;
        } else
            Error(fname, "The fileinfo seemed not to be pending?!?");

        signalSomeUpdate();
    }

    /// Called by plugins when they start a search, to add 1 to the pending counter
    void notifyItemsPending() {
        if (pending_items >= 0) {
            // Set the file status to pending with respect to the stat op
            pending_items++;
        }
    }

    /// Called by plugins when they end a search, to subtract 1 from the pending counter
    /// and wake up the clients that are waiting for something to happen to a file info
    void notifyItemsNotPending() {
        const char *fname = "UgrFileInfo::notifyItemsNotPending";
        if (pending_items > 0) {
            // Decrease the pending count with respect to the stat op
            pending_items--;
        } else
            Error(fname, "The fileinfo seemed not to be pending?!?");

        signalSomeUpdate();
    }


    /// The unix flags of this file entry
    int unixflags;

    /// The size of this file entry
    long long size;

    /// The owner of this file entry (if applicable)
    std::string owner;

    /// The group of this file entry (if applicable)
    std::string group;

    /// The subitems of this file entry, i.e. its subdirectories
    /// Just the keys, in a small structure
    std::set<UgrFileItem, UgrFileItemComp> subdirs;
    
    /// The list of the replicas (if this is a file)
    /// Just the keys, in a small structure
    std::set<UgrFileItem_replica, UgrFileItemComp> replicas;

    /// The list of the plugins that inserted this entry
    std::set<short> ownerpluginIDs;
    
    /// Helper to notify which plugin(s) this info comes from
    void setPluginID(const short pluginID, bool dolock = true);
    
    /// The last time there was a request to gather info about this entry
    time_t lastupdreqtime;

    /// The last time there was an update to this entry
    time_t lastupdtime;

    /// The last time this entry was referenced
    time_t lastreftime;

    /// The various unix times
    time_t atime, mtime, ctime;

    /// We will like to be able to encode this info to a string, e.g. for external caching purposes
    int encodeToString(std::string &str);

    /// We will like to be able to encode this info to a string, e.g. for external caching purposes
    int decode(void *data, int sz);

    /// We will like to be able to encode this info to a string, e.g. for external caching purposes
    int encodeSubitemsToString(std::string &str);

    /// We will like to be able to encode this info to a string, e.g. for external caching purposes
    int decodeSubitems(void *data, int sz);

    /// Selects the replica that looks best for the given client. Here we don't make assumptions
    /// on the method that we apply to choose one, since it could be implemented as a plugin
    int getBestReplicaIdx(std::string &clientlocation);

    /// Update last reference time
    void touch() {
        // only update reference time if the entry exist, otherwise it may be stuck in internal cache 
        // until max ttl expires
        if(getInfoStatus() == UgrFileInfo::NotFound)
            return;
        lastreftime = time(0);
    }

    /// Wait until any notification update comes
    /// Useful to recheck if what came is what we were waiting for
    /// 0 if notif received, nonzero if tmout
    int waitForSomeUpdate(boost::unique_lock<boost::mutex> &l, int sectmout);

    /// Wait for the stat info to be available
    /// @param l lock to be held
    /// @param sectmout Wait timeout in seconds
    int waitStat(boost::unique_lock<boost::mutex> &l, int sectmout);

    /// Wait for the locate info to be available
    /// @param l lock to be held
    /// @param sectmout Wait timeout in seconds
    int waitLocations(boost::unique_lock<boost::mutex> &l, int sectmout);

    /// Wait for the list info to be available
    /// @param l lock to be held
    /// @param sectmout Wait timeout in seconds
    int waitItems(boost::unique_lock<boost::mutex> &l, int sectmout);

    /// Signal that something changed here
    int signalSomeUpdate();


    // Useful for debugging
    void print(std::ostream &out);
  
    /// Utility that fixes a path
    /// @param s the string to be treated like a path to fix
    static void trimpath(std::string &s);

    /// Clear an object, so that it seems that we know nothing about it.
    /// Useful instead of removing the item from the cache (which is deprecated)
    void setToNoInfo();
    
    /// Fill the fields from a stat struct
    /// @param st the stat struct to copy fields from
    void takeStat(const struct stat &st);

    /// Add a replica to the replicas list
    /// @params replica struct to add
    void addReplica( const UgrFileItem_replica & replica);

    /// Get All replicas into a list
    void getReplicaList( UgrReplicaVec& reps);
};




#endif


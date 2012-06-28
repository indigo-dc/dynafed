/** @file   LocationInfo.hh
 * @brief  Handling of the info that is kept per each file
 * @author Fabrizio Furano
 * @date   Jul 2011
 */
#ifndef LOCATIONINFO_HH
#define LOCATIONINFO_HH


#include "Config.hh"

#include <string>
#include <set>
#include <boost/thread.hpp>
#include "SimpleDebug.hh"
#include <boost/date_time/posix_time/posix_time.hpp>
#include <sys/stat.h>
#include "dmlite/common/dm_types.h"

#include <iostream>

/// The information that we have about a subitem, e.g. an item in a directory listing,
/// or an item in a list of replicas. Maybe a file or directory
/// If the owner is a directory then this is a (relative) item of its listing
/// If the owner is a file, this is the info about one of its replicas (full path)
/// This can be used to build the full name of a file and fetch its info separately
class UgrFileItem {
public:

    UgrFileItem(): latitude(0.0), longitude(0.0) {};
    
    // The item's name
    std::string name;

    // Some info about the location, e.g. galactic coordinates
    std::string location;

    // Some info about the geographical location
    float latitude;
    float longitude;
};

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

/// Instances of UgrFileItem may be kept in a quasi-sorted way.
/// This is the compare functor that sorts them by distance from a point
class UgrFileItemGeoComp {
private:
    float ltt, lng;
public:

    UgrFileItemGeoComp(float latitude, float longitude): ltt(latitude), lng(longitude) {
        //std::cout << "geocomp" << std::endl;
    };
    virtual ~UgrFileItemGeoComp(){};
    
    virtual bool operator()(const UgrFileItem &s1, const UgrFileItem &s2) {
        float x, y, d1, d2;

        //std::cout << "client" << ltt << " " << lng << std::endl;

        // Distance client->repl1
        x = (s1.longitude-lng) * cos( (ltt+s1.latitude)/2 );
        y = (s1.latitude-ltt);
        d1 = x*x + y*y;

        //std::cout << "d1 " << d1 << std::endl;

        // Distance client->repl2
        x = (s2.longitude-lng) * cos( (ltt+s2.latitude)/2 );
        y = (s2.latitude-ltt);
        d2 = x*x + y*y;

        //std::cout << "d2 " << d2 << std::endl;


        if (d1 < d2)
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
    UgrFileInfo(std::string &lfn) {
        status_statinfo = NoInfo;
        status_locations = NoInfo;
        status_items = NoInfo;
        pending_statinfo = 0;
        pending_locations = 0;
        pending_items = 0;
        name = lfn;

        lastupdtime = time(0);
        lastupdreqtime = time(0);

        atime = mtime = ctime = 0;

        
    }

    /// Indicates that this entry was modified since the last time it was
    /// pushed to an eventual external caching thing
    /// This flag has to be set to true by any plugin that modifies
    /// the content of the object
    /// The purpose of this is to be able to implement write-through caching
    bool dirty;
    bool dirtyitems;
    
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
        if (pending_locations > 0) return InProgress;

        if (status_locations == Ok) return Ok;
        return status_locations;
    }

    /// Tells if the locate information is ready
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

    /// The subitems of this file entry
    /// The list of the replicas (if this is a file), eventually partial if in pending state
    /// Just the keys, in a small structure
    std::set<UgrFileItem, UgrFileItemComp> subitems;

    /// The last time there was a request to gather info about this entry
    time_t lastupdreqtime;

    /// The last time there was an update to this entry
    time_t lastupdtime;


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

    void takeStat(ExtendedStat &st);

};



#endif


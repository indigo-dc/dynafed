/** @file   LocationInfoHandler.hh
 * @brief  Handling of the info that is kept per each file
 * @author Fabrizio Furano
 * @date   Oct 2011
 */

#ifndef LOCATIONINFOHANDLER_HH
#define LOCATIONINFOHANDLER_HH


#include "Config.hh"
#include "LocationInfo.hh"

#include <string>
#include <map>
#include <boost/thread.hpp>
#include <boost/bimap.hpp>

class ExtCacheHandler;


/// This class acts like a repository of file locations/information that has to
/// be gathered and synchronized on the fly. Given the intrinsic nature of async buffer,
/// it may act as a first level cache, thus implementing a insert/purge mechanism.
/// It may also interface with a secondary (big) cache, thus adding a retrieve mechanism
/// These behaviors are implemented in the Tick() method, that has to be called at a
/// regular pace, to give life to this object

class LocationInfoHandler : public boost::mutex {
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
        maxitems = CFG->GetLong("infohandler.maxitems", 1000000);
        // Get the lifetime of an entry after the last reference
        maxttl = CFG->GetLong("infohandler.itemttl", 3600);
        // Get the maximum allowed lifetime of an entry
        maxmaxttl = CFG->GetLong("infohandler.itemmaxttl", 86400);
        maxttl_negative = CFG->GetLong("infohandler.itemttl_negative", 10);
       
        if (CFG->GetBool("infohandler.useextcache", true)) {
            Info(SimpleDebug::kLOW, "LocationInfoHandler::Init", "Setting the ExtCacheHandler");
            extcache = cache;
        }

    }


    //
    // Helper primitives to operate on the list of file locations
    //

    /// Get a pointer to a FileInfo, or create a new one, marked as pending
    UgrFileInfo *getFileInfoOrCreateNewOne(std::string &lfn, bool docachelookup=true, bool docachesubitemslookup=false);

    // Ext Cache in/out
    int getFileInfoFromCache(UgrFileInfo *fi);
    int getSubitemsFromCache(UgrFileInfo *fi);
    int putFileInfoToCache(UgrFileInfo *fi);
    int putSubitemsToCache(UgrFileInfo *fi);

    /// Gives life to this obj
    void tick();
};



#endif


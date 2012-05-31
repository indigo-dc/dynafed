#ifndef LOCATIONINFOHANDLER_HH
#define LOCATIONINFOHANDLER_HH

/* LocationInfoHandler
 * Handling of the info that is kept per each file
 *
 *
 * by Fabrizio Furano, CERN, Oct 2011
 */

#include "Config.hh"
#include "LocationInfo.hh"

#include <string>
#include <map>
#include <boost/thread.hpp>
#include <boost/bimap.hpp>


// This class acts like a repository of file locations/information
// It may act as a first level cache, thus implementing a insert/purge mechanism
// It may also interface with a secondary (big) cache, thus adding a retrieve mechanism
// These behaviors are implemented in the Tick() method, that has to be called at a
// regular pace

class LocationInfoHandler : public boost::mutex {
private:
    unsigned long long lrutick;
    unsigned long maxitems;
    unsigned int maxttl;

    // A simple implementation of an lru queue
    // To say the least, I don't like this.
    typedef boost::bimap< time_t, std::string > lrudatarepo;
    typedef lrudatarepo::value_type lrudataitem;
    lrudatarepo lrudata;

    // The repo itself. Basically a map key->UgrFileInfo
    // where the key is generally a LFN
    std::map< std::string, UgrFileInfo * > data;

    // Purge an item from the buffer, to make space
    void purgeLRUitem();

    // Purge the old items from the buffer
    void purgeExpired();

    // Cache in/out
    int getFileInfoFromCache(UgrFileInfo *fi) { return 0; };
    int putFileInfoToCache(UgrFileInfo *fi) { return 0; };

public:

    LocationInfoHandler() : lrutick(0) {

        // Get the max capacity from the config
        maxitems = CFG->GetLong("infohandler.maxitems", 1000000);

        

        // Get the lifetime of an etry
        maxttl = CFG->GetLong("infohandler.itemttl", 86400);
    };


    //
    // Helper primitives to operate on the list of file locations
    //

    // Get a pointer to a FileInfo, or create a new, pending one
    UgrFileInfo *getFileInfoOrCreateNewOne(std::string &lfn);
    
    // Gives life to this obj
    void tick();
};



#endif


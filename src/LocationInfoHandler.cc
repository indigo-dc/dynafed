/* LocationInfoHandler
 * Handling of the info that is kept per each file
 *
 *
 * by Fabrizio Furano, CERN, Oct 2011
 */


#include "LocationInfoHandler.hh"

using namespace boost;

// Get a pointer to a FileInfo, or create a new one

UgrFileInfo *LocationInfoHandler::getFileInfoOrCreateNewOne(std::string &lfn) {
    const char *fname = "LocationInfoHandler::getFileInfoOrCreateNewOne";

    boost::lock_guard<LocationInfoHandler> l(*this);

    std::map< std::string, UgrFileInfo *>::iterator p;

    p = data.find(lfn);
    if (p == data.end()) {

        // No item with the given key could be found in the buffer
        // An external/additional caching lookup would have to be hooked here

        // If we reached the max number of items, delete one
        if (data.size() > maxitems) purgeLRUitem();

        // If we still have no space, try to garbage collect the old items
        if (data.size() > maxitems) {
            Info(SimpleDebug::kLOW, fname, "Too many items, running garbage collection...");
            purgeExpired();
        }

        // If we still have no space, complain and do it anyway.
        if (data.size() > maxitems) {
            Error(fname, "Maximum capacity exceeded.");
        }


        // Create a new item
        UgrFileInfo *fi = new UgrFileInfo(lfn);
        data[lfn] = fi;
        lrudata.insert(lrudataitem(++lrutick, lfn));

        return fi;
    } else {
        // Promote the element to being the most recently used
        lrudata.right.erase(lfn);
        lrudata.insert(lrudataitem(++lrutick, lfn));
        return p->second;
    }

}

// Purge from the local workspace the least recently used element
void LocationInfoHandler::purgeLRUitem() {

    // Take the key of the lru item
    std::string s = lrudata.left.begin()->second;

    
    // Purge it
    lrudata.right.erase(s);
    // Remove it from the lru list
    UgrFileInfo *fi = data[s];
    // Remove the item from the map
    data.erase(s);

    // Delete it, eventually sending it to a 2nd level cache before
    delete fi;
}

// Purge the items that were not touched since a longer time
void LocationInfoHandler::purgeExpired() {
    const char *fname = "LocationInfoHandler::purgeExpired";
    int d = 0;
    time_t timelimit = time(0)-maxttl;

    for ( std::map< std::string, UgrFileInfo * >::iterator i=data.begin();
            i != data.end(); i++)
        if (i->second->lastupdtime < timelimit) {
            // The item is old...
            UgrFileInfo *fi = i->second;
            lrudata.right.erase(i->first);
            data.erase(i);
            delete(fi);
            d++;
    }

    if (d > 0)
        Info(SimpleDebug::kLOW, fname, "purged " << d << " expired items.");
}



void LocationInfoHandler::tick() {
    const char *fname = "LocationInfoHandler::tick";
    Info(SimpleDebug::kHIGHEST, fname, "tick...");
    
    boost::lock_guard<LocationInfoHandler> l(*this);

    purgeExpired();

}


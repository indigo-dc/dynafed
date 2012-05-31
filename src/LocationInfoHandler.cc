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

    boost::lock_guard<LocationInfoHandler> l(*this);

    std::map< std::string, UgrFileInfo *>::iterator p;

    p = data.find(lfn);
    if (p == data.end()) {

        // An external/additional caching would have to be hooked here

        UgrFileInfo *fi = new UgrFileInfo(lfn);
        data[lfn] = fi;
        lrudata.insert(lrudataitem(++lrutick, lfn));

        return fi;
    } else {
        // Promote the element to being the last used
        lrudata.right.erase(lfn);
        lrudata.insert(lrudataitem(++lrutick, lfn));
        return p->second;
    }

}

// Purge from the local workspace the least recently used element
void LocationInfoHandler::deleteLRUitem() {
    boost::lock_guard<LocationInfoHandler> l(*this);

    // Take the lru item from the back of the deque
    std::string s = lrudata.left.begin()->second;

    // Purge it if it is too old
    lrudata.right.erase(s);
    // Remove it from the lru list
    UgrFileInfo *fi = data[s];
    // Remove the item from the map
    data.erase(s);
    // Delete it
    delete fi;
}

void LocationInfoHandler::tick() {

    boost::lock_guard<LocationInfoHandler> l(*this);

    // Purge from the local workspace the least recently used element

    // Take the lru item from the back of the deque
    std::string s = lrudata.left.begin()->second;

    // Purge it if it is too old
    lrudata.right.erase(s);
    // Remove it from the lru list
    UgrFileInfo *fi = data[s];
    // Remove the item from the map
    data.erase(s);
    // Delete it
    delete fi;
}


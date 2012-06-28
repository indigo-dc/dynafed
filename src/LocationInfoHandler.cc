/** @file   LocationInfoHandler.cc
 * @brief  Handling of the info that is kept per each file
 * @author Fabrizio Furano
 * @date   Oct 2011
 */


#include "LocationInfoHandler.hh"
#include "ExtCacheHandler.hh"

using namespace boost;

// Get a pointer to a FileInfo, or create a new one

UgrFileInfo *LocationInfoHandler::getFileInfoOrCreateNewOne(std::string &lfn, bool docachelookup, bool docachesubitemslookup) {
    const char *fname = "LocationInfoHandler::getFileInfoOrCreateNewOne";
    bool dofetch = false;
    UgrFileInfo *fi = 0;

    {
        boost::lock_guard<LocationInfoHandler> l(*this);

        std::map< std::string, UgrFileInfo *>::iterator p;

        p = data.find(lfn);
        if (p == data.end()) {

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
            fi = new UgrFileInfo(lfn);

            if (docachelookup) {
                // We want to see if it's available in the external cache
                // hance, make it pending
                dofetch = true;

                // We don't need to lock here, as we are the only holders
                // Set this object as pending, as we'll try to fetch it from an external cache (if any)
                if (docachesubitemslookup) {
                    fi->notifyItemsPending();
                    fi->notifyLocationPending();
                }
                fi->notifyStatPending();
            }

            data[lfn] = fi;
            lrudata.insert(lrudataitem(++lrutick, lfn));

        } else {
            // Promote the element to being the most recently used
            lrudata.right.erase(lfn);
            lrudata.insert(lrudataitem(++lrutick, lfn));
            fi = p->second;
            if (docachesubitemslookup) {
                fi->notifyItemsPending();
                fi->notifyLocationPending();
            }
        }
    }

    if (dofetch) {

        // While we get it from the cache, the object is pending, and nothing is locked

        // Get the basic fields of the object, size, etc.
        getFileInfoFromCache(fi);
    }

    // Now see if we need to lookup the subitems.
    // The element must exist, with no subitems so far
    if (docachesubitemslookup) {

        // If necessary, get also the subitems from the cache
        // Maybe it's a good idea to store in the ext cache only
        // replica information, not file listings
        getSubitemsFromCache(fi);
    }

    // Found or not, the cache lookup for this object has ended
    // so it is marked as not pending
    {
        // Here we need to lock
        unique_lock<mutex> l(*fi);

        if (docachesubitemslookup) {
            fi->notifyItemsNotPending();
            fi->notifyLocationNotPending();
        }
        if (dofetch) fi->notifyStatNotPending();
    }


return fi;

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
    time_t timelimit = time(0) - maxttl;
    time_t timelimit_neg = time(0) - maxttl_negative;

    bool dodelete = false;
    std::map< std::string, UgrFileInfo * >::iterator i_deleteme;

    for (std::map< std::string, UgrFileInfo * >::iterator i = data.begin();
            i != data.end(); i++) {

        if (dodelete) {
            data.erase(i_deleteme);
            dodelete = false;
        }

        UgrFileInfo *fi = i->second;


        if (fi) {
            time_t tl = timelimit;
            if (fi->getInfoStatus() == UgrFileInfo::NotFound)
                tl = timelimit_neg;

            if (fi->lastupdtime < tl) {
                // The item is old...
                Info(SimpleDebug::kLOW, fname, "purging expired item " << fi->name);

                if (fi->getInfoStatus() == UgrFileInfo::InProgress) {
                    Info(SimpleDebug::kLOW, fname, "Found inconsistent pending expired entry. Cannot purge " << fi->name);
                    continue;
                }


                lrudata.right.erase(i->first);
                dodelete = true;
                //data.erase(i);
                delete(fi);
                d++;
            }

        }

    }

    if (dodelete) {
        data.erase(i_deleteme);
        dodelete = false;
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


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

/** @file   LocationInfoHandler.cc
 * @brief  Handling of the info that is kept per each file
 * @author Fabrizio Furano
 * @date   Oct 2011
 */


#include "LocationInfoHandler.hh"
#include "ExtCacheHandler.hh"

using namespace boost;

// Get a pointer to a FileInfo, or create a new one

UgrFileInfo *LocationInfoHandler::getFileInfoOrCreateNewOne(UgrConnector& context, std::string &lfn, bool docachelookup, bool docachesubitemslookup) {
    const char *fname = "LocationInfoHandler::getFileInfoOrCreateNewOne";
    bool dofetch = false;
    UgrFileInfo *fi = 0;

    UgrFileInfo::trimpath(lfn);

    {
        boost::lock_guard<LocationInfoHandler> l(*this);

        std::map< std::string, UgrFileInfo *>::iterator p;

        p = data.find(lfn);
        if (p == data.end()) {

            // If we reached the max number of items, delete as much as we can
            while (data.size() > maxitems) {
                if (purgeLRUitem()) break;
            }

            // If we still have no space, try to garbage collect the old items
            if (data.size() > maxitems) {
                Info(UgrLogger::Lvl4, fname, "Too many items " << data.size() << ">" << maxitems << ", running garbage collection...");
                purgeExpired();
            }

            // If we still have no space, complain and do it anyway.
            if (data.size() > maxitems) {
                Info(UgrLogger::Lvl4, fname, "Maximum capacity exceeded. " << data.size() << ">" << maxitems);
            }


            // Create a new item
            fi = new UgrFileInfo(context, lfn);

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
            //docachesubitemslookup = false;
            dofetch = false;

            fi->touch();
        }
    }

    // Here we have either
    //  - a new empty UgrFileInfo
    //  - an UgrFileInfo taken from the 1st level cache

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

        if (fi->status_statinfo == UgrFileInfo::Ok) {

            if ((fi->status_items != UgrFileInfo::Ok) &&
                    (fi->status_locations != UgrFileInfo::Ok))
                getSubitemsFromCache(fi);
        }
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


// Add the newly existing file to the subitems of the parent directory

int LocationInfoHandler::addChildToParentSubitem(UgrConnector& context, std::string &lfn, bool checkExtCache) {
     const char *fname = "LocationInfoHandler::addChildToParentSubItem";
     bool doinsert = false;
     UgrFileInfo *fi = 0;
     std::string parent, child;

     UgrFileInfo::trimpath(lfn);

     // seperate parent and child
     size_t pos = lfn.rfind("/");
     if(pos != std::string::npos){
         parent = lfn.substr(0, pos);
         child = lfn.substr(pos + 1);
     }

     {
         boost::lock_guard<LocationInfoHandler> l(*this);

         std::map< std::string, UgrFileInfo *>::iterator p;

         p = data.find(parent);
         if (p == data.end()) {
             // parent item not found in memory and we don't want to check extenal cache, just return
             if(!checkExtCache) return 0;
             
             // If we reached the max number of items, delete as much as we can
             while (data.size() > maxitems) {
                 if (purgeLRUitem()) break;
             }

             // If we still have no space, try to garbage collect the old items
             if (data.size() > maxitems) {
                 Info(UgrLogger::Lvl4, fname, "Too many items " << data.size() << ">" << maxitems << ", running garbage collection...");
                 purgeExpired();
             }

             // If we still have no space, complain and do it anyway.
             if (data.size() > maxitems) {
                 Info(UgrLogger::Lvl4, fname, "Too many items " << data.size() << ">" << maxitems << ", running garbage collection...");
                 Info(UgrLogger::Lvl4, fname, "Maximum capacity exceeded. " << data.size() << ">" << maxitems);
             }
             
             // Create a new item
             fi = new UgrFileInfo(context, parent);

             // We want to see if it's available in the external cache
             // hence, make it pending
             doinsert = true;

             // We don't need to lock here, as we are the only holders
             // Set this object as pending, as we'll try to fetch it from an external cache (if any)
             fi->notifyItemsPending();
             fi->notifyStatPending();


         } else {
           // Just touch the element, it has not been used, just updated monodirectionally
           doinsert = false;
           fi = p->second;
           fi->touch();
         }
         
     }

     // Here we have either
     //  - a new empty UgrFileInfo
     //  - an UgrFileInfo taken from the 1st level cache

     // While we get it from the cache, the object is pending, and nothing is locked
     // Get the basic fields of the object, size, etc.
     if (doinsert) {
        if(getFileInfoFromCache(fi)) {
            // If we had an empty fi element and we could not fill it from the ext cache
            // then we exit and delete the now useless object.
            delete fi;
            return 1;
        }
        // If parent item is found, we also need the subitems since we need to add one more
        // No checks, if we are here we already have a fi object to take care of
        getSubitemsFromCache(fi);
     }

     // Add the new item
     UgrFileItem it;
     it.name = child;
     it.location.clear();
     fi->subdirs.insert(it);

     fi->dirtyitems = true;
     fi->dirty = true;

     // Found or not, the cache lookup for this object has ended
     // so it is marked as not pending
     {
         // Here we need to lock
         unique_lock<mutex> l(*fi);

         fi->notifyItemsNotPending();
         if (doinsert) fi->notifyStatNotPending();
     }

     // if in lite mode, we don't need to push to ext cache, just return
     if(!checkExtCache)
         return 0;

     // Insert to internal cache only if parent entry did not exist there
     if (doinsert) {
         boost::lock_guard<LocationInfoHandler> l(*this);
         data[parent] = fi;
         lrudata.insert(lrudataitem(++lrutick, parent));
     }
    
     // Parent is now updated with new subitem, push to external cache
     putFileInfoToCache(fi);
     putSubitemsToCache(fi);
     
     return 0;

}


// Purge from the local workspace the least recently used element
// Returns 0 if the element was purged, non0 if it was not possible

int LocationInfoHandler::purgeLRUitem() {
    const char *fname = "LocationInfoHandler::purgeLRUitem";

    // No LRU item, the LRU list is empty
    if (lrudata.empty()) {
        Info(UgrLogger::Lvl4, fname, "LRU list is empty. Nothing to purge.");
        return 1;
    }

    // Take the key of the lru item   
    std::string s = lrudata.left.begin()->second;
    Info(UgrLogger::Lvl4, fname, "LRU item is " << s);

    // Lookup its instance in the cache
    UgrFileInfo *fi = data[s];

    if (!fi) {
        Error(fname, "Could not find the LRU item in the cache.");
        return 2;
    }


    {
        unique_lock<mutex> lck(*fi);
        if (fi->getInfoStatus() == UgrFileInfo::InProgress) {
            Info(UgrLogger::Lvl4,fname, "The LRU item is marked as pending. Cannot purge " << fi->name);
            return 3;
        }
        if (fi->ispinned()) {
            Info(UgrLogger::Lvl4,fname, "The LRU item is marked as pinned. Cannot purge " << fi->name);
            return 4;
        }
    }

    // We have decided that we can delete it...

    // Purge it from the lru list
    lrudata.right.erase(s);

    // Remove the item from the map
    data.erase(s);


    // Delete it, eventually sending it to a 2nd level cache before
    putFileInfoToCache(fi);
    delete fi;

    return 0;
}

// Purge the items that were not touched since a longer time

void LocationInfoHandler::purgeExpired() {
    const char *fname = "LocationInfoHandler::purgeExpired";
    int d = 0;
    time_t timelimit = time(0) - maxttl;
    time_t timelimit_max = time(0) - maxmaxttl;
    time_t timelimit_neg = time(0) - maxttl_negative;

    bool dodelete = false;
    std::map< std::string, UgrFileInfo * >::iterator i_deleteme;

    for (std::map< std::string, UgrFileInfo * >::iterator i = data.begin();
            i != data.end(); i++) {

        if (dodelete) {
            data.erase(i_deleteme);
            dodelete = false;
        }
        dodelete = false;

        UgrFileInfo *fi = i->second;

        if (fi) {

            {
                unique_lock<mutex> lck(*fi);

                time_t tl = timelimit;
                if (fi->getInfoStatus() == UgrFileInfo::NotFound){
                    tl = timelimit_neg;
                }

                if ((fi->lastreftime < tl) || (fi->lastreftime < timelimit_max)) {
                    // The item is old...
                    Info(UgrLogger::Lvl2, fname, "purging expired item " << fi->name);

                    if (fi->getInfoStatus() == UgrFileInfo::InProgress) {
                        Error(fname, "Found pending expired entry. Cannot purge " << fi->name);
                        continue;
                    }

                    if (fi->ispinned()) {
                        Error(fname, "Found pinned expired entry. Cannot purge " << fi->name);
                        continue;
                    }

                    lrudata.right.erase(i->first);
                    dodelete = true;
                    i_deleteme = i;

                    d++;

                }
            }

            if (dodelete && fi) delete fi;

        }

    }

    if (dodelete) {
        data.erase(i_deleteme);
        dodelete = false;
    }

    if (d > 0)
        Info(UgrLogger::Lvl1, fname, "purged " << d << " expired items.");
}

void LocationInfoHandler::tick() {
    const char *fname = "LocationInfoHandler::tick";
    Info(UgrLogger::Lvl4, fname, "tick...");

    boost::lock_guard<LocationInfoHandler> l(*this);

    purgeExpired();

    // If we reached the max number of items, delete as much as we can
    while (data.size() > maxitems) {
        if (purgeLRUitem()) break;
    }

    Info(UgrLogger::Lvl4, fname, "Cache status. nItems:" << data.size() << " nLRUItems: " << lrudata.size());


}

int LocationInfoHandler::getFileInfoFromCache(UgrFileInfo *fi) {
    if (extcache)
        return extcache->getFileInfo(fi);
    return 0;
};

int LocationInfoHandler::getSubitemsFromCache(UgrFileInfo *fi) {
    if (extcache)
        return extcache->getSubitems(fi);
    return 0;
};

int LocationInfoHandler::putFileInfoToCache(UgrFileInfo *fi) {
    if (extcache)
        return extcache->putFileInfo(fi);
    return 0;
};

int LocationInfoHandler::putSubitemsToCache(UgrFileInfo *fi) {
    if (extcache)
        return extcache->putSubitems(fi);

    return 0;
};

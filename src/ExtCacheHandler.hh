/** @file   ExtCacheHandler.hh
 * @brief  Talk to an external, shared cache
 * @author Fabrizio Furano
 * @date   Jun 2012
 */

#ifndef EXTCACHEHANDLER_HH
#define EXTCACHEHANDLER_HH


#include "Config.hh"
#include "LocationInfo.hh"
#include <libmemcached/memcached.h>
#include <string>


/// This class implement basic functions that retrieve or store
/// FileInfo objects in an external cache, that is shared by multiple
/// concurrent instances of UGR

class ExtCacheHandler {
private:
    /// The Memcached connection
    memcached_st* conn;

    std::string makekey(UgrFileInfo *fi);
    std::string makekey_subitems(UgrFileInfo *fi);
public:



    // Cache in/out
    int getFileInfo(UgrFileInfo *fi);
    int getSubitems(UgrFileInfo *fi);
    int putFileInfo(UgrFileInfo *fi);
    int putSubitems(UgrFileInfo *fi);

    ExtCacheHandler();

    ~ExtCacheHandler() {
        memcached_free(conn);
        conn = 0;
    }



};



#endif


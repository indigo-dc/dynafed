/** @file   ExtCacheHandler.hh
 * @brief  Talk to an external, shared cache
 * @author Fabrizio Furano
 * @date   Jun 2012
 */

#ifndef EXTCACHEHANDLER_HH
#define EXTCACHEHANDLER_HH


#include "Config.hh"
#include "LocationInfo.hh"
#include "LocationPlugin.hh"
#include <libmemcached/memcached.h>
#include <string>
#include <queue>

/// This class implement basic functions that retrieve or store
/// FileInfo objects in an external cache, that is shared by multiple
/// concurrent instances of UGR

class PluginEndpointStatus;

class ExtCacheHandler {
private:


    /// This is our simple but effective pool of connections to memcached
    /// If a connection is not available, a new one is created and it is then added to the pool
    std::queue<memcached_st *> conns;
    boost::mutex connsmtx;

    /// Getting the Memcached connection
    memcached_st* getconn();
    /// Releasing the Memcached connection
    void releaseconn(memcached_st *c);

    
    /// The max ttl for an item in the cache
    int maxttl;

    std::string makekey(UgrFileInfo *fi);
    std::string makekey_subitems(UgrFileInfo *fi);
    
    std::string makekey_endpointstatus(std::string endpointname);
public:



    // Cache in/out
    int getFileInfo(UgrFileInfo *fi);
    int getSubitems(UgrFileInfo *fi);
    int putFileInfo(UgrFileInfo *fi);
    int putSubitems(UgrFileInfo *fi);
    
    int getEndpointStatus(PluginEndpointStatus *st, std::string endpointname);
    int putEndpointStatus(PluginEndpointStatus *st, std::string endpointname);

    ExtCacheHandler() {};

    void Init();
    
    ~ExtCacheHandler() {
        while (!conns.empty()) {
            memcached_free(conns.front());
            conns.pop();
        }
        

    }



};



#endif


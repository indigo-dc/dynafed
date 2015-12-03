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



/** @file   ExtCacheHandler.cc
 * @brief  Talk to an external, shared cache
 * @author Fabrizio Furano
 * @date   Jun 2012
 */


#include "ExtCacheHandler.hh"
#include "LocationInfoHandler.hh"
#include "LocationPlugin.hh"

#include <libmemcached/memcached.h>
#include <queue>

void ExtCacheHandler::Init() {
    maxttl = CFG->GetLong("extcache.memcached.ttl", 600);
    if (maxttl <= 0) {
      Error("ExtCacheHandler::Init", "extcache.memcached.ttl cannot be smaller than 1 second. Setting to 600 secs instead of " << maxttl);
      maxttl = 600;
    }
    
    while (!conns.empty()) conns.pop();
    while (!syncconns.empty()) syncconns.pop();
}

std::string ExtCacheHandler::makekey(UgrFileInfo *fi) {
    if (fi->name.length() > 0)
        return fi->name;

    return "/";
}

std::string ExtCacheHandler::makekey_subitems(UgrFileInfo *fi) {
    return "items_" + fi->name;
}



std::string ExtCacheHandler::makekey_endpointstatus(std::string endpointname) {
    return "endpoint_" + endpointname;
}




int ExtCacheHandler::getFileInfo(UgrFileInfo *fi) {
    const char *fname = "ExtCacheHandler::getFileInfo";
    memcached_st *conn = getconn();
    if (!conn) return 0;

    size_t value_len = 0;
    memcached_return err;
    uint32_t flags;
    std::string k;

    k = makekey(fi);

    char *strnfo = memcached_get(conn, k.c_str(), k.length(),
            &value_len, &flags, &err);

    Info(UgrLogger::Lvl3, fname, "Memcached get: Key='" << k << "' flags:" << flags << " Res: " << memcached_strerror(conn, err));

    releaseconn(conn);

    if (err != MEMCACHED_SUCCESS) {
        return 1;
    }

    if (!strnfo) {
        Error(fname, "Memcached retured a null value. Key=" << fi->name);
        return 1;
    }

    {
        boost::lock_guard<UgrFileInfo > l(*fi);
        fi->decode(strnfo, value_len);
    }
    free(strnfo);

    return 0;
};

int ExtCacheHandler::getSubitems(UgrFileInfo *fi) {
    const char *fname = "ExtCacheHandler::getSubitems";
    memcached_st *conn = getconn();
    if (!conn) return 0;

    size_t value_len = 0;
    memcached_return err;
    uint32_t flags;
    std::string k;

    k = makekey_subitems(fi);

    char *strnfo = memcached_get(conn, k.c_str(), k.length(),
            &value_len, &flags, &err);

    Info(UgrLogger::Lvl3, fname, "Memcached get: Key='" << k << "' flags:" << flags << " Res: " << memcached_strerror(conn, err));

    releaseconn(conn);

    if (err != MEMCACHED_SUCCESS) {
        return 1;
    }

    if (!strnfo) {
        Error(fname, "Memcached retured a null value. Key=" << fi->name);
        return 1;
    }

    {
        boost::lock_guard<UgrFileInfo > l(*fi);
        fi->decodeSubitems((void *) strnfo, value_len);
    }
    free(strnfo);

    return 0;
};

int ExtCacheHandler::putFileInfo(UgrFileInfo *fi) {
    const char *fname = "ExtCacheHandler::putFileInfo";


    std::string s, s1, k;
    time_t expirationtime = time(0) + maxttl;

    {
        boost::lock_guard<UgrFileInfo > l(*fi);
        if (!fi->dirty) return 0;
        fi->dirty = false;
        if (fi->getStatStatus() != UgrFileInfo::Ok) return 0;
        fi->encodeToString(s);
    }


    memcached_st *conn = getconn();
    if (!conn) return 0;

    k = makekey(fi);

    if (s.length() > 0) {

        Info(UgrLogger::Lvl3, fname, "memcached_set " <<
                " key:" << k << " len:" << s.length());

        memcached_return r = memcached_set(conn,
                k.c_str(), k.length(),
                s.c_str(), s.length() + 1,
                expirationtime, (uint32_t) 0);

        Info(UgrLogger::Lvl4, fname, "memcached_set " << "r:" << r <<
                " key:" << k << " len:" << s.length());

        if (r != MEMCACHED_SUCCESS) {
            Error(fname, "Cannot write fileinfo to memcached. retval=" << r << " '" << memcached_strerror(conn, r) <<
                    "' Key= " << k <<
                    " Valuelen: " << s.length());

            releaseconn(conn);

            return 1;
        }

        releaseconn(conn);

    }

    return 0;
};

int ExtCacheHandler::putSubitems(UgrFileInfo *fi) {
    const char *fname = "ExtCacheHandler::putSubitems";


    std::string s, k;
    time_t expirationtime = time(0) + maxttl;

    {
        boost::lock_guard<UgrFileInfo > l(*fi);
        if (!fi->dirtyitems) return 0;
        fi->dirtyitems = false;

        if (fi->unixflags & S_IFDIR) {
            if (fi->getItemsStatus() != UgrFileInfo::Ok) return 0;
        } else {
            if (fi->getLocationStatus() != UgrFileInfo::Ok) return 0;
        }

        fi->encodeSubitemsToString(s);
    }

    if (s.length() > 0) {
        k = makekey_subitems(fi);

        memcached_st *conn = getconn();
        if (!conn) return 0;

        Info(UgrLogger::Lvl3, fname, "memcached_set " <<
                " key:" << k << " len:" << s.length());

        memcached_return r = memcached_set(conn,
                k.c_str(), k.length(),
                s.c_str(), s.length() + 1,
                expirationtime, (uint32_t) 0);

        Info(UgrLogger::Lvl4, fname, "memcached_set " << "r:" << r <<
                " key:" << k << " len:" << s.length());

        if (r != MEMCACHED_SUCCESS) {
            Error(fname, "Cannot write subitems to memcached. retval=" << r << " '" << memcached_strerror(conn, r) <<
                    "' Key=" << k <<
                    " Valuelen:" << s.length());

            releaseconn(conn);

            return 1;
        }

        releaseconn(conn);

    }

    return 0;
};

/// Getting the Memcached connection, async version
memcached_st* ExtCacheHandler::getconn() {
    const char *fname = "ExtCacheHandler::getconn";
    memcached_st *res = 0;
    int r;

    {
        boost::lock_guard<boost::mutex> l(connsmtx);
        if (!conns.empty()) {
            res = conns.front();
            conns.pop();
            if (res) return res;
        }
    }


    Info(UgrLogger::Lvl1, fname, "Creating NEW memcached instance...");

    // Passing NULL means dynamically allocating space
    res = memcached_create(NULL);

    if (res) {
        Info(UgrLogger::Lvl1, fname, "Configuring memcached...");

        // Configure the memcached behaviour
        Info(UgrLogger::Lvl3, fname, "Setting memcached protocol...");
        if (CFG->GetBool("extcache.memcached.useBinaryProtocol", true)) {
            r = memcached_behavior_set(res, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL, 1);
            if (r != MEMCACHED_SUCCESS) {
                Error(fname, "Cannot set memcached protocol to binary. retval=" << r);
            }
        } else {
            r = memcached_behavior_set(res, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL, 0);
            if (r != MEMCACHED_SUCCESS) {
                Error(fname, "Cannot set memcached protocol to ascii. retval=" << r);
            }
        }

        Info(UgrLogger::Lvl3, fname, "Setting memcached distribution...");
        r = memcached_behavior_set(res, MEMCACHED_BEHAVIOR_DISTRIBUTION, MEMCACHED_DISTRIBUTION_CONSISTENT);
        if (r != MEMCACHED_SUCCESS) {
            Error(fname, "Cannot set memcached behavior to consistent. retval=" << r);
        }

        Info(UgrLogger::Lvl3, fname, "Setting memcached noblock...");
        r = memcached_behavior_set(res, MEMCACHED_BEHAVIOR_NO_BLOCK, 1);
        if (r != MEMCACHED_SUCCESS) {
            Error(fname, "Cannot set memcached behavior to async. retval=" << r);
        }

        Info(UgrLogger::Lvl3, fname, "Setting memcached TCP_NODELAY...");
        r = memcached_behavior_set(res, MEMCACHED_BEHAVIOR_TCP_NODELAY, 1);
        if (r != MEMCACHED_SUCCESS) {
            Error(fname, "Cannot set memcached TCP_NODELAY. retval=" << r);
        }

        Info(UgrLogger::Lvl3, fname, "Setting memcached NOREPLY...");
        r = memcached_behavior_set(res, MEMCACHED_BEHAVIOR_NOREPLY, 1);
        if (r != MEMCACHED_SUCCESS) {
            Error(fname, "Cannot set memcached NOREPLY. retval=" << r);
        }

        // Add memcached TCP hosts, take them from the config
        int i = 0;
        char buf[1024];
        char server[1024];
        do {

            CFG->ArrayGetString("extcache.memcached.server", buf, i++);

            if (!buf[0]) break;



            // split host and port
            char* host = 0;
            unsigned int port = 0;


            strcpy(server, buf);

            char* token;
            token = strtok(server, ":/?");
            if (token != NULL) {
                host = token;
            }
            token = strtok(NULL, ":/?");
            if (token != NULL) {
                port = atoi(token);
            }

            Info(UgrLogger::Lvl3, fname, "Adding memcached server '" << buf << "'" << host << ":" << port);

            r = memcached_server_add(res, host, port);
            if (r != MEMCACHED_SUCCESS) {
                Error(fname, "Cannot add server '" << host << "' retval=" << r);
            }

        } while (buf[0]);

    } else
        Error(fname, "Cannot create memcached instance");








    return res;
}
/// Releasing the Memcached connection

void ExtCacheHandler::releaseconn(memcached_st *c) {

    boost::lock_guard<boost::mutex> l(connsmtx);
    if (c) conns.push(c);
}

/// Releasing the sync Memcached connection
void ExtCacheHandler::releasesyncconn(memcached_st *c) {

    boost::lock_guard<boost::mutex> l(connsmtx);
    if (c) syncconns.push(c);
}






/// Getting the Memcached connection, SYNC version
memcached_st* ExtCacheHandler::getsyncconn() {
    const char *fname = "ExtCacheHandler::getsyncconn";
    memcached_st *res = 0;
    int r;

    {
        boost::lock_guard<boost::mutex> l(connsmtx);
        if (!syncconns.empty()) {
            res = syncconns.front();
            syncconns.pop();
            if (res) return res;
        }
    }


    Info(UgrLogger::Lvl1, fname, "Creating NEW memcached instance...");

    // Passing NULL means dynamically allocating space
    res = memcached_create(NULL);

    if (res) {
        Info(UgrLogger::Lvl1, fname, "Configuring memcached...");

        // Configure the memcached behaviour
        Info(UgrLogger::Lvl3, fname, "Setting memcached protocol...");
        if (CFG->GetBool("extcache.memcached.useBinaryProtocol", true)) {
            r = memcached_behavior_set(res, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL, 1);
            if (r != MEMCACHED_SUCCESS) {
                Error(fname, "Cannot set memcached protocol to binary. retval=" << r);
            }
        } else {
            r = memcached_behavior_set(res, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL, 0);
            if (r != MEMCACHED_SUCCESS) {
                Error(fname, "Cannot set memcached protocol to ascii. retval=" << r);
            }
        }

        Info(UgrLogger::Lvl3, fname, "Setting memcached distribution...");
        r = memcached_behavior_set(res, MEMCACHED_BEHAVIOR_DISTRIBUTION, MEMCACHED_DISTRIBUTION_CONSISTENT);
        if (r != MEMCACHED_SUCCESS) {
            Error(fname, "Cannot set memcached behavior to consistent. retval=" << r);
        }

        Info(UgrLogger::Lvl3, fname, "Setting memcached noblock...");
        r = memcached_behavior_set(res, MEMCACHED_BEHAVIOR_NO_BLOCK, 1);
        if (r != MEMCACHED_SUCCESS) {
            Error(fname, "Cannot set memcached behavior to async. retval=" << r);
        }

        Info(UgrLogger::Lvl3, fname, "Setting memcached TCP_NODELAY...");
        r = memcached_behavior_set(res, MEMCACHED_BEHAVIOR_TCP_NODELAY, 1);
        if (r != MEMCACHED_SUCCESS) {
            Error(fname, "Cannot set memcached TCP_NODELAY. retval=" << r);
        }

        // Commenting this out makes the connection SYNC
        //Info(UgrLogger::Lvl3, fname, "Setting memcached NOREPLY...");
        //r = memcached_behavior_set(res, MEMCACHED_BEHAVIOR_NOREPLY, 1);
        //if (r != MEMCACHED_SUCCESS) {
        //    Error(fname, "Cannot set memcached NOREPLY. retval=" << r);
        //}

        // Add memcached TCP hosts, take them from the config
        int i = 0;
        char buf[1024];
        char server[1024];
        do {

            CFG->ArrayGetString("extcache.memcached.server", buf, i++);

            if (!buf[0]) break;



            // split host and port
            char* host = 0;
            unsigned int port = 0;


            strcpy(server, buf);

            char* token;
            token = strtok(server, ":/?");
            if (token != NULL) {
                host = token;
            }
            token = strtok(NULL, ":/?");
            if (token != NULL) {
                port = atoi(token);
            }

            Info(UgrLogger::Lvl3, fname, "Adding memcached server '" << buf << "'" << host << ":" << port);

            r = memcached_server_add(res, host, port);
            if (r != MEMCACHED_SUCCESS) {
                Error(fname, "Cannot add server '" << host << "' retval=" << r);
            }

        } while (buf[0]);

    } else
        Error(fname, "Cannot create memcached instance");








    return res;
}






int ExtCacheHandler::getEndpointStatus(PluginEndpointStatus *st, std::string endpointname) {
    const char *fname = "ExtCacheHandler::getEndpointStatus";
    memcached_st *conn = getconn();
    if (!conn) return 0;

    size_t value_len = 0;
    memcached_return err;
    uint32_t flags;
    std::string k;

    k = makekey_endpointstatus(endpointname);

    char *strnfo = memcached_get(conn, k.c_str(), k.length(),
            &value_len, &flags, &err);

    Info(UgrLogger::Lvl3, fname, "Memcached get: Key='" << k << "' flags:" << flags << " Res: " << memcached_strerror(conn, err));

    releaseconn(conn);

    if (err != MEMCACHED_SUCCESS) {
        return 1;
    }

    if (!strnfo) {
        Error(fname, "Memcached retured a null value. Key=" << k);
        return 1;
    }

    st->decode(strnfo, value_len);

    free(strnfo);
    
    return 0;
}

int ExtCacheHandler::putEndpointStatus(PluginEndpointStatus *st, std::string endpointname) {
    const char *fname = "ExtCacheHandler::putEndpointStatus";


    std::string s, s1, k;
    time_t expirationtime = time(0) + maxttl;

    
        
        st->encodeToString(s);
    


    memcached_st *conn = getconn();
    if (!conn) return 0;

    k = makekey_endpointstatus(endpointname);

    if (s.length() > 0) {

        Info(UgrLogger::Lvl3, fname, "memcached_set " <<
                " key:" << k << " len:" << s.length());

        memcached_return r = memcached_set(conn,
                k.c_str(), k.length(),
                s.c_str(), s.length() + 1,
                expirationtime, (uint32_t) 0);

        Info(UgrLogger::Lvl4, fname, "memcached_set " << "r:" << r <<
                " key:" << k << " len:" << s.length());

        if (r != MEMCACHED_SUCCESS) {
            Error(fname, "Cannot write endpointstatus to memcached. retval=" << r << " '" << memcached_strerror(conn, r) <<
                    "' Key= " << k <<
                    " Valuelen: " << s.length());

            releaseconn(conn);

            return 1;
        }

        releaseconn(conn);

    }

    return 0;
}


int ExtCacheHandler::putMoninfo(std::string val) {
  const char *fname = "ExtCacheHandler::putMoninfo";
  
  time_t expirationtime = time(0) + 86400 * 30;
  
  memcached_st *conn = getsyncconn();
  if (!conn) return 0;
  
  if (val.length() > 0) {
    std::string k = "Ugrpluginstats_idx";
    uint64_t newval = 0;
    
    // First get a new value of the counter Ugrpluginstats_idx
    memcached_return r1 = memcached_increment_with_initial(conn, k.c_str(), k.length(),
                                                    1, 0, expirationtime, &newval);
    
    
    if (r1 != MEMCACHED_SUCCESS) {
      Error(fname, "Cannot increment monitoring index to memcached. retval=" << r1 << " '" << memcached_strerror(conn, r1) <<
      "' Key= " << k <<
      " Valuelen: " << val.length());
      
      releasesyncconn(conn);
      
      return 1;
    }
    
    char buf[32];
    sprintf(buf, "%ld", newval);
    k = "Ugrpluginstats_";
    k = k + buf;
    
    
    Info(UgrLogger::Lvl3, fname, "memcached_set " <<
    " key:" << k << " len:" << val.length());
    
    memcached_return r = memcached_set(conn,
                                       k.c_str(), k.length(),
                                       val.c_str(), val.length() + 1,
                                       expirationtime, (uint32_t) 0);
    
    Info(UgrLogger::Lvl4, fname, "memcached_set " << "r:" << r <<
    " key:" << k << " len:" << val.length());
    
    if (r != MEMCACHED_SUCCESS) {
      Error(fname, "Cannot write endpointstatus to memcached. retval=" << r << " '" << memcached_strerror(conn, r) <<
      "' Key= " << k <<
      " Valuelen: " << val.length());
      
      releaseconn(conn);
      
      return 1;
    }
    
    releaseconn(conn);
    
  }
  
  return 0;
}

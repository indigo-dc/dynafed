/** @file   ExtCacheHandler.cc
 * @brief  Talk to an external, shared cache
 * @author Fabrizio Furano
 * @date   Jun 2012
 */


#include "ExtCacheHandler.hh"
#include <libmemcached/memcached.h>


ExtCacheHandler::ExtCacheHandler() {
    const char *fname = "ExtCacheHandler";
    int r;

    maxttl = CFG->GetLong("extcache.memcached.ttl", 600);

    Info(SimpleDebug::kLOW, fname, "Creating memcached instance...");
    // Passing NULL means dynamically allocating space
    conn = memcached_create(NULL);

    if (conn) {
        Info(SimpleDebug::kLOW, fname, "Configuring memcached...");

        // Configure the memcached behaviour
        Info(SimpleDebug::kHIGH, fname, "Setting memcached protocol...");
        if (CFG->GetBool("extcache.memcached.useBinaryProtocol", true)) {
            r = memcached_behavior_set(conn, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL, 1);
            if (r != MEMCACHED_SUCCESS) {
                Error(fname, "Cannot set memcached protocol to binary. retval=" << r);
            }
        } else {
            r = memcached_behavior_set(conn, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL, 1);
            if (r != MEMCACHED_SUCCESS) {
                Error(fname, "Cannot set memcached protocol to ascii. retval=" << r);
            }
        }

        Info(SimpleDebug::kHIGH, fname, "Setting memcached distribution...");
        r = memcached_behavior_set(conn, MEMCACHED_BEHAVIOR_DISTRIBUTION, MEMCACHED_DISTRIBUTION_CONSISTENT);
        if (r != MEMCACHED_SUCCESS) {
            Error(fname, "Cannot set memcached behavior to consistent. retval=" << r);
        }

        Info(SimpleDebug::kHIGH, fname, "Setting memcached noblock...");
        r = memcached_behavior_set(conn, MEMCACHED_BEHAVIOR_NO_BLOCK, 1);
        if (r != MEMCACHED_SUCCESS) {
            Error(fname, "Cannot set memcached behavior to async. retval=" << r);
        }

        Info(SimpleDebug::kHIGH, fname, "Setting memcached TCP_NODELAY...");
        r = memcached_behavior_set(conn, MEMCACHED_BEHAVIOR_TCP_NODELAY, 1);
        if (r != MEMCACHED_SUCCESS) {
            Error(fname, "Cannot set memcached TCP_NODELAY. retval=" << r);
        }

        Info(SimpleDebug::kHIGH, fname, "Setting memcached NOREPLY...");
        r = memcached_behavior_set(conn, MEMCACHED_BEHAVIOR_NOREPLY, 1);
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

            Info(SimpleDebug::kHIGH, fname, "Adding memcached server '" << buf << "'" << host << ":" << port);

            r = memcached_server_add(conn, host, port);
            if (r != MEMCACHED_SUCCESS) {
                Error(fname, "Cannot add server '" << host << "' retval=" << r);
            }

        } while (buf[0]);

    } else
        Error(fname, "Cannot create memcached instance");


}

std::string ExtCacheHandler::makekey(UgrFileInfo *fi) {
    if (fi->name.length() > 0)
      return fi->name;
    
    return "/";
}

std::string ExtCacheHandler::makekey_subitems(UgrFileInfo *fi) {
    return "items_" + fi->name;
}

int ExtCacheHandler::getFileInfo(UgrFileInfo *fi) {
    const char *fname = "ExtCacheHandler::getFileInfo";
    if (!conn) return 0;

    size_t value_len = 0;
    memcached_return err;
    uint32_t flags;
    std::string k;

    k = makekey(fi);

    char *strnfo = memcached_get(conn, k.c_str(), k.length(),
            &value_len, &flags, &err);

    Info(SimpleDebug::kHIGH, fname, "Memcached get: Key='" << k  << "' flags:" << flags << " Res: " << memcached_strerror(conn, err));

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
    if (!conn) return 0;

    size_t value_len = 0;
    memcached_return err;
    uint32_t flags;
    std::string k;

    k = makekey_subitems(fi);
   
    char *strnfo = memcached_get(conn, k.c_str(), k.length(),
            &value_len, &flags, &err);

    Info(SimpleDebug::kHIGH, fname, "Memcached get: Key='" << k  << "' flags:" << flags << " Res: " << memcached_strerror(conn, err));

    if (err != MEMCACHED_SUCCESS) {
        return 1;
    }
    
    if (!strnfo) {
        Error(fname, "Memcached retured a null value. Key=" << fi->name);
        return 1;
    }

    {
        boost::lock_guard<UgrFileInfo > l(*fi);
        fi->decodeSubitems((void *)strnfo, value_len);
    }
    free(strnfo);

    return 0;
};

int ExtCacheHandler::putFileInfo(UgrFileInfo *fi) {
    const char *fname = "ExtCacheHandler::putFileInfo";
    if (!conn) return 0;

    std::string s, s1, k;
    time_t expirationtime = time(0) + maxttl;

    {
        boost::lock_guard<UgrFileInfo > l(*fi);
        if (!fi->dirty) return 0;
        fi->dirty = false;
        if (fi->getStatStatus() != UgrFileInfo::Ok) return 0;
        fi->encodeToString(s);
    }

    k = makekey(fi);

    if (s.length() > 0) {

        Info(SimpleDebug::kHIGH, fname, "memcached_set " <<
                "key:" << k << " len:" << s.length());

        memcached_return r = memcached_set(conn,
                k.c_str(), k.length(),
                s.c_str(), s.length()+1,
                expirationtime, (uint32_t) 0);



        if (r != MEMCACHED_SUCCESS) {
            Error(fname, "Cannot write fileinfo to memcached. retval=" << r << " '" << memcached_strerror(conn, r) <<
                    "' Key= " << k <<
                    " Valuelen: " << s.length());
            return 1;
        }
    }

    return 0;
};

int ExtCacheHandler::putSubitems(UgrFileInfo *fi) {
    const char *fname = "ExtCacheHandler::putSubitems";
    if (!conn) return 0;

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

        Info(SimpleDebug::kHIGH, fname, "memcached_set " <<
                "key:" << k << " len:" << s.length());

        memcached_return r = memcached_set(conn,
                k.c_str(), k.length(),
                s.c_str(), s.length()+1,
                expirationtime, (uint32_t) 0);

        Info(SimpleDebug::kHIGH, fname, "memcached_set " << "r:" << r <<
                "key:" << k << " len:" << s.length());

        if (r != MEMCACHED_SUCCESS) {
            Error(fname, "Cannot write subitems to memcached. retval=" << r << " '" << memcached_strerror(conn, r) <<
                    "' Key=" << k <<
                    " Valuelen:" << s.length());
            return 1;
        }
    }

    return 0;
};


/// @file    UgrDMLite.cc
/// @brief   Let dmlite use UGR as a plugin.
/// @author  Fabrizio Furano <furano@cern.ch>
/// @date    Feb 2012
#include "UgrDMLite.hh"
#include <set>
#include <vector>
#include <regex.h>

#include <unistd.h>
#include <sys/types.h>
#include "dmlite/cpp/utils/dm_urls.h"

using namespace dmlite;

UgrConnector *UgrCatalog::conn = 0;

UgrFactory::UgrFactory(CatalogFactory* catalogFactory) throw (DmException) :
nestedFactory_(catalogFactory) {
    // Make sure that there is an UgrConnector ready to be used
    // NOTE: calls to this ctor MUST be serialized
    UgrCatalog::getUgrConnector();
}

UgrFactory::~UgrFactory() throw (DmException) {
    // Nothing
}

void UgrFactory::configure(const std::string& key, const std::string& value) throw (DmException) {
    if (!key.compare("Ugr_cfgfile")) {
        cfgfile = value;
        UgrCatalog::getUgrConnector()->resetinit();


    } else
        throw DmException(DM_UNKNOWN_OPTION, std::string("Unknown option ") + key);
}

Catalog* UgrFactory::createCatalog(dmlite::PluginManager *pm) throw (DmException) {

    int r = UgrCatalog::getUgrConnector()->init((char *) cfgfile.c_str());
    if (r > 0)
        throw DmException(DM_NO_CATALOG, "UgrConnector initialization failed.");

    if (this->nestedFactory_ != 0x00)

        return new UgrCatalog(CatalogFactory::createCatalog(this->nestedFactory_, pm));
    else
        return new UgrCatalog(0x00);
}

static void registerPluginUgr(PluginManager* pm) throw (DmException) {
    try {
        pm->registerFactory(new UgrFactory(NULL /*pm->getCatalogFactory()*/));
    } catch (DmException e) {
        //        if (e.code() == DM_NO_FACTORY)
        //            throw DmException(DM_NO_FACTORY, std::string("UgrDMLite can not be loaded first"));
        //        throw;
    }

    try {
        pm->registerFactory(new UgrUserGroupDbFactory());
    } catch (DmException e) {
        //        if (e.code() == DM_NO_FACTORY)
        //            throw DmException(DM_NO_FACTORY, std::string("UgrDMLite can not be loaded first"));
        //        throw;
    }
}



/// This is what the PluginManager looks for
PluginIdCard plugin_ugr = {
    API_VERSION,
    registerPluginUgr
};

void UgrCatalog::setSecurityContext(const SecurityContext *c) throw (DmException) {
    secCredentials = c->getCredentials();
}

UgrCatalog::UgrCatalog(Catalog* decorates) throw (DmException) :
DummyCatalog(decorates) {

}

UgrCatalog::~UgrCatalog() throw (DmException) {
    // Nothing
}

std::string UgrCatalog::getImplId() throw () {
    return std::string("UgrCatalog");
}

std::vector<FileReplica> UgrCatalog::getReplicas(const std::string& path) throw (DmException) {
    std::vector<FileReplica> replicas;


    // Get all of them
    UgrFileInfo *nfo = 0;
    int priocnt = 0;

    if (!getUgrConnector()->locate((std::string&)path, &nfo) && nfo) {
        Info(SimpleDebug::kLOW, "UgrCatalog::getReplicas", " get location with success, try to sort / choose a proper one");
        // Request UgrConnector to sort a replica set according to proximity to the client
        std::set<UgrFileItem_replica, UgrFileItemGeoComp> repls = getUgrConnector()->getGeoSortedReplicas(secCredentials.remote_addr, nfo);

        // Populate the vector
        FileReplica r;
        Url u;
        for (std::set<UgrFileItem_replica>::iterator i = repls.begin(); i != repls.end(); ++i) {
            Info(SimpleDebug::kHIGH, "UgrCatalog::getReplicas", i->name << " " << i->location << " " << i->latitude << " " << i->longitude);
            r.fileid = 0;
            r.replicaid = 0;
            r.status = '-';
            r.priority = priocnt++;
            u = splitUrl(i->name);


            strncpy(r.rfn, i->name.c_str(), sizeof (r.rfn));
            r.rfn[sizeof (r.rfn) - 1] = '\0';

            if (u.host) {
                strncpy(r.server, u.host, sizeof (r.server));
                r.server[sizeof (r.server) - 1] = '\0';
            }


            replicas.push_back(r);
        }


    } else {
        Info(SimpleDebug::kLOW, "UgrCatalog::getReplicas", "  -> failure in get location ");
    }

    // Return
    if (replicas.size() == 0)
        throw DmException(DM_NO_REPLICAS, "There are no available replicas");
    return replicas;
}

void UgrCatalog::getIdMap(const std::string &userName, const std::vector<std::string> &groupNames,
        uid_t *uid, std::vector<gid_t> *gids) throw (DmException) {

    *uid = 0;
    gids->push_back(0);

}

Location UgrCatalog::get(const std::string& path) throw (DmException) {

    std::vector<FileReplica> replicas;

    // Get all the available
    replicas = this->getReplicas(path);

    // The first one is fine
    Url u = splitUrl(replicas[0].rfn);

    return Location(u);
}

void UgrCatalog::exclude(int64_t replicaId) {
    this->excluded_.insert(replicaId);
}

bool UgrCatalog::isExcluded(int64_t replicaId) {
    return this->excluded_.count(replicaId) != 0;
}

void fillstat(struct stat &st, UgrFileInfo *nfo) {
    nfo->lock();
    st.st_uid = 0;
    st.st_size = nfo->size;
    st.st_rdev = 0;
    st.st_nlink = 0;

    st.st_mtim.tv_sec = nfo->mtime;
    st.st_mtim.tv_nsec = 0;

    st.st_ctim.tv_sec = nfo->ctime;
    st.st_ctim.tv_nsec = 0;

    st.st_atim.tv_sec = nfo->atime;
    st.st_atim.tv_nsec = 0;

    st.st_mode = nfo->unixflags;
    st.st_ino = 0;
    st.st_gid = 0;
    st.st_dev = 0;

    st.st_blocks = nfo->size / 1024;
    st.st_blksize = 1024;


    nfo->unlock();
}

struct stat UgrCatalog::stat(const std::string& path) throw (DmException) {
    // do the stat
    struct stat st;
    UgrFileInfo *nfo = 0;

    if (!getUgrConnector()->stat((std::string&)path, &nfo) && nfo && (nfo->getInfoStatus() != nfo->NotFound)) {

        fillstat(st, nfo);

        return st;
    }
    throw DmException(DM_NO_SUCH_FILE, "File not found");
}

struct xstat UgrCatalog::extendedStat(const std::string& path, bool followsym) throw (DmException) {
    struct xstat st;
    UgrFileInfo *nfo = 0;
    if (!getUgrConnector()->stat((std::string&)path, &nfo) && nfo && (nfo->getInfoStatus() != nfo->NotFound)) {
        st.csumtype[0] = '\0';
        st.csumvalue[0] = '\0';
        st.guid[0] = '\0';
        strncpy(st.name, nfo->name.c_str(), sizeof (st.name));
        st.name[sizeof (st.name) - 1] = '\0';
        st.parent = 0;
        st.status = '-';
        st.type = 0;
        fillstat(st.stat, nfo);

        return st;
    }
    throw DmException(DM_NO_SUCH_FILE, "File not found");
}

class myDirectory {
  public:
    UgrFileInfo *nfo;
    std::set<UgrFileItem>::iterator idx;

    ExtendedStat buf;
    struct dirent direntbuf;

    myDirectory(UgrFileInfo *finfo) : nfo(finfo) {
        idx = finfo->subdirs.begin();
        memset(&buf, 0, sizeof (buf));
        memset(&direntbuf, 0, sizeof (direntbuf));
    }

};

Directory* UgrCatalog::openDir(const std::string &path) throw (DmException) {
    UgrFileInfo *fi;

    if (!getUgrConnector()->list((std::string&)path, &fi) && fi && (fi->getItemsStatus() == fi->Ok)) {

        // This is just an opaque pointer, we can store what we want
        return (Directory *) (new myDirectory(fi));
    }

    if (fi->getItemsStatus() == UgrFileInfo::Error)
        throw DmException(DM_TOO_MANY_SYMLINKS, "Error getting directory content (likely the directory is bigger than the limit)");


    return 0;
}

void UgrCatalog::closeDir(Directory *opaque) throw (DmException) {
    myDirectory *d = (myDirectory *) opaque;
    delete d;
}

struct dirent* UgrCatalog::readDir(Directory *opaque) throw (DmException) {
    myDirectory *d = (myDirectory *) opaque;

    if (d->idx == d->nfo->subdirs.end()) return 0;

    // Only the name is relevant here, it seems
    strncpy(d->direntbuf.d_name, (d->idx)->name.c_str(), sizeof (d->direntbuf.d_name));
    d->direntbuf.d_name[sizeof (d->direntbuf.d_name) - 1] = '\0';

    d->idx++;

    return &(d->direntbuf);
}

ExtendedStat* UgrCatalog::readDirx(Directory *opaque) throw (DmException) {
    myDirectory *d = (myDirectory *) opaque;

    if (d->idx == d->nfo->subdirs.end()) return 0;

    // Only the name is relevant here, it seems
    strncpy(d->buf.name, (d->idx)->name.c_str(), sizeof (d->buf.name));
    d->buf.name[sizeof (d->buf.name) - 1] = '\0';

    std::string s = d->nfo->name;

    if (*s.rbegin() != '/')
        s += "/";

    s += d->buf.name;

    d->buf.stat = stat(s);

    d->idx++;

    return &(d->buf);
}






// ---------------------------

SecurityContext* UgrUserGroupDb::createSecurityContext(const SecurityCredentials &c) throw (dmlite::DmException) {

    Info(SimpleDebug::kHIGHEST, "UgrUserGroupDb::createSecurityContext", c.remote_addr);


    return new SecurityContext(c);

}



/// UserGroupDbFactory

/// Set a configuration parameter
/// @param key   The configuration parameter
/// @param value The value for the configuration parameter

void UgrUserGroupDbFactory::configure(const std::string& key, const std::string& value) throw (DmException) {
    throw DmException(DM_UNKNOWN_OPTION, std::string("Unknown option ") + key);
};

/// Instantiate a implementation of UserGroupDb
/// @param si The StackInstance that is instantiating the context. It may be NULL.

UserGroupDb* UgrUserGroupDbFactory::createUserGroupDb(dmlite::PluginManager *) throw (DmException) {
    Info(SimpleDebug::kHIGHEST, "UserGroupDbFactory::createUserGroupDb", "Creating UsrUserGroupDb instance...");


    return new UgrUserGroupDb();
};



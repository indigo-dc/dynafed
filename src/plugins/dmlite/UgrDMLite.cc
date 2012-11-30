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
#include "dmlite/cpp/utils/urls.h"

using namespace dmlite;

UgrConnector *UgrCatalog::conn = 0;

UgrFactory::UgrFactory() throw (DmException) {
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
        throw DmException(DMLITE_CFGERR(0), std::string("Unknown option ") + key);
}

Catalog* UgrFactory::createCatalog(CatalogFactory* factory,
        PluginManager* pm) throw (DmException) {

    int r = UgrCatalog::getUgrConnector()->init((char *) cfgfile.c_str());
    if (r > 0)
        throw DmException(DMLITE_CFGERR(DMLITE_NO_CATALOG), "UgrConnector initialization failed.");

    return new UgrCatalog();
}

static void registerPluginUgr(PluginManager* pm) throw (DmException) {
    UgrFactory *f = new UgrFactory();

    try {
        pm->registerCatalogFactory(f);
    } catch (DmException e) {
        //        if (e.code() == DM_NO_FACTORY)
        //            throw DmException(DM_NO_FACTORY, std::string("UgrDMLite can not be loaded first"));
        //        throw;
    }

    try {
        pm->registerAuthnFactory(f);
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
    secCredentials = c->credentials;
}

UgrCatalog::UgrCatalog() throw (DmException) : DummyCatalog(NULL) {

}

UgrCatalog::~UgrCatalog() throw (DmException) {
    // Nothing
}

std::string UgrCatalog::getImplId() const throw () {
    return std::string("UgrCatalog");
}

std::vector<Replica> UgrCatalog::getReplicas(const std::string &path) throw (DmException) {
    std::vector<Replica> replicas;


    // Get all of them
    UgrFileInfo *nfo = 0;

    std::string abspath = getAbsPath(const_cast<std::string&> (path));
    if (!getUgrConnector()->locate((std::string&)abspath, &nfo) && nfo) {
        Info(SimpleDebug::kLOW, "UgrCatalog::getReplicas", " get location with success, try to sort / choose a proper one");
        // Request UgrConnector to sort a replica set according to proximity to the client
        std::set<UgrFileItem_replica, UgrFileItemGeoComp> repls = getUgrConnector()->getGeoSortedReplicas(secCredentials.remoteAddress, nfo);

        // Populate the vector
        Replica r;

        for (std::set<UgrFileItem_replica>::iterator i = repls.begin(); i != repls.end(); ++i) {

            // Filter out the replicas that belong to dead endpoints
            if (!getUgrConnector()->isEndpointOK(i->pluginID)) {
                Info(SimpleDebug::kHIGH, "UgrCatalog::getReplicas", "Skipping " << i->name << " " << i->location << " " << i->latitude << " " << i->longitude);
                continue;
            }

            Info(SimpleDebug::kHIGH, "UgrCatalog::getReplicas", i->name << " " << i->location << " " << i->latitude << " " << i->longitude);
            r.fileid = 0;
            r.replicaid = 0;
            r.status = Replica::kAvailable;

            Url u(i->name);


            r.rfn = i->name;


            r.server = u.domain;



            replicas.push_back(r);
        }


    } else {
        Info(SimpleDebug::kLOW, "UgrCatalog::getReplicas", "  -> failure in get location ");
    }

    // Return
    if (replicas.size() == 0)
        throw DmException(DMLITE_NO_REPLICAS, "There are no available replicas");
    return replicas;
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

dmlite::ExtendedStat UgrCatalog::extendedStat(const std::string& path, bool followsym) throw (DmException) {
    dmlite::ExtendedStat st;
    UgrFileInfo *nfo = 0;
    std::string abspath = getAbsPath(const_cast<std::string&> (path));
    if (!getUgrConnector()->stat((std::string&)abspath, &nfo) && nfo && (nfo->getInfoStatus() != nfo->NotFound)) {
        st.csumtype[0] = '\0';
        st.csumvalue[0] = '\0';
        st.guid[0] = '\0';
        st.name = nfo->name;
        st.name[sizeof (st.name) - 1] = '\0';
        st.parent = 0;
        st.status = ExtendedStat::kOnline;

        fillstat(st.stat, nfo);

        return st;
    }
    throw DmException(ENOENT, "File not found");
}

class myDirectory {
  public:
    UgrFileInfo *nfo;
    std::set<UgrFileItem>::iterator idx;

    dmlite::ExtendedStat buf;
    struct dirent direntbuf;

    myDirectory(UgrFileInfo *finfo) : nfo(finfo) {
        idx = finfo->subdirs.begin();
        buf.clear();

        memset(&direntbuf, 0, sizeof (direntbuf));
    }

};

Directory* UgrCatalog::openDir(const std::string &path) throw (DmException) {
    UgrFileInfo *fi;

    std::string abspath = getAbsPath(const_cast<std::string&> (path));
    if (!getUgrConnector()->list((std::string&)abspath, &fi) && fi) {

        if (fi->getItemsStatus() == UgrFileInfo::Ok) {
            boost::lock_guard<UgrFileInfo > l(*fi);
            fi->pin();
            // This is just an opaque pointer, we can store what we want
            return (Directory *) (new myDirectory(fi));
        }
    }

    if (fi->getItemsStatus() == UgrFileInfo::NotFound)
        throw DmException(ENOENT, "File not found");

    if (fi->getItemsStatus() == UgrFileInfo::InProgress)
        throw DmException(DMLITE_MALFORMED, "Error getting directory content. Timeout.");

    if (fi->getItemsStatus() == UgrFileInfo::Error)
        throw DmException(DMLITE_MALFORMED, "Error getting directory content (likely the directory is bigger than the limit)");


    return 0;
}

void UgrCatalog::closeDir(Directory *opaque) throw (DmException) {
    myDirectory *d = (myDirectory *) opaque;
    
    if (d && d->nfo) {
        boost::lock_guard<UgrFileInfo > l(*d->nfo);
        d->nfo->unpin();
        delete d;
    }
}

struct dirent* UgrCatalog::readDir(Directory *opaque) throw (DmException) {
    myDirectory *d = (myDirectory *) opaque;

    if (!opaque) return 0;
    if (!d->nfo) return 0;

    {
        boost::lock_guard<UgrFileInfo > l(*d->nfo);
        d->nfo->touch();

        if (d->idx == d->nfo->subdirs.end()) return 0;

        // Only the name is relevant here, it seems
        strncpy(d->direntbuf.d_name, (d->idx)->name.c_str(), sizeof (d->direntbuf.d_name));
        d->direntbuf.d_name[sizeof (d->direntbuf.d_name) - 1] = '\0';

        d->idx++;
    }

    return &(d->direntbuf);
}

dmlite::ExtendedStat* UgrCatalog::readDirx(Directory *opaque) throw (DmException) {
    myDirectory *d = (myDirectory *) opaque;
    std::string s;

    if (!opaque) return 0;
    if (!d->nfo) return 0;

    {
        boost::lock_guard<UgrFileInfo > l(*d->nfo);
        d->nfo->touch();
        s = d->nfo->name;
        if (d->idx == d->nfo->subdirs.end()) return 0;

        // Only the name is relevant here, it seems
        d->buf.name = (d->idx)->name;
        d->idx++;
    }


    if (*s.rbegin() != '/')
        s += "/";

    s += d->buf.name;
    d->buf.stat = extendedStat(s, true).stat;




    return &(d->buf);
}

void UgrCatalog::changeDir(const std::string &d) throw (DmException) {
    workingdir = d;
    UgrFileInfo::trimpath(workingdir);
}

std::string UgrCatalog::getWorkingDir() throw (DmException) {
    return workingdir;
}

std::string UgrCatalog::getAbsPath(std::string &path) {
    if (workingdir.empty()) return path;
    if (path[0] == '/') return path;
    if (path == ".") return workingdir;


    std::string s = workingdir + path;
    return s;
}

// ---------------------------

dmlite::SecurityContext* UgrAuthn::createSecurityContext(const SecurityCredentials &c) throw (dmlite::DmException) {

    Info(SimpleDebug::kHIGHEST, "UgrAuthn::createSecurityContext", c.remoteAddress);


    return new dmlite::SecurityContext(cred, userinfo, groupinfo);

}



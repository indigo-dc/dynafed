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


    } else
        throw DmException(DM_UNKNOWN_OPTION, std::string("Unknown option ") + key);
}

Catalog* UgrFactory::createCatalog() throw (DmException) {

    UgrCatalog::getUgrConnector()->init((char *) cfgfile.c_str());
    if (this->nestedFactory_ != 0x00)

        return new UgrCatalog(this->nestedFactory_->createCatalog());
    else
        return new UgrCatalog(0x00);
}

static void registerPluginUgr(PluginManager* pm) throw (DmException) {
    try {
        pm->registerCatalogFactory(new UgrFactory(NULL /*pm->getCatalogFactory()*/));
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


// ---------------------------

UgrCatalog::UgrCatalog(Catalog* decorates) throw (DmException) :
DummyCatalog(decorates) {

}

UgrCatalog::~UgrCatalog() throw (DmException) {
    // Nothing
}

std::string UgrCatalog::getImplId() throw () {
    return std::string("UgrCatalog");
}

void UgrCatalog::set(const std::string& key, va_list vargs) throw (DmException) {
    if (key == "ExcludeReplicas") {
        int n_list = va_arg(vargs, int);
        int64_t* id_list = va_arg(vargs, int64_t*);

        for (int i = 0; i < n_list; ++i)
            this->exclude(id_list[i]);
    } else if (key == "ClearExcluded") {
        this->excluded_.clear();
    }

    if (this->decorated_ != 0x00)
        this->decorated_->set(key, vargs);
    else
        throw DmException(DM_UNKNOWN_OPTION, "Unknown option " + key);
}

void UgrCatalog::setUserId(uid_t uid, gid_t gid, const std::string& dn) throw (DmException) {

    userdn = dn;

}

void UgrCatalog::setVomsData(const std::string &vo, const std::vector<std::string> &fqans) throw (DmException) {

    voms_vo = vo;
    voms_fqans = fqans;
}

Uri splitUri(const std::string& uri) {
    regex_t regexp;
    regmatch_t matches[7];
    const char *p = uri.c_str();
    Uri parsed;

    // Compile the first time
    assert(regcomp(&regexp,
            "(([[:alnum:]]+):/{2})?([[:alnum:]]+(\\.[[:alnum:]]+)*)?(:[[:digit:]]*)?(/.*)?",
            REG_EXTENDED | REG_ICASE) == 0);

    // Match and extract
    if (regexec(&regexp, p, 7, matches, 0) == 0) {
        int len;

        // Scheme
        len = matches[2].rm_eo - matches[2].rm_so;
        memcpy(parsed.scheme, p + matches[2].rm_so, len);
        parsed.scheme[len] = '\0';

        // Host
        len = matches[3].rm_eo - matches[3].rm_so;
        memcpy(parsed.host, p + matches[3].rm_so, len);
        parsed.host[len] = '\0';

        // Port
        len = matches[5].rm_eo - matches[5].rm_so;
        if (len > 0)
            parsed.port = atoi(p + matches[5].rm_so + 1);
        else
            parsed.port = 0;

        // Rest
        strncpy(parsed.path, p + matches[6].rm_so, PATH_MAX);
    }

    return parsed;
}

std::vector<FileReplica> UgrCatalog::getReplicas(const std::string& path) throw (DmException) {
    std::vector<FileReplica> replicas;


    // Get all of them
    UgrFileInfo *nfo = 0;

    if (!getUgrConnector()->locate((std::string&)path, &nfo) && nfo) {


        // Populate the vector
        FileReplica r;
        for (std::set<UgrFileItem>::iterator i = nfo->subitems.begin(); i != nfo->subitems.end(); ++i) {

            r.fileid = 0;
            r.replicaid = 0;
            r.status = '-';

            strncpy(r.unparsed_location, i->name.c_str(), sizeof (r.unparsed_location));
            r.unparsed_location[sizeof(r.unparsed_location) - 1] = '\0';

            Info(SimpleDebug::kHIGH, "UgrCatalog::getReplicas", i->name);
            r.location = splitUri(i->name);
            replicas.push_back(r);
        }

        // Remove excluded
        std::vector<FileReplica>::iterator i;
        for (i = replicas.begin(); i != replicas.end(); ++i) {
            if (this->isExcluded(i->replicaid)) {
                i = replicas.erase(i);
            }
        }
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

FileReplica UgrCatalog::get(const std::string& path) throw (DmException) {
    std::vector<FileReplica> replicas;

    // Get all the available
    replicas = this->getReplicas(path);

    // The first one is fine
    return replicas[0];
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

    memset(&st.st_mtim, 0, sizeof (st.st_mtim));

    st.st_mode = nfo->unixflags;
    st.st_ino = 0;
    st.st_gid = 0;
    st.st_dev = 0;

    memset(&st.st_ctim, 0, sizeof (st.st_ctim));

    st.st_blocks = nfo->size / 1024;
    st.st_blksize = 1024;

    memset(&st.st_atim, 0, sizeof (st.st_atim));

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

    struct direntstat buf;

    myDirectory(UgrFileInfo *finfo) : nfo(finfo) {
        idx = finfo->subitems.begin();
        memset(&buf, 0, sizeof (buf));
    }

};

Directory* UgrCatalog::openDir(const std::string &path) throw (DmException) {
    UgrFileInfo *fi;

    if (!getUgrConnector()->list((std::string&)path, &fi) && fi && (fi->getInfoStatus() != fi->NotFound)) {

        // This is just an opaque pointer, we can store what we want
        return (Directory *) (new myDirectory(fi));
    }
    return 0;
}

void UgrCatalog::closeDir(Directory *opaque) throw (DmException) {
    myDirectory *d = (myDirectory *) opaque;
    delete d;
}

struct dirent* UgrCatalog::readDir(Directory *opaque) throw (DmException) {
    myDirectory *d = (myDirectory *) opaque;

    if (d->idx == d->nfo->subitems.end()) return 0;

    // Only the name is relevant here, it seems
    strncpy(d->buf.dirent.d_name, (d->idx)->name.c_str(), sizeof (d->buf.dirent.d_name));
    d->buf.dirent.d_name[sizeof (d->buf.dirent.d_name) - 1] = '\0';

    d->idx++;

    return &(d->buf.dirent);
}

struct direntstat* UgrCatalog::readDirx(Directory *opaque) throw (DmException) {
    myDirectory *d = (myDirectory *) opaque;

    if (d->idx == d->nfo->subitems.end()) return 0;

    // Only the name is relevant here, it seems
    strncpy(d->buf.dirent.d_name, (d->idx)->name.c_str(), sizeof (d->buf.dirent.d_name));
    d->buf.dirent.d_name[sizeof (d->buf.dirent.d_name) - 1] = '\0';

    std::string s = d->nfo->name;

    if (*s.rbegin() != '/')
        s += "/";
    
    s += d->buf.dirent.d_name;

    d->buf.stat = stat(s);

    d->idx++;

    return &(d->buf);
}
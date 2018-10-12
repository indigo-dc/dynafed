



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
#include <string.h>
#include "UgrAuthorization.hh"
#include <string.h>

#include <dmlite/cpp/utils/urls.h>
#include <dmlite/cpp/pooldriver.h>

using namespace dmlite;

UgrConnector *UgrCatalog::conn = 0;


int ugrToDmliteErrCode(const UgrCode & c){
    switch(c.getCode()){
        case UgrCode::FileNotFound:
            return ENOENT;
        case UgrCode::PermissionDenied:
        case UgrCode::OverwriteNotAllowed:
            return EPERM;
        case UgrCode::Ok:
            return 0;
        default:
            return EFAULT;


    }
}


UgrFactory::UgrFactory() {

    ugrlogmask = UgrLogger::get()->getMask(ugrlogname);
    Info(UgrLogger::Lvl3, "UgrFactory::UgrFactory", "UgrFactory starting");

    createremoteparentdirs = true;
    
    // Make sure that there is an UgrConnector ready to be used
    // NOTE: calls to this ctor MUST be serialized
    UgrCatalog::getUgrConnector();
}

UgrFactory::~UgrFactory()  {
    // Nothing
}

void UgrFactory::configure(const std::string& key, const std::string& value) {
  if (!key.compare("Ugr_cfgfile")) {
    cfgfile = value;
    
    Info(UgrLogger::Lvl2, "UgrFactory::configure", "Getting config file: " << value);
    UgrCatalog::getUgrConnector()->resetinit();
  }
  else if (!key.compare("Ugr_precreateremoteparentdirsonput")) {
    Info(UgrLogger::Lvl2, "UgrFactory::configure", "key: '"<< key << "' <- " << value);
    if ((value == "n") || (value =="no") || (value == "0") || (value == "false"))
      createremoteparentdirs = false;
  }
}

Catalog* UgrFactory::createCatalog(CatalogFactory* factory,
        PluginManager* pm) {

    Info(UgrLogger::Lvl2, "UgrFactory::createCatalog", "Creating catalog instance. cfg: " << cfgfile.c_str());
    int r = UgrCatalog::getUgrConnector()->init((char *) cfgfile.c_str());
    if (r > 0)
        throw DmException(DMLITE_CFGERR(DMLITE_NO_CATALOG), "UgrConnector initialization failed.");

    return new UgrCatalog();
}

static void registerPluginUgr(PluginManager* pm) {
    UgrFactory *f = new UgrFactory();

    try {
	Info(UgrLogger::Lvl0, "registerPluginUgr", "Registering Ugr Catalog Factory");
        pm->registerCatalogFactory(f);
    } catch (DmException e) {
        //        if (e.code() == DM_NO_FACTORY)
        //            throw DmException(DM_NO_FACTORY, std::string("UgrDMLite can not be loaded first"));
        //        throw;
    }

    try {
	Info(UgrLogger::Lvl0, "registerPluginUgr", "Registering Ugr Authn Factory");
        pm->registerAuthnFactory(f);
    } catch (DmException e) {
        //        if (e.code() == DM_NO_FACTORY)
        //            throw DmException(DM_NO_FACTORY, std::string("UgrDMLite can not be loaded first"));
        //        throw;
    }

    try {
	Info(UgrLogger::Lvl0, "registerPluginUgr", "Registering Ugr PoolManager Factory");
        pm->registerPoolManagerFactory(f);
    } catch (DmException e) {
        //        if (e.code() == DM_NO_FACTORY)
        //            throw DmException(DM_NO_FACTORY, std::string("UgrDMLite can not be loaded first"));
        //        throw;
    }

}






/// Checks the permissions inside a given func
/// throws an exception if not allowed
void checkperm(const char *fname, UgrConnector *ugr, const SecurityCredentials &c, char *reqresource, char reqmode) {
  
  // Now build the keyvalue pairs (sob)
  std::vector< std::pair<std::string, std::string> > keyvals;
  std::vector< std::string > kk = c.getKeys();
  for (unsigned int i = 0; i < kk.size(); i++) {
    
    // Get the key
    std::string k = kk[i];
    if (!k.length()) continue;
    
    // Get the value, maliciously
    std::string v;
    try {
      v = c.getString(k, "");
    } catch ( ... ) {
      v.clear();
    }
    
    std::pair<std::string, std::string> pp;
    pp.first = k;
    pp.second = v;
    keyvals.push_back( pp );
  }
  
  if ( ugr->checkperm(fname, c.clientName, c.remoteAddress, c.fqans, keyvals, reqresource, reqmode) ) {

    // Not allowed. Throw a damn exception

    std::ostringstream ss;
    ss << "Unauthorized operation " << reqmode << " on " << reqresource;
    ss << " ClientName: " << c.clientName << " Addr:" << c.remoteAddress << " fqans: ";
    for (unsigned int i = 0; i < c.fqans.size(); i++ ) {
      ss << c.fqans[i];
      if (i < c.fqans.size() - 1) ss << ",";
    }
    
    if (keyvals.size() > 0) {
      ss << " Other keys: ";
      for (unsigned int i = 0; i < keyvals.size(); i++ ) {
        ss << "'" << keyvals[i].first << "':'" << keyvals[i].second << "'";
        if (i < keyvals.size() - 1) ss << ",";
      }
    } else {
      ss << " Other keys: (none)";
    }

    throw DmException(EACCES, ss.str());
  }

}



/// This is what the PluginManager looks for
PluginIdCard plugin_ugr = {
    API_VERSION,
    registerPluginUgr
};

void UgrCatalog::setSecurityContext(const SecurityContext *c) {
    secCredentials = c->credentials;
}

UgrCatalog::UgrCatalog() : DummyCatalog(NULL) {

}

UgrCatalog::~UgrCatalog()  {
    // Nothing
}

std::string UgrCatalog::getImplId() const throw () {
    return std::string("UgrCatalog");
}

std::vector<Replica> UgrCatalog::getReplicas(const std::string &path) {
    std::vector<Replica> replicas;

    // Get all of them
    UgrFileInfo *nfo = 0;

    std::string abspath = getAbsPath(const_cast<std::string&> (path));


    checkperm("UgrCatalog::getReplicas", getUgrConnector(), this->secCredentials, (char *)abspath.c_str(), 'r');

    if (!getUgrConnector()->locate((std::string&)abspath,
                                   UgrClientInfo(secCredentials.remoteAddress),
				   &nfo) && nfo) {

        UgrClientInfo info(secCredentials.remoteAddress);
        Info(UgrLogger::Lvl3, "UgrCatalog::getReplicas", "UgrDmlite Client remote address (" << info.ip << ")");
        UgrReplicaVec reps;
        nfo->getReplicaList(reps);
        getUgrConnector()->filterAndSortReplicaList(reps, info);

        for (UgrReplicaVec::iterator i = reps.begin(); i != reps.end(); ++i) {
            // Populate the vector
            Replica r;

            r.fileid = 0;
            r.replicaid = 0;
            r.status = Replica::kAvailable;

            // We need to get the server from the full url that we have
            r.rfn = i->name;

            // Look for ://, then for the subsequent / or :
            size_t p1;
            p1 = i->name.find("://");
            //Info(UgrLogger::Lvl3, "UgrCatalog::getReplicas000", p1 << " " <<  i->name.npos);
            if (p1 != i->name.npos) {
                //Info(UgrLogger::Lvl3, "UgrCatalog::getReplicas00", r.server);
                // get the name of the server
                size_t p2 = i->name.find_first_of(":/", p1 + 3);
                if (p2 != std::string::npos) {
                    r.server = i->name.substr(p1 + 3, p2 - p1 - 3);
                    //Info(UgrLogger::Lvl3, "UgrCatalog::getReplicas0", r.server);
                }


            }


//            // Fix the rest of the url, eliminate double slashes
//            size_t pslh = 10;
//
//            for (pslh = 10; pslh < r.rfn.size() - 1; pslh++) {
//
//                if ((r.rfn[pslh] == '/') && (r.rfn[pslh + 1] == '/')) {
//                    Info(UgrLogger::Lvl3, "UgrCatalog::getReplicas1", r.rfn << "-" << r.server << "-" << pslh);
//                    r.rfn.erase(pslh + 1, 1);
//                    Info(UgrLogger::Lvl3, "UgrCatalog::getReplicas2", r.rfn << " " << r.server << " " << pslh);
//                }
//
//
//
//            };

            Info(UgrLogger::Lvl3, "UgrCatalog::getReplicas", r.rfn << " " << r.server << " " << i->location << " " << i->latitude << " " << i->longitude);
            replicas.push_back(r);
        }


    } else {
        Info(UgrLogger::Lvl1, "UgrCatalog::getReplicas", "Failure in get location. " << path);
    }

    // Return
    if (replicas.size() == 0) {
        Info(UgrLogger::Lvl1, "UgrCatalog::getReplicas", "No endpoints have replicas of this file. " << path);
        throw DmException(DMLITE_NO_REPLICAS, "No active endpoints have replicas of this file now. " + path);
    }

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

DmStatus UgrCatalog::extendedStat(dmlite::ExtendedStat& st, const std::string& path, bool followsym) {
    UgrFileInfo *nfo = 0;
    std::string abspath = getAbsPath(const_cast<std::string&> (path));
    //checkperm("UgrCatalog::extendedStat", getUgrConnector(), this->secCredentials, (char *)abspath.c_str(), 'r');

    if (!getUgrConnector()->stat((std::string&)abspath, UgrClientInfo(secCredentials.remoteAddress), &nfo) &&
        nfo &&
        (nfo->getStatStatus() == nfo->Ok) ) {

        st.csumtype[0] = '\0';
        st.csumvalue[0] = '\0';
        st.guid[0] = '\0';
        st.name = nfo->name;
        st.name[sizeof (st.name) - 1] = '\0';
        st.parent = 0;
        st.status = ExtendedStat::kOnline;

        fillstat(st.stat, nfo);

        return DmStatus();
    }
    return DmStatus(ENOENT, "File not found");
}

dmlite::ExtendedStat UgrCatalog::extendedStat(const std::string& path, bool followsym) {
    ExtendedStat ret;
    DmStatus st = this->extendedStat(ret, path, followsym);
    if(!st.ok()) throw st.exception();
    return ret;
}

void UgrCatalog::unlink(const std::string& path){

    UgrReplicaVec vl;

    std::string abspath = getAbsPath(const_cast<std::string&> (path));

    checkperm("UgrCatalog::unlink", getUgrConnector(), this->secCredentials, (char *)abspath.c_str(), 'd');

    UgrCode ret_code = getUgrConnector()->remove(abspath,
                                                 UgrClientInfo(secCredentials.remoteAddress),
                                                 vl );

    switch(ret_code.getCode()){
        case UgrCode::Ok:{
            return;
        }

        case UgrCode::FileNotFound:{
           throw DmException(ENOENT, "File not found or not available");
        }

        case UgrCode::PermissionDenied:{
          throw DmException(EPERM, "Permission Denied. You are not allowed to execute this operation on the resource");
        }

        default:{
            throw DmException(DMLITE_MALFORMED, "Error during unlink operation, Canceled");
        }
    }
}



void UgrCatalog::makeDir(const std::string& path, mode_t mode){
  
  UgrReplicaVec vl;
  
  std::string abspath = getAbsPath(const_cast<std::string&> (path));
  UgrCode ret_code = getUgrConnector()->makeDir(abspath,
                                                  UgrClientInfo(secCredentials.remoteAddress));
  
  switch(ret_code.getCode()){
    case UgrCode::Ok:{
      return;
    }
    
    case UgrCode::FileNotFound:{
      throw DmException(ENOENT, "File not found or not available");
    }
    
    case UgrCode::PermissionDenied:{
      throw DmException(EPERM, "Permission Denied. You are not allowed to execute this operation on the resource");
    }
    
    default:{
      throw DmException(DMLITE_MALFORMED, "Error during unlink operation, Canceled");
    }
  }
}


void UgrCatalog::removeDir(const std::string& path){

    UgrReplicaVec vl;

    std::string abspath = getAbsPath(const_cast<std::string&> (path));
    UgrCode ret_code = getUgrConnector()->removeDir(abspath,
                                                 UgrClientInfo(secCredentials.remoteAddress),
                                                 vl );

    switch(ret_code.getCode()){
        case UgrCode::Ok:{
            return;
        }

        case UgrCode::FileNotFound:{
           throw DmException(ENOENT, "File not found or not available");
        }

        case UgrCode::PermissionDenied:{
          throw DmException(EPERM, "Permission Denied. You are not allowed to execute this operation on the resource");
        }

        default:{
            throw DmException(DMLITE_MALFORMED, "Error during unlink operation, Canceled");
        }
    }
}


class myDirectory {
  public:
    UgrFileInfo *nfo;
    std::set<UgrFileItem>::iterator idx;

    std::string origpath;

    dmlite::ExtendedStat buf;
    struct dirent direntbuf;

    myDirectory(UgrFileInfo *finfo, std::string unxlatedpath) : nfo(finfo), origpath(unxlatedpath) {
        idx = finfo->subdirs.begin();
        buf.clear();

        memset(&direntbuf, 0, sizeof (direntbuf));
    }

};

Directory* UgrCatalog::openDir(const std::string &path) {
    UgrFileInfo *fi;

    std::string abspath = getAbsPath(const_cast<std::string&> (path));
    checkperm("UgrCatalog::openDir", getUgrConnector(), this->secCredentials, (char *)abspath.c_str(), 'l');

    if (!getUgrConnector()->list((std::string&)abspath,
				 UgrClientInfo(secCredentials.remoteAddress),
				 &fi)
	&& fi) {

        if (fi->getItemsStatus() == UgrFileInfo::Ok) {
            boost::lock_guard<UgrFileInfo > l(*fi);
            fi->pin();
            // This is just an opaque pointer, we can store what we want
            return (Directory *) (new myDirectory(fi, abspath));
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

void UgrCatalog::closeDir(Directory *opaque) {
    myDirectory *d = (myDirectory *) opaque;

    if (d && d->nfo) {
        boost::lock_guard<UgrFileInfo > l(*d->nfo);
        d->nfo->unpin();
        delete d;
    }
}

struct dirent* UgrCatalog::readDir(Directory *opaque) {
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

dmlite::ExtendedStat* UgrCatalog::readDirx(Directory *opaque) {
    myDirectory *d = (myDirectory *) opaque;
    std::string s;
    bool trynext;

    if (!opaque) return 0;
    if (!d->nfo) return 0;

    do {
      trynext = false;

      {
        boost::lock_guard<UgrFileInfo > l(*d->nfo);
        d->nfo->touch();
        s = d->origpath;
        if (d->idx == d->nfo->subdirs.end()) return 0;

        // Only the name is relevant here, it seems
        d->buf.name = (d->idx)->name;
        d->idx++;
      }


      if (*s.rbegin() != '/')
        s += "/";

      s += d->buf.name;

      try {
        d->buf.stat = extendedStat(s, true).stat;
      }
      catch (DmException e) {
        Info(UgrLogger::Lvl1, "UgrCatalog::readDirx", "No stat information for '" << s << "'");
        trynext = true;
      }

    } while (trynext);



    return &(d->buf);
}

void UgrCatalog::changeDir(const std::string &d) {
    workingdir = d;
    UgrFileInfo::trimpath(workingdir);
}

std::string UgrCatalog::getWorkingDir() {
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

dmlite::SecurityContext* UgrAuthn::createSecurityContext(const SecurityCredentials &c) {

  std::ostringstream ss;
  ss << "ClientName: " << c.clientName << " Addr:" << c.remoteAddress << " fqans: ";
  for (unsigned int i = 0; i < c.fqans.size(); i++ ) {
    ss << c.fqans[i];
    if (i < c.fqans.size() - 1) ss << ",";
  }
  std::vector<std::string> vs = c.getKeys();
  if (vs.size() > 0) {
    ss << " Other keys: ";
    for (unsigned int i = 0; i < vs.size(); i++ ) {
      ss << vs[i];
      if (i < vs.size() - 1) ss << ",";
    }
  }



  Info(UgrLogger::Lvl1, "UgrAuthn::createSecurityContext", ss.str());


  return new dmlite::SecurityContext(c, userinfo, groupinfo);

}


UserInfo UgrAuthn::getUser(const std::string& userName)
{
  UserInfo   user;



    user.name      = userName;
    user["ca"]     = std::string();
    user["banned"] = 0;
    user["uid"]    = 0u;

  Info(UgrLogger::Lvl3, "UgrAuthn::getUser", "usr:" << userName);

  return user;
}





GroupInfo UgrAuthn::getGroup(const std::string& groupName)
{
  GroupInfo group;


  Info(UgrLogger::Lvl3, "UgrAuthn::getGroup", "group:" << groupName);

  group.name      = groupName;
  group["gid"]    = 0;
  group["banned"] = 0;

  Info(UgrLogger::Lvl3, "UgrAuthn::getGroup", "Exiting. group:" << groupName);

  return group;
}



void UgrAuthn::getIdMap(const std::string& userName,
                          const std::vector<std::string>& groupNames,
                          UserInfo* user,
                          std::vector<GroupInfo>* groups)
{
  std::string vo;
  GroupInfo   group;

  Info(UgrLogger::Lvl3, "UgrAuthn::getIdMap", "usr:" << userName);

  // Clear
  groups->clear();

  // User mapping
  *user = this->getUser(userName);

  // No VO information, so use the mapping file to get the group
  if (groupNames.empty()) {
    group = this->getGroup("dummy");
    groups->push_back(group);
  }
  else {
    // Get group info
    std::vector<std::string>::const_iterator i;
    for (i = groupNames.begin(); i != groupNames.end(); ++i) {
      group = this->getGroup(*i);
      groups->push_back(group);
    }
  }

  Info(UgrLogger::Lvl3, "UgrAuthn::getIdMap", "Exiting. usr:" << userName);
}



dmlite::SecurityContext* UgrAuthn::createSecurityContext(void) {
  Info(UgrLogger::Lvl1, "UgrAuthn::createSecurityContext", "Creating dummy");

  UserInfo user;
  std::vector<GroupInfo> groups;
  GroupInfo group;

  user.name    = "root";
  user["uid"]  = 0;
  group.name   = "root";
  group["gid"] = 0;
  groups.push_back(group);

  SecurityContext* sec = new SecurityContext(SecurityCredentials(), user, groups);
  Info(UgrLogger::Lvl1, "UgrAuthn::createSecurityContext", SecurityCredentials().clientName << " " << SecurityCredentials().remoteAddress);

  return sec;
}



// --------------------- PoolManager


UgrPoolManager::UgrPoolManager(UgrFactory* factory):
  si_(NULL), factory_(factory), secCtx_(NULL)
{
  Info(UgrLogger::Lvl4, "UgrPoolManager::UgrPoolManager", "Ctor");


}


UgrPoolManager::~UgrPoolManager()
{
  Info(UgrLogger::Lvl4, "UgrPoolManager::~UgrPoolManager", "Dtor");
}


std::string UgrPoolManager::getImplId() const throw()
{
  return std::string("Ugr");
}


void UgrPoolManager::setStackInstance(StackInstance* si)
{
  // Nothing
  this->si_ = si;
}


void UgrPoolManager::setSecurityContext(const SecurityContext* ctx)
{
  Info(UgrLogger::Lvl4, "UgrPoolManager::setSecurityContext", "Entering");

  if (!ctx) {
    Info(UgrLogger::Lvl4, "UgrPoolManager::setSecurityContext", "Context is null. Exiting.");
    return;
  }

  this->secCtx_ = ctx;

  Info(UgrLogger::Lvl3, "UgrPoolManager::setSecurityContext", "Exiting.");
}


std::vector<Pool> UgrPoolManager::getPools(PoolAvailability availability)
{
  Info(UgrLogger::Lvl4, "UgrPoolManager::getPools", " PoolAvailability: " << availability);

  std::vector<Pool> pools;
  return pools;
}


Pool UgrPoolManager::getPool(const std::string& poolname)
{
  Info(UgrLogger::Lvl4, "UgrPoolManager::getPool", " PoolName: " << poolname);
  Pool p;
  p.name = poolname;
  return p;
}



Location UgrPoolManager::whereToRead(const std::string& path)
{
  Info(UgrLogger::Lvl4, "UgrPoolManager::whereToRead", " Path: " << path);


  // This will throw an exception if no file
  std::vector<Replica> r = si_->getCatalog()->getReplicas(path);

  Chunk single( r[0].rfn, 0, 1234);


  Info(UgrLogger::Lvl3, "UgrPoolManager::whereToRead", " Path: " << path << " --> " << single.toString());
  return Location(1, single);

}



Location UgrPoolManager::whereToRead(ino_t)
{
  throw DmException(DMLITE_SYSERR(ENOSYS),
                    "UgrPoolManager: Access by inode not supported");
}



Location UgrPoolManager::whereToWrite(const std::string& path)
{
  Info(UgrLogger::Lvl4, "UgrPoolManager::whereToWrite", " path:" << path);
  UgrReplicaVec vl;

  checkperm("UgrPoolManager::whereToWrite", UgrCatalog::getUgrConnector(), secCtx_->credentials, (char *)path.c_str(), 'w');

  off64_t reqsz = 0;
  try {
    reqsz = Extensible::anyToU64(this->si_->get("requested_size"));
  } catch ( ... ) {}
  
  UgrClientInfo cnfo(secCtx_->credentials.remoteAddress);
  try {
    cnfo.s3uploadid = Extensible::anyToString(this->si_->get("x-s3-uploadid"));
  } catch ( ... ) {}
  
  try {
    cnfo.s3uploadpluginid = Extensible::anyToLong(this->si_->get("x-ugrpluginid"));
  } catch ( ... ) {}
  
  try {
    cnfo.nchunks = Extensible::anyToLong(this->si_->get("x-s3-upload-nchunks"));
  } catch ( ... ) {}
  
  UgrCode code = UgrCatalog::getUgrConnector()->findNewLocation(
    path,
    reqsz,
    cnfo,
    vl );

  if(!code.isOK()){
      throw DmException(DMLITE_SYSERR(ugrToDmliteErrCode(code)), code.getString());
  }
  
  
  if (vl.size() > 0) {
    Location loc;
    
    for (uint16_t i = 0; i < vl.size(); i++) {
      Chunk ck( vl[i].name, 0, 1);
      /// Some implementations need to pass two urls per chunk, e.g. one for PUT and one for POST
      ck.url_alt = vl[i].alternativeUrl;
      char buf[32];
      sprintf(buf, "%d", vl[i].pluginID);
      ck.chunkid = buf;
      loc.push_back(ck);
    }
    
    // Note that we pass a full URL here, already translated for usage into
    // a particular endpoint
    // This is to avoid creating empty directories in all the remote endpoints
    // Only the endpoints that match the destination of the write will receive the directory
    // This is needed because accessing individual plugins from here would be highly problematic
    UgrCatalog::getUgrConnector()->mkDirMinusPonSiteFN(vl[0].name);
    
    
    // Done!
    Info(UgrLogger::Lvl3, "UgrPoolManager::whereToWrite", "Exiting. nlocations: " << vl.size() << " first loc:" << vl[0].name);
    return loc;
  }

  Error("UgrPoolManager::whereToWrite", " Didn't get a destination from writing : " << path);
  throw DmException(DMLITE_SYSERR(ENOENT),
		    "Didn't get a destination for writing : %s", path.c_str());
}


DmStatus UgrPoolManager::fileCopyPush(const std::string& localsrcpath, const std::string &remotedesturl, int cksumcheck, char *cksumtype, dmlite_xferinfo *progressdata) {
  
  Info(Logger::Lvl2, "UgrPoolManager", "Requesting or checking file push. chksumcheck: " << cksumcheck << " chksumtype: '" << cksumtype << "' src: '" << localsrcpath << "' dest: '" << remotedesturl << "'");
  // If not already running, start the copy job using the TaskExec, as a local process
  
  // We don't have a ticker to give life to this sad API, so we do what we can
  // to avoid bloating it with one more thread
  dmTaskExec::tick();
  
  // Use the stackinstance as a context for repeated calls of this function
  int id = 0;
  if (si_->contains("ugr-3cp-taskid"))
    id = boost::any_cast<int>(si_->get("ugr-3cp-taskid"));
  
  if (id <= 0) {
    
    
    checkperm("UgrPoolManager::filePush", UgrCatalog::getUgrConnector(), si_->getSecurityContext()->credentials, (char *)localsrcpath.c_str(), 'r');
    
    // We have the local LFN to be copied from. This needs to be located by Ugr and used
    // as a source for the copy
    // Note that a file not found will be signalled by an exception here, which will reach the caller
    Location copysrc = whereToRead(localsrcpath);
    
    std::vector<std::string> params;
    std::string x509proxypath;
    if (si_->contains("x509_delegated_proxy_path")) {
      // Beware, this info may come from the frontend, e.g. lcgdm-dav
      x509proxypath = boost::any_cast<std::string>(si_->get("x509_delegated_proxy_path"));
    }
    
    Info(Logger::Lvl1, "UgrPoolManager", "Starting file push. chksumcheck: " << cksumcheck << " chksumtype: '" << cksumtype << "' src: '" << copysrc[0].url.toString() << "' dest: '" << remotedesturl << "' proxy: '" << x509proxypath << "'");
    
    params.push_back(UgrCFG->GetString("glb.filepushhook", (char *)"/usr/bin/ugr-filepush"));
    params.push_back(boost::lexical_cast<std::string>(cksumcheck));
    if ((!cksumtype) || (!cksumtype[0]))
      params.push_back("<nochecksumtype>");
    else
      params.push_back(cksumtype);
    params.push_back(copysrc[0].url.toString());
    params.push_back(remotedesturl);
    params.push_back(x509proxypath);
    
    
    // pass any other interesting parameter, e.g. a transfer bearer token to be passed to source or dest or both
    
    // Surely we pass the Authentication header
    std::string hdrline;
    const dmlite::SecurityContext *secctx = si_->getSecurityContext();
    if (secctx) {
      hdrline = secctx->credentials.getString("http.Authentication", "");
      if (hdrline.size()) {
        Info(Logger::Lvl4, "UgrPoolManager", "Passing Authentication header to file pusher: '" << hdrline << "'");
        
        params.push_back(hdrline);
      }
      else
        params.push_back("");
      
      
      // if the stackinstance contains all the keys passed by Apache, give a
      // configfile-based way to pass selected items
      int i = 0;
      do {
        char buf[1024];
        UgrCFG->ArrayGetString("glb.filepush.header2params", buf, i);
        if (!buf[0]) break;
        
        std::string hdrkey = "http.";
        hdrkey += buf;
        
        hdrline = secctx->credentials.getString(hdrkey, "");
        if (hdrline.size()) {
          Info(Logger::Lvl4, "UgrPoolManager", "Passing '" << buf << "' header to file pusher: '" << hdrline << "'");
          params.push_back(hdrline);
        }
        else
          params.push_back("");
        
        ++i;
      } while (1);
      
    }
    else
      params.push_back("");
    
    
    id = this->submitCmd(params);
    
    if(id < 0) {
      return DmStatus(500, SSTR("An error occured - unable to initiate file push."));
    }
    
    this->goCmd(id);
    boost::any to_append = id;
    si_->set("ugr-3cp-taskid", to_append);
    
    
  }
  
  // Wait for the job to finish, max 10 secs
  int runres = waitResult(id, 1);
  
  // finished or not yet, let's pick the current task output
  std::string myout;
  if (getTaskStdout(id, myout)) 
    return DmStatus(500, SSTR("An error occured - unable to retrieve job output."));
  
  
  // If the additional stdout is parsable as a GFAL/FTS performance marker,
  // do it and fill the struct. Parse only what has not yet been parsed
  int stdoutprocessed = 0;
  
  if (si_->contains("ugr-3cp-stdoutprocessed"))
    stdoutprocessed = boost::any_cast<int>(si_->get("ugr-3cp-stdoutprocessed"));
  
  const char *p = strstr(myout.c_str()+stdoutprocessed, "monitor: ");
  
  if (p) {
    const char *myfmt = "monitor: %s %s %f %ld %ld %ld";
    char buf1[1024], buf2[1024];
    long elapsed, inst, xferred;
    float avg;
    int nfields = sscanf(p, myfmt, buf1, buf2, &avg, &inst, &xferred, &elapsed);
    if (nfields == 6) {
      
      // Yep, we got a perf marker, log it
      //       Info(UgrLogger::Lvl2, "UgrPoolManager",
      //            SSTR("Got perf marker. Timestamp: " << progressdata->timestamp <<
      //            " stripe idx: " << progressdata->stripeindex <<
      //            " stripe bytes: " << progressdata->stripexferred <<
      //            " stripe totcount: " << progressdata->totstripecount <<
      //            " localsrc: '" << localsrcpath <<
      //            "' remotedest: '" << remotedesturl << "'"));
      
      Info(UgrLogger::Lvl2, "UgrPoolManager",
           SSTR("Got perf marker. src: '" << buf1 << "' dst: '" << buf2 <<
           " avg: " << avg << " inst: " << inst <<
           " xferred: " << xferred <<
           " elapsed: " << elapsed));
      
      progressdata->stripexferred = xferred;
      progressdata->stripeindex = 0;
      progressdata->timestamp = time(0);
      progressdata->totstripecount = 1;
      
      
    }
    
    // Anyway advance the last parsed position
    boost::any to_append = (int)((p-myout.c_str()) + strlen(myfmt));
    si_->set("ugr-3cp-stdoutprocessed", to_append);
    
  } else {
    // Anyway advance the last parsed position
    int app = myout.length()-10;
    
    if (app > 0) {
      boost::any to_append = (int)myout.length();
      si_->set("ugr-3cp-stdoutprocessed", to_append);
    }
    
    
  }
  
  // Send OK or EAGAIN, with the performance marker filled
  Info(UgrLogger::Lvl4, "UgrPoolManager",
       SSTR("runres: " << runres));
//   
  if (!runres) {
    // finished, let's see if it was success or failure
    dmTask *t = getTask(id);
    if (!t)
      return DmStatus(500, SSTR("Can't find my task id " << id << " Internal error"));
    
    if (!t->resultcode)
      return DmStatus();
    
    // If it looks like an HTTP number let's forward the original result code
    if ((t->resultcode >= 200) && (t->resultcode < 600))
      return DmStatus(t->resultcode, SSTR("Task id " << id << " failed copying '" << localsrcpath << "' to '" << remotedesturl << "' result code: " << t->resultcode));
    
    // Otherwise 422 will blame the user. HE gave bad URLs, not us!
    return DmStatus(422, SSTR("Task id " << id << " failed copying '" << localsrcpath << "' to '" << remotedesturl << "' result code: " << t->resultcode));
  }
  
  return DmStatus(EAGAIN, SSTR("Task id " << id << " has not yet finished"));
  
}

DmStatus UgrPoolManager::fileCopyPull(const std::string& localdestpath, const std::string &remotesrcurl, int cksumcheck, char *cksumtype, dmlite_xferinfo *progressdata) {
  
  Info(Logger::Lvl2, "UgrPoolManager", "Requesting or checking file pull. chksumcheck: " << cksumcheck << " chksumtype: '" << cksumtype << "' src: '" << remotesrcurl << "' dest: '" << localdestpath << "'");
  // If not already running, start the copy job using the TaskExec, as a local process
  
  // We don't have a ticker to give life to this sad API, so we do what we can
  // to avoid bloating it with one more thread
  dmTaskExec::tick();
  
  // Use the stackinstance as a context for repeated calls of this function
  int id = 0;
  if (si_->contains("ugr-3cp-taskid"))
    id = boost::any_cast<int>(si_->get("ugr-3cp-taskid"));
  
  if (id <= 0) {
    
    // Beware, we need to write into the local SE
    checkperm("UgrPoolManager::filePull", UgrCatalog::getUgrConnector(), si_->getSecurityContext()->credentials, (char *)localdestpath.c_str(), 'w');
    
    // We have the local LFN to be copied to. This needs to be located by Ugr and used
    // as a dest for the copy
    // Note that a file not found will be signalled by an exception here, which will reach the caller
    Location copylocaldest = whereToWrite(localdestpath);
    
    std::vector<std::string> params;
    std::string x509proxypath;
    if (si_->contains("x509_delegated_proxy_path")) {
      // Beware, this info may come from the frontend, e.g. lcgdm-dav
      x509proxypath = boost::any_cast<std::string>(si_->get("x509_delegated_proxy_path"));
    }
    
    Info(Logger::Lvl1, "UgrPoolManager", "Starting file pull. chksumcheck: " << cksumcheck << " chksumtype: '" << cksumtype << "' src: '" << remotesrcurl << "' dest: '" << copylocaldest[0].url.toString() << "' proxy: '" << x509proxypath << "'");
    
    params.push_back(UgrCFG->GetString("glb.filepullhook", (char *)"/usr/bin/ugr-filemover"));
    params.push_back(boost::lexical_cast<std::string>(cksumcheck));
    if ((!cksumtype) || (!cksumtype[0]))
      params.push_back("<nochecksumtype>");
    else
      params.push_back(cksumtype);
    params.push_back(remotesrcurl);
    params.push_back(copylocaldest[0].url.toString());
    params.push_back(x509proxypath);
    
    // pass any other interesting parameter, e.g. a transfer bearer token to be passed to source or dest or both
    
    // Surely we pass the Authentication header
    std::string hdrline;
    const dmlite::SecurityContext *secctx = si_->getSecurityContext();
    if (secctx) {
      hdrline = secctx->credentials.getString("http.Authentication", "");
      if (hdrline.size()) {
        Info(Logger::Lvl4, "UgrPoolManager", "Passing Authentication header to file pusher: '" << hdrline << "'");
        params.push_back(hdrline);
      }
        else
          params.push_back("");

    
    // if the stackinstance contains all the keys passed by Apache, give a
    // configfile-based way to pass selected items
    int i = 0;
    do {
      char buf[1024];
      UgrCFG->ArrayGetString("glb.filepull.header2params", buf, i);
      if (!buf[0]) break;
      
      std::string hdrkey = "http.";
      hdrkey += buf;
      
      hdrline = secctx->credentials.getString(hdrkey, "");
      if (hdrline.size()) {
        Info(Logger::Lvl4, "UgrPoolManager", "Passing '" << buf << "' header to file puller: '" << hdrline << "'");
        params.push_back(hdrline);
      }
      else
        params.push_back("");
      
      ++i;
    } while (1);
    
    }
    else
      params.push_back("");
    
    
    id = this->submitCmd(params);
    
    if(id < 0) {
      return DmStatus(500, SSTR("An error occured - unable to initiate file pull."));
    }
    
    this->goCmd(id);
    boost::any to_append = id;
    si_->set("ugr-3cp-taskid", to_append);
    
    
  }
  
  // Wait for the job to finish, max 10 secs
  int runres = waitResult(id, 1);
  
  // finished or not yet, let's pick the current task output
  std::string myout;
  if (getTaskStdout(id, myout)) 
    return DmStatus(500, SSTR("An error occured - unable to retrieve job output."));
  
  
  // If the additional stdout is parsable as a GFAL/FTS performance marker,
  // do it and fill the struct. Parse only what has not yet been parsed
  int stdoutprocessed = 0;
  
  if (si_->contains("ugr-3cp-stdoutprocessed"))
    stdoutprocessed = boost::any_cast<int>(si_->get("ugr-3cp-stdoutprocessed"));
  
  const char *p = strstr(myout.c_str()+stdoutprocessed, "monitor: ");
  
  if (p) {
    const char *myfmt = "monitor: %s %s %f %ld %ld %ld";
    char buf1[1024], buf2[1024];
    long elapsed, inst, xferred;
    float avg;
    int nfields = sscanf(p, myfmt, buf1, buf2, &avg, &inst, &xferred, &elapsed);
    if (nfields == 6) {
      
      // Yep, we got a perf marker, log it
      //       Info(UgrLogger::Lvl2, "UgrPoolManager",
      //            SSTR("Got perf marker. Timestamp: " << progressdata->timestamp <<
      //            " stripe idx: " << progressdata->stripeindex <<
      //            " stripe bytes: " << progressdata->stripexferred <<
      //            " stripe totcount: " << progressdata->totstripecount <<
      //            " localsrc: '" << localsrcpath <<
      //            "' remotedest: '" << remotedesturl << "'"));
      
      Info(UgrLogger::Lvl2, "UgrPoolManager",
           SSTR("Got perf marker. src: '" << buf1 << "' dst: '" << buf2 <<
           " avg: " << avg << " inst: " << inst <<
           " xferred: " << xferred <<
           " elapsed: " << elapsed));
      
      progressdata->stripexferred = xferred;
      progressdata->stripeindex = 0;
      progressdata->timestamp = time(0);
      progressdata->totstripecount = 1;
      
      
    }
    
    // Anyway advance the last parsed position
    boost::any to_append = (int)((p-myout.c_str()) + strlen(myfmt));
    si_->set("ugr-3cp-stdoutprocessed", to_append);
    
  } else {
    // Anyway advance the last parsed position
    int app = myout.length()-10;
    
    if (app > 0) {
      boost::any to_append = (int)myout.length();
      si_->set("ugr-3cp-stdoutprocessed", to_append);
    }
    
    
  }
  
  // Send OK or EAGAIN, with the performance marker filled
  Info(UgrLogger::Lvl4, "UgrPoolManager",
       SSTR("runres: " << runres));
  //   
  if (!runres) {
    // finished, let's see if it was success or failure
    dmTask *t = getTask(id);
    if (!t)
      return DmStatus(500, SSTR("Can't find my task id " << id << " Internal error"));
    
    if (!t->resultcode)
      return DmStatus();
    
    // If it looks like an HTTP number let's forward the original result code
    if ((t->resultcode >= 200) && (t->resultcode < 600))
      return DmStatus(t->resultcode, SSTR("Task id " << id << " failed pulling '" << remotesrcurl << "' to '" << localdestpath << "' result code: " << t->resultcode));
    
    // Otherwise 422 will blame the user. HE gave bad URLs, not us!
    return DmStatus(422, SSTR("Task id " << id << " failed pulling '" << remotesrcurl << "' to '" << localdestpath << "' result code: " << t->resultcode));
  }
  
  return DmStatus(EAGAIN, SSTR("Task id " << id << " has not yet finished"));
}


/// Event invoked internally to log stuff
void UgrPoolManager::onLoggingRequest(Logger::Level lvl, std::string const & msg) {
  Info(lvl, "UgrPoolManager", msg);
}


/// Event invoked internally to log stuff
void UgrPoolManager::onErrLoggingRequest(std::string const & msg) {
  Error("UgrPoolManager", msg);
}


#ifndef UGRDMLITECATALOG_HH
#define UGRDMLITECATALOG_HH





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


/// @file    plugins/dmlite/UgrDMLite.hh
/// @brief   Let dmlite use UGR as a plugin.
/// @author  Fabrizio Furano <furano@cern.ch>
/// @date    Feb 2012


#include <map>
#include <dmlite/cpp/dmlite.h>
#include <dmlite/cpp/inode.h>
#include <dmlite/cpp/dummy/DummyCatalog.h>
#include <dmlite/cpp/poolmanager.h>
#include <dmlite/cpp/catalog.h>
#include <set>
#include <boost/thread.hpp>
#include "../../UgrConnector.hh"


namespace dmlite {


    using namespace dmlite;

    /// Inherits from DummyCatalog, in order to treat there the non implemented methods

    class UgrCatalog : public DummyCatalog {
    public:
        /// Constructor
        UgrCatalog();

        /// Destructor
        ~UgrCatalog() ;

        // Overloading
        virtual std::string getImplId() const throw ();

        virtual void setSecurityContext(const dmlite::SecurityContext*);

        void set(const std::string&, va_list);

        virtual void setStackInstance(StackInstance* si) {
        };

        virtual std::vector<Replica> getReplicas(const std::string&);


        virtual DmStatus extendedStat(ExtendedStat&, const std::string&, bool);
        virtual ExtendedStat extendedStat(const std::string&, bool);

        virtual void getChecksum(const std::string& path,
                    const std::string& csumtype,
                    std::string& csumvalue,
                    const std::string& pfn, const bool forcerecalc, const int waitsecs) {
                      csumvalue = "";
                    }                    
                    
        virtual void unlink(const std::string&);

        virtual void removeDir(const std::string&);

        virtual void changeDir(const std::string&);
        
        virtual void makeDir(const std::string&, mode_t);
        
        virtual std::string getWorkingDir(void);

        virtual Directory* openDir(const std::string&);
        virtual void closeDir(Directory*);

        virtual struct dirent* readDir(Directory*);
        virtual ExtendedStat* readDirx(Directory*);

        static UgrConnector *getUgrConnector() {
            if (!UgrCatalog::conn) UgrCatalog::conn = new UgrConnector();
            return UgrCatalog::conn;
        }


    private:

        /// The instance of UGRConnector in use. Must be only one, so that the
        /// internal buffers can be shared
        static UgrConnector *conn;

        SecurityCredentials secCredentials;

        std::string workingdir;

        std::string getAbsPath(std::string &path);

    };





    /// User and group handling.

    class UgrAuthn : public Authn {
    public:

        /// String ID of the user DB implementation.

        virtual std::string getImplId() const throw () {
            return "The Ugr implementation of the UserGroupDb class.";
        };

        /// Create a security context from the credentials.
        /// @param cred The security credentials.
        virtual SecurityContext* createSecurityContext(const SecurityCredentials& cred);
	virtual SecurityContext* createSecurityContext();

        /// Create a new group.
        /// @param gname The new group name.

        virtual GroupInfo newGroup(const std::string& gname) {
            throw DmException(DMLITE_NO_USER_MAPPING, std::string("Not supported on a federation."));
        };

	virtual GroupInfo getGroup(const std::string& groupName);

        virtual std::vector<dmlite::GroupInfo> getGroups() {
            throw DmException(DMLITE_NO_USER_MAPPING, std::string("Not supported on a federation."));
        };

        virtual void updateGroup(const dmlite::GroupInfo& g) {
            throw DmException(DMLITE_NO_USER_MAPPING, std::string("Not supported on a federation."));
        };

        virtual void deleteGroup(const std::string& g) {
            throw DmException(DMLITE_NO_USER_MAPPING, std::string("Not supported on a federation."));
        };

        virtual UserInfo newUser(const std::string& uname) {
            throw DmException(DMLITE_NO_USER_MAPPING, std::string("Not supported on a federation."));
        };

        virtual dmlite::UserInfo getUser(const std::string&);

        virtual std::vector<dmlite::UserInfo> getUsers() {
            throw DmException(DMLITE_NO_USER_MAPPING, std::string("Not supported on a federation."));
        };

        virtual void updateUser(const dmlite::UserInfo&) {
            throw DmException(DMLITE_NO_USER_MAPPING, std::string("Not supported on a federation."));
        };

        virtual void deleteUser(const std::string&) {
            throw DmException(DMLITE_NO_USER_MAPPING, std::string("Not supported on a federation."));
        };

        virtual void getIdMap(const std::string& userName,
                const std::vector<std::string>& groupNames,
                UserInfo* user,
                std::vector<GroupInfo>* groups);

    protected:
        SecurityCredentials cred;
        UserInfo userinfo;
        std::vector<dmlite::GroupInfo> groupinfo;

    };


  class UgrFactory;

  class UgrPoolManager: public PoolManager {
  public:
    UgrPoolManager(UgrFactory* factory);
    ~UgrPoolManager();

    std::string getImplId() const throw ();

    void setStackInstance(StackInstance* si);
    void setSecurityContext(const SecurityContext*);

    std::vector<Pool> getPools(PoolAvailability availability = kAny);
    Pool getPool(const std::string&);

    Location whereToRead (const std::string& path);
    Location whereToRead (ino_t inode)            ;
    Location whereToWrite(const std::string& path);

    virtual DmStatus filePush(const std::string& localsrcpath, const std::string &remotedesturl, int cksumcheck, char *cksumtype, dmlite_xferprogmarker *progressdata)  ;
    virtual DmStatus filePull(const std::string& localdestpath, const std::string &remotesrcurl, int cksumcheck, char *cksumtype, dmlite_xferprogmarker *progressdata)  ;
  private:

    StackInstance* si_;

    /// The corresponding factory.
    UgrFactory* factory_;
    const SecurityContext* secCtx_;
  };




    /// Concrete factory for the Librarian plugin.

    class UgrFactory : public CatalogFactory, public AuthnFactory, public PoolManagerFactory {
    public:
        /// Constructor
        UgrFactory();
        /// Destructor
        ~UgrFactory() ;

        virtual void configure(const std::string& key, const std::string& value);
        Catalog* createCatalog(CatalogFactory* factory,
                PluginManager* pm);

        Catalog* createCatalog(PluginManager* pm) {
            return createCatalog(NULL, pm);
        }

        Authn* createAuthn(PluginManager* pm) {
            return new UgrAuthn();
        };

	PoolManager* createPoolManager(PluginManager*) {
	  return new UgrPoolManager(this);

	}


    private:
        std::string cfgfile;
        bool createremoteparentdirs;
    };








}

#endif

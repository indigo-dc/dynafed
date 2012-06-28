/// @file    plugins/dmlite/UgrDMLite.hh
/// @brief   Let dmlite use UGR as a plugin.
/// @author  Fabrizio Furano <furano@cern.ch>
/// @date    Feb 2012
#ifndef UGRDMLITECATALOG_HH
#define UGRDMLITECATALOG_HH

#include <dmlite/cpp/dmlite.h>
#include <dmlite/cpp/dummy/DummyCatalog.h>
#include <set>
#include <boost/thread.hpp>
#include "../../UgrConnector.hh"

namespace dmlite {

    /// Inherits from DummyCatalog, in order to treat there the non implemented methods

    class UgrCatalog : public DummyCatalog {
    public:
        /// Constructor
        /// @param decorates The underlying decorated catalog.
        UgrCatalog(Catalog* decorates) throw (DmException);

        /// Destructor
        ~UgrCatalog() throw (DmException);

        // Overloading
        std::string getImplId(void) throw ();

        virtual void setSecurityContext(const SecurityContext*) throw (DmException);

        void set(const std::string&, va_list) throw (DmException);

        virtual void setStackInstance(StackInstance* si) throw (DmException) {};
        
        virtual std::vector<FileReplica> getReplicas(const std::string&) throw (DmException);
        
        virtual Location get(const std::string&) throw (DmException);

        virtual void getIdMap(const std::string&, const std::vector<std::string>&,
                uid_t*, std::vector<gid_t>*) throw (DmException);

        virtual struct stat stat(const std::string&) throw (DmException);
        virtual struct xstat extendedStat(const std::string&, bool) throw (DmException);

        virtual Directory* openDir(const std::string&) throw (DmException);
        virtual void closeDir(Directory*) throw (DmException);

        virtual struct dirent* readDir(Directory*) throw (DmException);
        virtual ExtendedStat* readDirx(Directory*) throw (DmException);

        static UgrConnector *getUgrConnector() {
            if (!UgrCatalog::conn) UgrCatalog::conn = new UgrConnector();
            return UgrCatalog::conn;
        }

    protected:
        void exclude(int64_t replicaId);
        bool isExcluded(int64_t replicaId);
    private:
        static std::string implId_;
        std::set<int64_t> excluded_;

        friend class UgrFactory;

        /// The instance of UGRConnector in use. Must be only one, so that the
        /// internal buffers can be shared
        static UgrConnector *conn;

        SecurityCredentials secCredentials;

    };




    /// Concrete factory for the Librarian plugin.

    class UgrFactory : public CatalogFactory {
    public:
        /// Constructor
        UgrFactory(CatalogFactory* catalogFactory) throw (DmException);
        /// Destructor
        ~UgrFactory() throw (DmException);

        virtual void configure(const std::string& key, const std::string& value) throw (DmException);
        Catalog* createCatalog(dmlite::PluginManager *) throw (DmException);
    protected:
        CatalogFactory* nestedFactory_;
    private:
        std::string cfgfile;

    };







    /// User and group handling.

    class UgrUserGroupDb : public UserGroupDb {
    public:

        /// String ID of the user DB implementation.

        virtual std::string getImplId(void) throw () {
            return "The Ugr implementation of the UserGroupDb class.";
        };

        /// Create a security context from the credentials.
        /// @param cred The security credentials.
        virtual SecurityContext* createSecurityContext(const SecurityCredentials& cred) throw (DmException);


        /// Create a new group.
        /// @param gname The new group name.
        virtual GroupInfo newGroup(const std::string& gname) throw (DmException) {
            throw DmException(DM_NO_USER_MAPPING, std::string("Not supported on a federation."));
        };

        /// Get the group name associated with a group id.
        /// @param gid The group ID.
        /// @return    The group.
        virtual GroupInfo getGroup(gid_t gid) throw (DmException) {
            throw DmException(DM_NO_USER_MAPPING, std::string("Not supported on a federation."));
        };

        /// Get the group id of a specific group name.
        /// @param groupName The group name.
        /// @return          The group.
        virtual GroupInfo getGroup(const std::string& groupName) throw (DmException) {
            throw DmException(DM_NO_USER_MAPPING, std::string("Not supported on a federation."));
        };

        /// Create a new user.
        /// @param uname The new user name.
        /// @param ca    The user Certification Authority.
        virtual UserInfo newUser(const std::string& uname, const std::string& ca) throw (DmException) {
            throw DmException(DM_NO_USER_MAPPING, std::string("Not supported on a federation."));
        };

        /// Get the name associated with a user id.
        /// @param uid      The user ID.
        virtual UserInfo getUser(uid_t uid) throw (DmException) {
            throw DmException(DM_NO_USER_MAPPING, std::string("Not supported on a federation."));
        };

        /// Get the user id of a specific user name.
        /// @param userName The user name.
        virtual UserInfo getUser(const std::string& userName) throw (DmException) {
            throw DmException(DM_NO_USER_MAPPING, std::string("Not supported on a federation."));
        };

        /// Get the mapping of a user/group.
        /// @param userName   The user name.
        /// @param groupNames The different groups. Can be empty.
        /// @param user       Pointer to an UserInfo struct where to put the data.
        /// @param groups     Pointer to a vector where the group mapping will be put.
        /// @note If groupNames is empty, grid mapfile will be used to retrieve the default group.
        virtual void getIdMap(const std::string& userName,
                const std::vector<std::string>& groupNames,
                UserInfo* user,
                std::vector<GroupInfo>* groups) throw (DmException) {

            throw DmException(DM_NO_USER_MAPPING, std::string("Not supported on a federation."));
        };
        
    };


    /// UserGroupDbFactory

    class UgrUserGroupDbFactory : public UserGroupDbFactory {
    public:

        /// Set a configuration parameter
        /// @param key   The configuration parameter
        /// @param value The value for the configuration parameter
        virtual void configure(const std::string& key, const std::string& value) throw (DmException);

        /// Instantiate a implementation of UserGroupDb
        /// @param si The StackInstance that is instantiating the context. It may be NULL.
        virtual UserGroupDb* createUserGroupDb(dmlite::PluginManager *) throw (DmException);

    protected:
    private:
    };







};

#endif



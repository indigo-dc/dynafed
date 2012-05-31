/// @file    plugins/dmlite/UgrDMLite.hh
/// @brief   Let dmlite use UGR as a plugin.
/// @author  Fabrizio Furano <furano@cern.ch>
#ifndef UGRDMLITECATALOG_HH
#define UGRDMLITECATALOG_HH

#include <dmlite/dmlite++.h>
#include <dmlite/dummy/Dummy.h>
#include <set>
#include <boost/thread.hpp>
#include "../../UgrConnector.hh"

namespace dmlite {

    /// Librarian plugin

    class UgrCatalog : public DummyCatalog {
    public:
        /// Constructor
        /// @param decorates The underlying decorated catalog.
        UgrCatalog(Catalog* decorates) throw (DmException);

        /// Destructor
        ~UgrCatalog() throw (DmException);

        // Overloading
        std::string getImplId(void) throw ();

        void set(const std::string&, va_list) throw (DmException);

        virtual std::vector<FileReplica> getReplicas(const std::string&) throw (DmException);
        virtual FileReplica get(const std::string&) throw (DmException);

        virtual void getIdMap(const std::string&, const std::vector<std::string>&,
                uid_t*, std::vector<gid_t>*) throw (DmException);

        virtual struct stat stat(const std::string&) throw (DmException);
        virtual struct xstat extendedStat(const std::string&) throw (DmException);

        virtual Directory* openDir(const std::string&) throw (DmException);
        virtual void closeDir(Directory*) throw (DmException);

        virtual struct dirent* readDir(Directory*) throw (DmException);
        virtual struct direntstat* readDirx(Directory*) throw (DmException);

        virtual void setUserId(uid_t, gid_t, const std::string&) throw (DmException);
        virtual void setVomsData(const std::string&, const std::vector<std::string>&) throw (DmException);

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

        // We use only one instance of this
        static UgrConnector *conn;
        static bool initdone;

        // User info
        std::string userdn;
        std::string voms_vo;
        std::vector<std::string> voms_fqans;
    };




    /// Concrete factory for the Librarian plugin.

    class UgrFactory : public CatalogFactory {
    public:
        /// Constructor
        UgrFactory(CatalogFactory* catalogFactory) throw (DmException);
        /// Destructor
        ~UgrFactory() throw (DmException);

        virtual void configure(const std::string& key, const std::string& value) throw (DmException);
        Catalog* createCatalog() throw (DmException);
    protected:
        CatalogFactory* nestedFactory_;
    private:
        std::string cfgfile;

    };

};

#endif



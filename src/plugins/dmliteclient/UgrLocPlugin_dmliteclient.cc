/** @file   UgrLocPlugin_dmlite.cc
 * @brief  Plugin that talks to dmlite
 * @author Fabrizio Furano
 * @date   Jan 2012
 */
#include "UgrLocPlugin_dmliteclient.hh"
#include "../../PluginLoader.hh"
#include <time.h>


using namespace boost;
using namespace std;

UgrLocPlugin_dmlite::UgrLocPlugin_dmlite(SimpleDebug *dbginstance, Config *cfginstance, std::vector<std::string> &parms) :
LocationPlugin(dbginstance, cfginstance, parms) {

    Info(SimpleDebug::kLOW, "UgrLocPlugin_dmlite", "Creating instance named " << name);

    pluginManager = 0;
    catalogfactory = 0;

    if (parms.size() > 3) {
        Info(SimpleDebug::kHIGH, "UgrLocPlugin_dmlite", "Initializing dmlite client. cfg: " << parms[3]);

        try {
            pluginManager = new dmlite::PluginManager();
            pluginManager->loadConfiguration(parms[3]);
            catalogfactory = pluginManager->getCatalogFactory();

        } catch (int e) {
            Error("UgrLocPlugin_dmlite", "Catched exception: " << e);

        }

        Info(SimpleDebug::kLOW, "UgrLocPlugin_dmlite", "Dmlite plugin manager loaded. cfg: " << parms[3]);

    }


};

void UgrLocPlugin_dmlite::runsearch(struct worktoken *op, int myidx) {
    const char *fname = "UgrLocPlugin_dmlite::runsearch";
    ExtendedStat st;
    std::vector<FileReplica> repvec;
    dmlite::Directory *d = 0;
    bool exc = false;
    std::string xname;
    dmlite::Catalog *catalog = 0;
    dmlite::StackInstance *si = 0;
    dmlite::SecurityContext secCtx;

    bool listerror = false;

    if (!pluginManager) return;
    if (!catalogfactory) return;

    // Catalog
    LocPluginLogInfoThr(SimpleDebug::kHIGH, fname, "Getting the catalogue instance");
    {

        boost::unique_lock< boost::mutex > l(dmlitemutex);

        // create stackinstance (this will instantiate the catalog)
        // invoke si->setsecuritycontext
        si = simap[myidx];

        if (!si) {

            try {
                si = new dmlite::StackInstance(pluginManager);
            } catch (dmlite::DmException e) {
                LocPluginLogErr(fname, "Cannot create StackInstance. op: " << op->wop << " name: " << xname << " Catched exception: " << e.code() << " what: " << e.what());
                exc = true;
            }

        }
    }


    LocPluginLogInfoThr(SimpleDebug::kHIGH, fname, "Got the catalogue instance.");


    if (si) {
        catalog = si->getCatalog();
        si->setSecurityContext(secCtx);
    }
    if (!catalog) {
        LocPluginLogErr(fname, "Cannot find catalog.");
        exc = true;
    }

    // We act using the identity of this service, hence we don't need to invoke
    // getIdMap/setUserblahblah

    // Now xlate the name , by applying the default xlation
    xname = op->fi->name;
    if ((xlatepfx_from.size() > 0) && ((op->fi->name.size() == 0) || (op->fi->name.compare(0, xlatepfx_from.length(), xlatepfx_from) == 0))) {

        if (op->fi->name.size() == 0)
            xname = xlatepfx_to;
        else
            xname = xlatepfx_to + op->fi->name.substr(xlatepfx_from.length());

    }

    if (!exc) {



        try {
            switch (op->wop) {

                case LocationPlugin::wop_Stat:
                    LocPluginLogInfoThr(SimpleDebug::kHIGH, fname, "invoking Stat(" << xname << ")");

                    // For now I don't see why it should not follow the links here
                    st = catalog->extendedStat(xname, true);
                    break;

                case LocationPlugin::wop_Locate:
                    LocPluginLogInfoThr(SimpleDebug::kHIGH, fname, "invoking getReplicas(" << xname << ")");
                    repvec = catalog->getReplicas(xname);
                    break;

                case LocationPlugin::wop_List:
                    LocPluginLogInfoThr(SimpleDebug::kHIGH, fname, "invoking openDir(" << xname << ")");
                    d = catalog->openDir(xname);
                    break;

                default:
                    break;
            }
        } catch (dmlite::DmException e) {
            LocPluginLogErr(fname, "op: " << op->wop << " name: " << xname << " Catched exception: " << e.code() << " what: " << e.what());
            exc = true;
        }
        LocPluginLogInfoThr(SimpleDebug::kMEDIUM, fname, "Worker: inserting data for " << op->fi->name);
    }



    //
    // Now put the results
    //



    UgrFileItem it;



    op->fi->lastupdtime = time(0);

    switch (op->wop) {


        case LocationPlugin::wop_Stat:
            if (exc) {
                //op->fi->status_statinfo = UgrFileInfo::NotFound;
                LocPluginLogInfoThr(SimpleDebug::kHIGHEST, fname, "Worker: stat not found.");
            } else
                op->fi->takeStat(st);


            break;

        case LocationPlugin::wop_Locate:
            if (exc) {
                //op->fi->status_locations = UgrFileInfo::NotFound;
                LocPluginLogInfoThr(SimpleDebug::kHIGHEST, fname, "Worker: locations not found.");
            } else {

                for (vector<FileReplica>::iterator i = repvec.begin();
                        i != repvec.end();
                        i++) {
                    it.name = i->rfn;
                    LocPluginLogInfoThr(SimpleDebug::kHIGHEST, fname, "Worker: Inserting replicas" << i->rfn);

                    // Process it with the Geo plugin, if needed
                    if (geoPlugin) geoPlugin->setReplicaLocation(it);

                    // We have modified the data, hence set the dirty flag
                    op->fi->dirtyitems = true;

                    {
                        // Lock the file instance
                        unique_lock<mutex> l(*(op->fi));

                        op->fi->subitems.insert(it);
                    }
                }




            }
            break;

        case LocationPlugin::wop_List:
            if (exc) {
                LocPluginLogInfoThr(SimpleDebug::kHIGHEST, fname, "Worker: list not found.");
                //op->fi->status_items = UgrFileInfo::NotFound;
            } else {

                ExtendedStat *dent;
                long cnt = 0;
                LocPluginLogInfoThr(SimpleDebug::kHIGHEST, fname, "Worker: Inserting list. ");
                while ((dent = catalog->readDirx(d))) {
                    // Lock the file instance
                    unique_lock<mutex> l(*(op->fi));

                    if (cnt++ > CFG->GetLong("glb.maxlistitems", 2000)) {
                        LocPluginLogInfoThr(SimpleDebug::kMEDIUM, fname, "Setting as non listable. cnt=" << cnt);
                        listerror = true;
                        op->fi->subitems.clear();
                        break;
                    }
                    it.name = dent->name;
                    it.location.clear();
                    op->fi->subitems.insert(it);

                    // We have modified the data, hence set the dirty flag
                    op->fi->dirtyitems = true;

                    // We have some info to add to the cache
                    if (op->handler) {
                        string newlfn = op->fi->name + "/" + dent->name;
                        UgrFileInfo *fi = op->handler->getFileInfoOrCreateNewOne(newlfn, false);
                        if (fi) fi->takeStat(*dent);
                    }

                }

                catalog->closeDir(d);


            }

            break;

        default:
            break;
    }

    // We have modified the data, hence set the dirty flag

    {
        // Lock the file instance
        unique_lock<mutex> l(*(op->fi));


        // Anyway the notification has to be correct, not redundant
        switch (op->wop) {

            case LocationPlugin::wop_Stat:
                op->fi->notifyStatNotPending();
                break;

            case LocationPlugin::wop_Locate:
                op->fi->status_locations = UgrFileInfo::Ok;
                op->fi->notifyLocationNotPending();
                break;

            case LocationPlugin::wop_List:
                if (listerror) {
                    op->fi->status_items = UgrFileInfo::Error;

                } else
                    op->fi->status_items = UgrFileInfo::Ok;

                op->fi->notifyItemsNotPending();
                break;

            default:
                break;
        }

    }



    {
        boost::unique_lock< boost::mutex > l(dmlitemutex);
        simap[myidx] = si;
    }


}




// ------------------------------------------------------------------------------------
// Plugin-related stuff
// ------------------------------------------------------------------------------------


// The hook function. GetLocationPluginClass must be given the name of this function
// for the plugin to be loaded

extern "C" LocationPlugin * GetLocationPlugin(GetLocationPluginArgs) {
    return (LocationPlugin *)new UgrLocPlugin_dmlite(dbginstance, cfginstance, parms);
}

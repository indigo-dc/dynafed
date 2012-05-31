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

    if (parms.size() > 3) {
        Info(SimpleDebug::kHIGH, "UgrLocPlugin_dmlite", "Initializing dmlite client. cfg: " << parms[3]);

        try {
            pluginManager = new dmlite::PluginManager();
            pluginManager->loadConfiguration(parms[3]);

        } catch (int e) {
            Error("UgrLocPlugin_dmlite", "Catched exception: " << e);
           
        }

        Info(SimpleDebug::kLOW, "UgrLocPlugin_dmlite", "Dmlite plugin manager loaded. cfg: " << parms[3]);

    }


};

void UgrLocPlugin_dmlite::runsearch(struct worktoken *op) {
    const char *fname = "UgrLocPlugin_dmlite::runsearch";
    struct stat st;
    std::vector<FileReplica> repvec;
    dmlite::Directory *d;
    bool exc = false;
    std::string xname;

    if (!pluginManager) return;

    // Catalog
    LocPluginLogInfo(SimpleDebug::kHIGH, fname, "Getting the catalogue instance");

    dmlite::Catalog *catalog = pluginManager->getCatalogFactory()->createCatalog();
    if (!catalog) {
        LocPluginLogErr(fname, "Catalog creation failed.");
        exc = true;
    }

    // We act using the identity of this service, hence we don't need to invoke
    // getIdMap/setUserblahblah

    // Now xlate the name , by applying the default xlation
    xname = op->fi->name;
    if ( (xlatepfx_from.size() > 0) && ((op->fi->name.size() == 0) || (op->fi->name.compare(0, xlatepfx_from.length(), xlatepfx_from) == 0)) ) {

        if (op->fi->name.size() == 0)
            xname = xlatepfx_to;
        else
            xname = xlatepfx_to + op->fi->name.substr(xlatepfx_from.length());

    }

    if (!exc) {
        try {
            switch (op->wop) {

                case LocationPlugin::wop_Stat:
                    LocPluginLogInfo(SimpleDebug::kHIGH, fname, "invoking Stat(" << xname << ")");
                    st = catalog->stat(xname);
                    break;

                case LocationPlugin::wop_Locate:
                    LocPluginLogInfo(SimpleDebug::kHIGH, fname, "invoking getReplicas(" << xname << ")");
                    repvec = catalog->getReplicas(xname);
                    break;

                case LocationPlugin::wop_List:
                    LocPluginLogInfo(SimpleDebug::kHIGH, fname, "invoking openDir(" << xname << ")");
                    d = catalog->openDir(xname);
                    break;

                default:
                    break;
            }
        } catch (dmlite::DmException e) {
            LocPluginLogErr(fname, "op: " << op->wop << " name: " << xname << " Catched exception: " << e.code() << " what: " << e.what());
            exc = true;
        }
        LocPluginLogInfo(SimpleDebug::kMEDIUM, fname, "Worker: inserting data for " << op->fi->name);
    }

    // Now put the results
    {
        UgrFileItem it;

        // Lock the file instance
        unique_lock<mutex> l(*(op->fi));

        op->fi->lastupdtime = time(0);

        switch (op->wop) {


            case LocationPlugin::wop_Stat:
                if (exc) {
                    //op->fi->status_statinfo = UgrFileInfo::NotFound;
                    LocPluginLogInfo(SimpleDebug::kHIGHEST, fname, "Worker: stat not found.");
                } else {
                    LocPluginLogInfo(SimpleDebug::kHIGHEST, fname, "Worker: stat info:" << st.st_size << " " << st.st_mode);
                    op->fi->size = st.st_size;
                    op->fi->status_statinfo = UgrFileInfo::Ok;
                    op->fi->unixflags = st.st_mode;
                }
                break;

            case LocationPlugin::wop_Locate:
                if (exc) {
                    //op->fi->status_locations = UgrFileInfo::NotFound;
                    LocPluginLogInfo(SimpleDebug::kHIGHEST, fname, "Worker: locations not found.");
                } else {

                    for (vector<FileReplica>::iterator i = repvec.begin();
                            i != repvec.end();
                            i++) {
                        it.name = i->unparsed_location;
                        LocPluginLogInfo(SimpleDebug::kHIGHEST, fname, "Worker: Inserting repl" << i->unparsed_location);
                        it.location.clear();
                        op->fi->subitems.insert(it);
                    }
                    op->fi->status_locations = UgrFileInfo::Ok;
                }
                break;

            case LocationPlugin::wop_List:
                if (exc) {
                    LocPluginLogInfo(SimpleDebug::kHIGHEST, fname, "Worker: list not found.");
                    //op->fi->status_items = UgrFileInfo::NotFound;
                } else {

                    dirent *dent;
                    while ((dent = catalog->readDir(d))) {
                        it.name = dent->d_name;
                        it.location.clear();
                        op->fi->subitems.insert(it);
                    }
                    op->fi->status_items = UgrFileInfo::Ok;
                }

                break;

            default:
                break;
        }

        // We have modified the data, hence set the dirty flag
        op->fi->dirty = true;

        // Anyway the notification has to be correct, not redundant
        switch (op->wop) {

            case LocationPlugin::wop_Stat:
                op->fi->notifyStatNotPending();
                break;

            case LocationPlugin::wop_Locate:
                op->fi->notifyLocationNotPending();
                break;

            case LocationPlugin::wop_List:
                op->fi->notifyItemsNotPending();
                break;

            default:
                break;
        }



    }

    delete catalog;


}




// ------------------------------------------------------------------------------------
// Plugin-related stuff
// ------------------------------------------------------------------------------------


// The hook function. GetLocationPluginClass must be given the name of this function
// for the plugin to be loaded

extern "C" LocationPlugin * GetLocationPlugin(GetLocationPluginArgs) {
    return (LocationPlugin *)new UgrLocPlugin_dmlite(dbginstance, cfginstance, parms);
}

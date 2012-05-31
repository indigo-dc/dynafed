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
    catalog = 0;

    if (parms.size() > 3) {
        Info(SimpleDebug::kLOW, "UgrLocPlugin_dmlite", "Dmlite cfg: " << parms[3]);

        pluginManager = new dmlite::PluginManager();
        pluginManager->loadConfiguration(parms[3]);
        // Catalog
        catalog = pluginManager->getCatalogFactory()->createCatalog();
    }


};

void UgrLocPlugin_dmlite::runsearch(struct worktoken *op) {
    const char *fname = "UgrLocPlugin_dmlite::runsearch";
    struct stat st;
    std::vector<FileReplica> repvec;
    dmlite::Directory *d;
    bool exc = false;

    if (!catalog) return;
    if (!pluginManager) return;

    // We act using the identity of this service, hence we don't need to invoke
    // getIdMap/setUserblahblah
    
    try {
        switch (op->wop) {

            case LocationPlugin::wop_Stat:
                LocPluginLogInfo(SimpleDebug::kHIGH, fname, "invoking Stat(" << op->fi->name << ")");
                st = catalog->stat(op->fi->name);
                break;

            case LocationPlugin::wop_Locate:
                LocPluginLogInfo(SimpleDebug::kHIGH, fname, "invoking getReplicas(" << op->fi->name << ")");
                repvec = catalog->getReplicas(op->fi->name);
                break;

            case LocationPlugin::wop_List:
                LocPluginLogInfo(SimpleDebug::kHIGH, fname, "invoking openDir(" << op->fi->name << ")");
                d = catalog->openDir(op->fi->name);
                break;

            default:
                break;
        }
    } catch (dmlite::DmException e) {
        LocPluginLogInfo(SimpleDebug::kMEDIUM, fname, "Catched exception: " << e.code() << " what: " << e.what());
        exc = true;
    }
    LocPluginLogInfo(SimpleDebug::kMEDIUM, fname, "Worker: inserting data for " << op->fi->name);

    // Now put the results
    {
        UgrFileItem it;
        
        // Lock the file instance
        unique_lock<mutex> l(*(op->fi));

        op->fi->lastupdtime = time(0);

        switch (op->wop) {


            case LocationPlugin::wop_Stat:
                if (exc) {
                    op->fi->status_statinfo = UgrFileInfo::NotFound;
                   
                } else {
                    op->fi->size = st.st_size;
                    op->fi->status_statinfo = UgrFileInfo::Ok;
                    op->fi->unixflags = st.st_mode;
                }
                break;

            case LocationPlugin::wop_Locate:
                if (exc)
                    op->fi->status_locations = UgrFileInfo::NotFound;
                else {

                    for (vector<FileReplica>::iterator i = repvec.begin();
                            i != repvec.end();
                            i++) {
                        it.name = i->unparsed_location;
                        it.location.clear();
                        op->fi->subitems.insert(it);
                    }
                    op->fi->status_locations = UgrFileInfo::Ok;
                }
                break;

            case LocationPlugin::wop_List:
                if (exc)
                    op->fi->status_items = UgrFileInfo::NotFound;
                else {

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


}




// ------------------------------------------------------------------------------------
// Plugin-related stuff
// ------------------------------------------------------------------------------------


// The hook function. GetLocationPluginClass must be given the name of this function
// for the plugin to be loaded

extern "C" LocationPlugin * GetLocationPlugin(GetLocationPluginArgs) {
    return (LocationPlugin *)new UgrLocPlugin_dmlite(dbginstance, cfginstance, parms);
}

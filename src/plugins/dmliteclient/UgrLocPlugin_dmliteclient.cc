/** @file   UgrLocPlugin_dmlite.cc
 * @brief  Plugin that talks to dmlite
 * @author Fabrizio Furano
 * @date   Jan 2012
 */
#include "UgrLocPlugin_dmliteclient.hh"
#include "../../PluginLoader.hh"
#include <time.h>
#include <dmlite/cpp/catalog.h>


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
    dmlite::ExtendedStat st;
    std::vector<dmlite::Replica> repvec;
    dmlite::Directory *d = 0;
    bool exc = false;
    std::string xname;
    dmlite::Catalog *catalog = 0;
    dmlite::StackInstance *si = 0;
    dmlite::SecurityContext secCtx;

    bool listerror = false;

    if (!pluginManager) return;
    if (!catalogfactory) return;
      
    if (op->wop == wop_CheckReplica) {
        
        // Do the default name translation for this plugin (prefix xlation)
        if(doNameXlation(op->repl, xname)) {
            unique_lock<mutex> l(*(op->fi));
            op->fi->notifyLocationNotPending();
            return;
        }

    } else {
        // Do the default name translation for this plugin (prefix xlation)
        doNameXlation(op->fi->name, xname);
    } 
    
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
        // I suppose that secCtx must be filled with the agent's information
        si->setSecurityContext(secCtx);
        catalog = si->getCatalog();
    }
    if (!catalog) {
        LocPluginLogErr(fname, "Cannot find catalog.");
        exc = true;
    }

    // We act using the identity of this service, hence we don't need to invoke
    // getIdMap/setUserblahblah

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

                case LocationPlugin::wop_CheckReplica:
                    LocPluginLogInfoThr(SimpleDebug::kHIGH, fname, "invoking Stat(" << xname << ")");

                    // For now I don't see why it should not follow the links here
                    st = catalog->extendedStat(op->repl, true);
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






    op->fi->lastupdtime = time(0);

    switch (op->wop) {


        case LocationPlugin::wop_Stat:
            if (exc) {
                //op->fi->status_statinfo = UgrFileInfo::NotFound;
                LocPluginLogInfoThr(SimpleDebug::kHIGHEST, fname, "Worker: stat not found.");
            } else {
                op->fi->setPluginID(myID);
                op->fi->takeStat(st.stat);
            }


            break;

        case LocationPlugin::wop_Locate:
            if (exc) {
                //op->fi->status_locations = UgrFileInfo::NotFound;
                LocPluginLogInfoThr(SimpleDebug::kHIGHEST, fname, "Worker: locations not found.");
            } else {

                op->fi->setPluginID(myID);

                for (vector<dmlite::Replica>::iterator i = repvec.begin();
                        i != repvec.end();
                        i++) {

                    UgrFileItem_replica it;
                    it.name = i->rfn;
                    it.pluginID = myID;
                    LocPluginLogInfoThr(SimpleDebug::kHIGHEST, fname, "Worker: Inserting replicas" << i->rfn);

                    // Process it with the Geo plugin, if needed
                    if (geoPlugin) geoPlugin->setReplicaLocation(it);

                    // We have modified the data, hence set the dirty flag
                    op->fi->dirtyitems = true;

                    if (!isReplicaXlator()) {
                        // Lock the file instance
                        unique_lock<mutex> l(*(op->fi));

                        op->fi->replicas.insert(it);
                    }
                    else {
                        req_checkreplica(op->fi, i->rfn);
                    }
                    
                }
            }


            break;


        case LocationPlugin::wop_CheckReplica:
            if (!exc) {
                UgrFileItem_replica itr;
                doNameXlation(op->repl, itr.name);
                
                itr.pluginID = myID;
                LocPluginLogInfoThr(SimpleDebug::kHIGHEST, fname, "Worker: Inserting replicas " << op->repl);

                // We have modified the data, hence set the dirty flag
                op->fi->dirtyitems = true;

                // Process it with the Geo plugin, if needed
                if (geoPlugin) geoPlugin->setReplicaLocation(itr);
                {
                    // Lock the file instance
                    unique_lock<mutex> l(*(op->fi));

                    op->fi->replicas.insert(itr);
                }

                break;
            }
            
        case LocationPlugin::wop_List:
            if (exc) {
                LocPluginLogInfoThr(SimpleDebug::kHIGHEST, fname, "Worker: list not found.");
                //op->fi->status_items = UgrFileInfo::NotFound;
            } else {

                dmlite::ExtendedStat *dent;
                long cnt = 0;
                LocPluginLogInfoThr(SimpleDebug::kHIGHEST, fname, "Worker: Inserting list. ");
                op->fi->setPluginID(myID);

                try {
                    UgrFileItem it;
                    while ((dent = catalog->readDirx(d))) {

                        {
                            // Lock the file instance
                            unique_lock<mutex> l(*(op->fi));

                            if (cnt++ > CFG->GetLong("glb.maxlistitems", 2000)) {
                                LocPluginLogInfoThr(SimpleDebug::kMEDIUM, fname, "Setting as non listable. cnt=" << cnt);
                                listerror = true;
                                op->fi->subdirs.clear();
                                break;
                            }
                            it.name = dent->name;

                            op->fi->subdirs.insert(it);

                            // We have modified the data, hence set the dirty flag
                            op->fi->dirtyitems = true;
                        }

                        // We have some info to add to the cache
                        if (op->handler) {
                            string newlfn = op->fi->name + "/" + dent->name;
                            UgrFileInfo *fi = op->handler->getFileInfoOrCreateNewOne(newlfn, false);
                            LocPluginLogInfoThr(SimpleDebug::kHIGHEST, fname, "Worker: Inserting readdirx stat info for  " << dent->name << ", flags " << dent->stat.st_mode << " size : " << dent->stat.st_size);
                            if (fi) fi->takeStat(dent->stat);
                        }

                    }



                } catch (dmlite::DmException e) {
                    LocPluginLogErr(fname, "op: " << op->wop << "(processing) name: " << xname << " Catched exception: " << e.code() << " what: " << e.what());
                    exc = true;
                }

                try {

                    catalog->closeDir(d);


                } catch (dmlite::DmException e) {
                    LocPluginLogErr(fname, "op: " << op->wop << "(closing) name: " << xname << " Catched exception: " << e.code() << " what: " << e.what());
                    exc = true;
                }


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
            case LocationPlugin::wop_CheckReplica:
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

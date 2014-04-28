/** @file   UgrLocPlugin_dmlite.cc
 * @brief  Plugin that talks to dmlite
 * @author Fabrizio Furano
 * @date   Jan 2012
 */
#include "UgrLocPlugin_dmliteclient.hh"
#include "../../PluginLoader.hh"
#include "../../ExtCacheHandler.hh"
#include <time.h>
#include <dmlite/cpp/catalog.h>
#include "libs/time_utils.h"

using namespace boost;
using namespace std;

UgrLocPlugin_dmlite::UgrLocPlugin_dmlite(UgrConnector & c, std::vector<std::string> & parms) :
LocationPlugin(c, parms) {
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

    } else {
        Error("UgrLocPlugin_dav", "Not enough parameters in the plugin line.");
        throw std::runtime_error("No correct parameter for this Plugin : Unable to load the plugin properly ");
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

    if( doParentQueryCheck(op->fi->name, op, myidx)){
        return;
    }

    if (op->wop == wop_CheckReplica){

        // Do the default name translation for this plugin (prefix xlation)
        if (doNameXlation(op->repl, xname)) {
            unique_lock<mutex> l(*(op->fi));
            op->fi->notifyLocationNotPending();
            return;
        }

    } else {
        // Do the default name translation for this plugin (prefix xlation)
        if (doNameXlation(op->fi->name, xname)) {
            unique_lock<mutex> l(*(op->fi));
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
            return;
        }

    }

    // Catalog
    LocPluginLogInfoThr(SimpleDebug::kHIGH, fname, "Getting the catalogue instance");
    si = this->GetStackInstance(myidx);
    if (!si) exc = true;

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


    ReleaseStackInstance(si);
    si = 0;
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

                    // We have modified the data, hence set the dirty flag
                    op->fi->dirtyitems = true;

                    if (!isReplicaXlator()) {
                        op->fi->addReplica(it);
                    } else {
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
                        LocPluginLogInfoThr(SimpleDebug::kHIGHEST, fname, "readDirx -> " << dent->name);
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



}

dmlite::StackInstance *UgrLocPlugin_dmlite::GetStackInstance(int myidx, bool cancreate) {
    const char *fname = "UgrLocPlugin_dmliteclient::GetStackInstance";
    
    
    
    dmlite::StackInstance *si = 0;

    {

        boost::unique_lock< boost::mutex > l(dmlitemutex);
        if (siqueue.size() > 0) {
            si = siqueue.front();
            siqueue.pop();
        }

    }


    if (!si && cancreate) {

        try {
            LocPluginLogInfoThr(SimpleDebug::kLOW, fname, "Creating new StackInstance.");
            si = new dmlite::StackInstance(pluginManager);
        } catch (dmlite::DmException e) {
            LocPluginLogErr(fname, "Cannot create StackInstance. Catched exception: " << e.code() << " what: " << e.what());

        }

    }

    LocPluginLogInfo(SimpleDebug::kHIGHEST, fname, "Got stack instance " << si);
    return si;

}

void UgrLocPlugin_dmlite::ReleaseStackInstance(dmlite::StackInstance *inst) {
    LocPluginLogInfo(SimpleDebug::kHIGHEST, "fUgrLocPlugin_dmlite::ReleaseStackInstance", "Releasing stack instance " << inst);
    if (inst) {
        boost::unique_lock< boost::mutex > l(dmlitemutex);
        siqueue.push(inst);
    }
}

void UgrLocPlugin_dmlite::do_Check(int myidx) {
    const char *fname = "UgrLocPlugin_dmliteclient::do_Check";

    struct timespec t1, t2;

    bool test_ok = true;
    dmlite::StackInstance *si = 0;
    dmlite::Catalog *catalog = 0;
    dmlite::ExtendedStat st;
    PluginEndpointStatus status;
    dmlite::SecurityContext secCtx;


    LocPluginLogInfo(SimpleDebug::kHIGH, fname, "Start checker for " << xlatepfx_to << " with timeout " << availInfo.time_interval_ms);

    // Measure the time needed
    clock_gettime(CLOCK_MONOTONIC, &t1);

    // Do the check

    // Get a handle, if there are none then the check is fine
    LocPluginLogInfoThr(SimpleDebug::kHIGH, fname, "Getting the catalogue instance");

    si = this->GetStackInstance(myidx, false);
    if (!si) {
        LocPluginLogInfoThr(SimpleDebug::kHIGH, fname, "All the instances of StackInstance are busy. The check is passed.");
        return;
    }

    if (si) {
        // I suppose that secCtx must be filled with the agent's information
        si->setSecurityContext(secCtx);
        catalog = si->getCatalog();
    }

    if (!catalog) {
        LocPluginLogErr(fname, "Cannot find catalog.");
        return;
    }

    try {

        LocPluginLogInfoThr(SimpleDebug::kHIGH, fname, "invoking Stat(" << xlatepfx_to << ")");
        st = catalog->extendedStat(xlatepfx_to, false);

    } catch (dmlite::DmException e) {
        LocPluginLogErr(fname, "name: " << xlatepfx_to << " Catched exception: " << e.code() << " what: " << e.what());
        test_ok = false;

        // Prepare the text status message to display

        std::ostringstream ss;
        ss << "Check failed on " << xlatepfx_to << " " << e.what();
        status.explanation = ss.str();
        status.errcode = -1;

    }


    

    // Finish measuring the time needed
    clock_gettime(CLOCK_MONOTONIC, &t2);


    // Calculate the latency
    struct timespec diff_time;
    timespec_sub(&t2, &t1, &diff_time);
    status.latency_ms = (diff_time.tv_sec)*1000 + (diff_time.tv_nsec) / 1000000L;


    // For HTTP we CANNOT check that the prefix directory is known
    if (test_ok) {
        if (status.latency_ms > availInfo.max_latency_ms) {
            std::ostringstream ss;
            ss << "Latency of the endpoint " << status.latency_ms << "ms is superior to the limit " << availInfo.max_latency_ms << "ms";
            status.explanation = ss.str();

            status.state = PLUGIN_ENDPOINT_OFFLINE;

        } else {
            status.explanation = "";
            status.state = PLUGIN_ENDPOINT_ONLINE;
        }

    } else {
        if (status.explanation.empty()) {
            std::ostringstream ss;
            ss << "Server error reported : " << status.errcode;
            status.explanation = ss.str();
        }
        status.state = PLUGIN_ENDPOINT_OFFLINE;

    }

    status.lastcheck = time(0);
    availInfo.setStatus(status, true, (char *) name.c_str());


    // Propagate this fresh result to the extcache
    if (extCache)
        extCache->putEndpointStatus(&status, name);


    // If the test failed, destroy the catalog instance, it's useless. Other threads will try to create a new one.
    if (!test_ok) {
        delete si;
    } else {
        this->ReleaseStackInstance(si);

    }
    LocPluginLogInfo(SimpleDebug::kHIGHEST, fname, " End checker for " << xlatepfx_to);

}






// ------------------------------------------------------------------------------------
// Plugin-related stuff
// ------------------------------------------------------------------------------------


// The hook function. GetPluginInterfaceClass must be given the name of this function
// for the plugin to be loaded

extern "C" PluginInterface * GetPluginInterface(GetPluginInterfaceArgs) {
    return (PluginInterface *)new UgrLocPlugin_dmlite(c, parms);
}

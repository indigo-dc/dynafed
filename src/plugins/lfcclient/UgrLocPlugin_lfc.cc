/** 
 * @file   UgrLocPlugin_lfc.cc
 * @brief  Plugin that talks to any Webdav compatible endpoint
 * @author Devresse Adrien
 * @date   Feb 2012
 */


#include "UgrLocPlugin_lfc.hh"
#include "../../PluginLoader.hh"
#include "../../ExtCacheHandler.hh"


const std::string CONFIG_PREFIX("locplugin.");
const std::string config_timeout_conn_key("conn_timeout");
const std::string config_timeout_ops_key("ops_timeout");


using namespace boost;
using namespace std;

// ------------------------------------------------------------------------------------
// Plugin-related stuff
// ------------------------------------------------------------------------------------



// The hook function. GetLocationPluginClass must be given the name of this function
// for the plugin to be loaded

/**
 * Hook for the dav plugin Location plugin
 * */
extern "C" LocationPlugin *GetLocationPlugin(GetLocationPluginArgs) {
    return (LocationPlugin *)new UgrLocPlugin_lfc(dbginstance, cfginstance, parms);
}


UgrLocPlugin_lfc::UgrLocPlugin_lfc(SimpleDebug *dbginstance, Config *cfginstance, std::vector<std::string> &parms) :
LocationPlugin(dbginstance, cfginstance, parms) {
    GError* tmp_err=NULL;
    Info(SimpleDebug::kLOW, "UgrLocPlugin_lfc", "Creating instance named " << name);
    // try to get config
    const int params_size = parms.size();
    if (params_size > 3) {

        Info(SimpleDebug::kLOW, "UgrLocPlugin_lfc", "Try to bind UgrLocPlugin_lfc with " << parms[3]);
        base_url = parms[3];
        UgrFileInfo::trimpath(base_url);
    } else {
        throw std::runtime_error("No correct parameter for this plugin : Unable to load the lfc plugin");
    }
    load_configuration(CONFIG_PREFIX + name);

    if( (context = gfal2_context_new(&tmp_err)) == NULL){
        std::ostringstream ss;
        ss << "Impossible to load GFAL 2.0, " << name.c_str() << " plugin disabled : " <<  ((tmp_err)?(tmp_err->message):("Unknow Error")) << std::endl;
        Error("UgrLocPlugin_lfc::UgrLocPlugin_lfc", ss.str());
        g_clear_error(&tmp_err);
    }


}

void UgrLocPlugin_lfc::load_configuration(const std::string & prefix){

}

void UgrLocPlugin_lfc::insertReplicas(UgrFileItem_replica & itr, struct worktoken *op){
    // We have modified the data, hence set the dirty flag
    op->fi->dirtyitems = true;

    // Process it with the Geo plugin, if needed
    if (geoPlugin) geoPlugin->setReplicaLocation(itr);
    {
        // Lock the file instance
        unique_lock<mutex> l(*(op->fi));

        op->fi->replicas.insert(itr);
    }
}

int UgrLocPlugin_lfc::getReplicasFromLFC(const std::string & url, const int myidx,
                                         const boost::function<void (UgrFileItem_replica & it)> & inserter, GError** err){
    static const int b_size = 10000;
    char buffer[b_size];
    ssize_t ret;
    char* p = buffer;
    if( ( ret = gfal2_getxattr(context, url.c_str(), GFAL_XATTR_REPLICA, buffer, b_size, err)) < 0)
        return -1;

    while( p < buffer + ret){
        UgrFileItem_replica itr;
        itr.name = p;
        itr.pluginID = myID;
        LocPluginLogInfoThr(SimpleDebug::kHIGHEST, "UgrLocPlugin_lfc::getReplicasFromLFC", "Worker: Inserting replicas " << p);
        p += strlen(p) +1; // select next replicas
        inserter(itr);
    }
    return 0;
}

void UgrLocPlugin_lfc::runsearch(struct worktoken *op, int myidx) {
    GError * tmp_err=NULL;
    struct stat st;
    static const char * fname = "UgrLocPlugin_lfc::runsearch";
    std::string canonical_name = base_url;
    std::string xname;
    bool bad_answer = true;
    DIR* d = NULL;
    bool listerror = false;


    if(context == NULL){
        Error(fname, "Impossible to request " << name << " GFAL 2.0 disabled");
        return;
    }

    // We act using the identity of this service, hence we don't need to invoke
    // getIdMap/setUserblahblah
    if (op == NULL || op->fi == NULL) {
        Error(fname, " Bad request Handle : search aborted");
        return;
    }


    // Do the default name translation for this plugin (prefix xlation)
    doNameXlation(op->fi->name, xname);
    // Then prepend the URL prefix
    canonical_name += xname;

    memset(&st, 0, sizeof (st));

    switch (op->wop) {

        case LocationPlugin::wop_Stat:
            LocPluginLogInfoThr(SimpleDebug::kHIGH, fname, "invoking gfal2_stat(" << canonical_name << ")");
            gfal2_stat(context, canonical_name.c_str(), &st, &tmp_err);
            break;

        case LocationPlugin::wop_Locate:
            LocPluginLogInfoThr(SimpleDebug::kHIGH, fname, "invoking Locate(" << canonical_name << ")");
            getReplicasFromLFC(canonical_name.c_str(), myidx,
                               boost::function< void(UgrFileItem_replica &) >(boost::bind(&UgrLocPlugin_lfc::insertReplicas, ref(*this), _1, op)),&tmp_err);
            break;

        case LocationPlugin::wop_List:
            LocPluginLogInfoThr(SimpleDebug::kHIGH, fname, " invoking davix_openDir(" << canonical_name << ")");
            d= gfal2_opendir(context, canonical_name.c_str(), &tmp_err);
            op->fi->unixflags |= S_IFDIR;
            break;

        default:
            break;
    }

    if (!tmp_err) {
        bad_answer = false; // reach here -> request complete
    } else {

        // Connection problem...
        if(tmp_err->code == ECOMM ) {
            PluginEndpointStatus st;
            availInfo.getStatus(st);
            st.lastcheck = time(0);
            st.state = PLUGIN_ENDPOINT_OFFLINE;
            availInfo.setStatus(st, true, (char *) name.c_str());
            // Propagate this fresh result to the extcache
            if (extCache)
                extCache->putEndpointStatus(&st, name);
        }


        LocPluginLogInfoThr(SimpleDebug::kHIGH, fname, " UgrDav plugin request Error : " << ((int) tmp_err->code) << " errMsg: " << tmp_err->message);
    }


    op->fi->lastupdtime = time(0);

    if (bad_answer == false) {
        LocPluginLogInfoThr(SimpleDebug::kMEDIUM, fname, "Worker: inserting data for " << op->fi->name);
        op->fi->setPluginID(myID);

        switch (op->wop) {

            case LocationPlugin::wop_Stat:
                LocPluginLogInfoThr(SimpleDebug::kHIGHEST, fname, "Worker: stat info:" << st.st_size << " " << st.st_mode);
                op->fi->takeStat(st);
                break;

            case LocationPlugin::wop_List:
            {
                dirent * dent;
                long cnt = 0;
                struct stat st2;
                while ((dent = gfal2_readdir(context, d, &tmp_err)) != NULL
                       && gfal2_stat(context, canonical_name.c_str(), &st2, &tmp_err) == 0) {
                    UgrFileItem it;
                    {
                        unique_lock<mutex> l(*(op->fi));

                        // We have modified the data, hence set the dirty flag
                        op->fi->dirtyitems = true;

                        if (cnt++ > CFG->GetLong("glb.maxlistitems", 2000)) {
                            LocPluginLogInfoThr(SimpleDebug::kMEDIUM, fname, "Setting as non listable. cnt=" << cnt);
                            listerror = true;
                            op->fi->subdirs.clear();
                            break;
                        }

                        // create new items
                        LocPluginLogInfoThr(SimpleDebug::kHIGHEST, fname, "Worker: Inserting list " << dent->d_name);
                        it.name = std::string(dent->d_name);
                        it.location.clear();

                        // populate answer
                        op->fi->subdirs.insert(it);
                    }

                    // add childrens
                    string child = op->fi->name;
                    if (child[child.length() - 1] != '/')
                        child = child + "/";
                    child = child + it.name;

                    LocPluginLogInfoThr(SimpleDebug::kHIGHEST, fname,
                            "Worker: Inserting readdirpp stat info for  " << child <<
                            ", flags " << st.st_mode << " size : " << st.st_size);
                    UgrFileInfo *fi = op->handler->getFileInfoOrCreateNewOne(child, false);

                    // If the entry was already in cache, don't overwrite
                    // This avoids a massive, potentially useless burst of writes to the 2nd level cache 
                    if (fi && (fi->status_statinfo != UgrFileInfo::Ok)) {
                        fi->takeStat(st2);
                    }
                }
                gfal2_closedir(context, d, NULL);
            }
                break;

            default:
                break;
        }


        if (tmp_err) {
            LocPluginLogInfoThr(SimpleDebug::kHIGH, fname, " UgrDav plugin request Error : " << ((int) tmp_err->code) << " errMsg: " << tmp_err->message);
        }



    }



    // Anyway the notification has to be correct, not redundant
    {
        // Lock the file instance
        unique_lock<mutex> l(*(op->fi));
        switch (op->wop) {

            case LocationPlugin::wop_Stat:
                LocPluginLogInfoThr(SimpleDebug::kHIGHEST, fname, "Notify End Stat");
                op->fi->notifyStatNotPending();
                break;

            case LocationPlugin::wop_Locate:
                LocPluginLogInfoThr(SimpleDebug::kHIGHEST, fname, "Notify End Locate");
                op->fi->status_locations = UgrFileInfo::Ok;
                op->fi->notifyLocationNotPending();
                break;

            case LocationPlugin::wop_List:
                if (listerror) {
                    op->fi->status_items = UgrFileInfo::Error;

                } else
                    op->fi->status_items = UgrFileInfo::Ok;

                LocPluginLogInfoThr(SimpleDebug::kHIGHEST, fname, "Notify End Listdir");
                op->fi->notifyItemsNotPending();
                break;

            default:
                break;
        }

    }

}






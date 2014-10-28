/**
 * @file   UgrLocPlugin_lfc.cc
 * @brief  Plugin that talks to any Webdav compatible endpoint
 * @author Devresse Adrien
 * @date   Feb 2012
 */


#include "UgrLocPlugin_lfc.hh"
#include "../../PluginLoader.hh"
#include "../../ExtCacheHandler.hh"

const std::string config_timeout_conn_key("conn_timeout");
const std::string config_timeout_ops_key("ops_timeout");


using namespace boost;

// ------------------------------------------------------------------------------------
// Plugin-related stuff
// ------------------------------------------------------------------------------------



// The hook function. GetPluginInterfaceClass must be given the name of this function
// for the plugin to be loaded

/**
 * Hook for the dav plugin Location plugin
 * */
extern "C" PluginInterface *GetPluginInterface(GetPluginInterfaceArgs) {
    return (PluginInterface *)new UgrLocPlugin_lfc(c, parms);
}


UgrLocPlugin_lfc::UgrLocPlugin_lfc(UgrConnector & c, std::vector<std::string> & parms) :
LocationPlugin(c, parms) {
    GError* tmp_err=NULL;
    Info(UgrLogger::Lvl1, "UgrLocPlugin_lfc", "Creating instance named " << name);
    // try to get config
    const int params_size = parms.size();
    if (params_size > 3) {

        Info(UgrLogger::Lvl1, "UgrLocPlugin_lfc", "Try to bind UgrLocPlugin_lfc with " << parms[3]);
        base_url = parms[3];
        UgrFileInfo::trimpath(base_url);
    } else {
        throw std::runtime_error("No correct parameter for this plugin : Unable to load the lfc plugin");
    }
    load_configuration(getConfigPrefix() + name);

    if( (context = gfal2_context_new(&tmp_err)) == NULL){
        std::ostringstream ss;
        ss << "Impossible to load GFAL 2.0, " << name.c_str() << " plugin disabled : " <<  ((tmp_err)?(tmp_err->message):("Unknow Error")) << std::endl;
        Error("UgrLocPlugin_lfc::UgrLocPlugin_lfc", ss.str());
        g_clear_error(&tmp_err);
    }


}

void UgrLocPlugin_lfc::load_configuration(const std::string & prefix){
    Config * c = Config::GetInstance();
    std::string pref_dot = prefix + std::string(".");

    const std::string proxy_cred = c->GetString(pref_dot + std::string("cli_proxy_cert"), "");
    if(proxy_cred.empty() == false){
        Info(UgrLogger::Lvl1, "UgrLocPlugin_lfc", " Client proxy credential:  " + proxy_cred);
        g_setenv("X509_USER_PROXY",proxy_cred.c_str(), TRUE);
    }

    const std::string credential_path = c->GetString(pref_dot + std::string("cli_certificate"), "");
    if(credential_path.empty() == false){
        Info(UgrLogger::Lvl1, "UgrLocPlugin_lfc", " Client certificate:  " + credential_path);
        g_setenv("X509_USER_CERT",credential_path.c_str(), TRUE);
    }

    const std::string privatekey_path = c->GetString(pref_dot + std::string("cli_privatekey"), "");
    if(privatekey_path.empty() == false){
        Info(UgrLogger::Lvl1, "UgrLocPlugin_lfc", " Client private key:  " + privatekey_path);
        g_setenv("X509_USER_KEY",privatekey_path.c_str(), TRUE);
    }

    const std::string csec_mech = c->GetString(pref_dot + std::string("csec_mech"), "");
    if(csec_mech.empty() == false){
        Info(UgrLogger::Lvl1, "UgrLocPlugin_lfc", " Csec mechanism:  " + csec_mech);
        g_setenv("CSEC_MECH",csec_mech.c_str(), TRUE);
    }else{
         Info(UgrLogger::Lvl1, "UgrLocPlugin_lfc", " default Csec Mechanism");
    }

    const bool debug = c->GetBool(pref_dot + std::string("debug"), false);
    if(debug){
        gfal_set_verbose(GFAL_VERBOSE_TRACE | GFAL_VERBOSE_DEBUG | GFAL_VERBOSE_VERBOSE | GFAL_VERBOSE_TRACE_PLUGIN);
    }

}

void UgrLocPlugin_lfc::insertReplicas(UgrFileItem_replica & itr, struct worktoken *op){
    // We have modified the data, hence set the dirty flag
    op->fi->dirtyitems = true;

    if (!isReplicaXlator()) {
        op->fi->addReplica(itr);
    } else {
        req_checkreplica(op->fi, itr.name);
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
        itr.pluginID = getID();
        LocPluginLogInfoThr(UgrLogger::Lvl4, "UgrLocPlugin_lfc::getReplicasFromLFC", "Worker: Inserting replicas " << p);
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


    if( doParentQueryCheck(op->fi->name, op, myidx)){
        return;
    }
    // Do the default name translation for this plugin (prefix xlation)
    if (op->wop == wop_CheckReplica) {

        // Do the default name translation for this plugin (prefix xlation)
        if (doNameXlation(op->repl, xname, op->wop, op->altpfx)) {
            unique_lock<mutex> l(*(op->fi));
            op->fi->notifyLocationNotPending();
            return;
        }// Then prepend the URL prefix
        else canonical_name.append(xname);
    } else {

        // Do the default name translation for this plugin (prefix xlation)
        if (doNameXlation(op->fi->name, xname, op->wop, op->altpfx)) {
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

        // Then prepend the URL prefix
        canonical_name.append(xname);
    }


    memset(&st, 0, sizeof (st));

    switch (op->wop) {

        case LocationPlugin::wop_Stat:
            LocPluginLogInfoThr(UgrLogger::Lvl3, fname, "invoking LFC Stat(" << canonical_name << ")");
            gfal2_stat(context, canonical_name.c_str(), &st, &tmp_err);
            break;

        case LocationPlugin::wop_Locate:
            LocPluginLogInfoThr(UgrLogger::Lvl3, fname, "invoking LFC Locate(" << canonical_name << ")");
            getReplicasFromLFC(canonical_name.c_str(), myidx,
                               boost::function< void(UgrFileItem_replica &) >(boost::bind(&UgrLocPlugin_lfc::insertReplicas, ref(*this), _1, op)),&tmp_err);
            break;

        case LocationPlugin::wop_List:
            LocPluginLogInfoThr(UgrLogger::Lvl3, fname, " invoking LFC openDir(" << canonical_name << ")");
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


        LocPluginLogInfoThr(UgrLogger::Lvl3, fname, "LFC plugin request Error : " << ((int) tmp_err->code) << " errMsg: " << tmp_err->message);
    }


    op->fi->lastupdtime = time(0);

    if (bad_answer == false) {
        LocPluginLogInfoThr(UgrLogger::Lvl2, fname, "Worker: inserting data for " << op->fi->name);
        op->fi->setPluginID(getID());

        switch (op->wop) {

            case LocationPlugin::wop_Stat:
                LocPluginLogInfoThr(UgrLogger::Lvl4, fname, "Worker: stat info:" << st.st_size << " " << st.st_mode);
                op->fi->takeStat(st);
                break;

            case LocationPlugin::wop_List:
            {
                dirent * dent;
                long cnt = 0;
                struct stat st2;
                while ((dent = gfal2_readdirpp(context, d, &st2, &tmp_err)) != NULL ) {
                    UgrFileItem it;
                    {
                        unique_lock<mutex> l(*(op->fi));

                        // We have modified the data, hence set the dirty flag
                        op->fi->dirtyitems = true;

                        if (cnt++ > CFG->GetLong("glb.maxlistitems", 20000)) {
                            LocPluginLogInfoThr(UgrLogger::Lvl2, fname, "Setting as non listable. cnt=" << cnt);
                            listerror = true;
                            op->fi->subdirs.clear();
                            break;
                        }

                        // create new items
                        LocPluginLogInfoThr(UgrLogger::Lvl4, fname, "Worker: Inserting list " << dent->d_name);
                        it.name = std::string(dent->d_name);
                        it.location.clear();

                        // populate answer
                        op->fi->subdirs.insert(it);
                    }

                    // add childrens
                    std::string child = op->fi->name;
                    if (child[child.length() - 1] != '/')
                        child = child + "/";
                    child = child + it.name;

                    LocPluginLogInfoThr(UgrLogger::Lvl4, fname,
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
            LocPluginLogInfoThr(UgrLogger::Lvl3, fname, " UgrDav plugin request Error : " << ((int) tmp_err->code) << " errMsg: " << tmp_err->message);
        }



    }



    // Anyway the notification has to be correct, not redundant
    {
        // Lock the file instance
        unique_lock<mutex> l(*(op->fi));
        switch (op->wop) {

            case LocationPlugin::wop_Stat:
                LocPluginLogInfoThr(UgrLogger::Lvl4, fname, "Notify End Stat");
                op->fi->notifyStatNotPending();
                break;

            case LocationPlugin::wop_Locate:
                LocPluginLogInfoThr(UgrLogger::Lvl4, fname, "Notify End Locate");
                op->fi->status_locations = UgrFileInfo::Ok;
                op->fi->notifyLocationNotPending();
                break;

            case LocationPlugin::wop_List:
                if (listerror) {
                    op->fi->status_items = UgrFileInfo::Error;

                } else
                    op->fi->status_items = UgrFileInfo::Ok;

                LocPluginLogInfoThr(UgrLogger::Lvl4, fname, "Notify End Listdir");
                op->fi->notifyItemsNotPending();
                break;

            default:
                break;
        }

    }

}







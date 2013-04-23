/** 
 * @file   UgrLocPlugin_dav.cc
 * @brief  Plugin that talks to any Webdav compatible endpoint
 * @author Devresse Adrien
 * @date   Feb 2012
 */


#include "UgrLocPlugin_dav.hh"
#include "../../PluginLoader.hh"
#include "../../ExtCacheHandler.hh"
#include "../utils/PluginUtils.hh"
#include <time.h>
#include "libs/time_utils.h"

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
    davix_set_log_level(DAVIX_LOG_WARNING);
    return (LocationPlugin *)new UgrLocPlugin_dav(dbginstance, cfginstance, parms);
}

UgrLocPlugin_dav::UgrLocPlugin_dav(SimpleDebug *dbginstance, Config *cfginstance, std::vector<std::string> &parms) :
LocationPlugin(dbginstance, cfginstance, parms), dav_core(new Davix::Context()), pos(dav_core.get()) {
    Info(SimpleDebug::kLOW, "UgrLocPlugin_dav", "Creating instance named " << name);
    // try to get config
    const int params_size = parms.size();
    if (params_size > 3) {
        Info(SimpleDebug::kLOW, "UgrLocPlugin_dav", "Try to bind UgrLocPlugin_dav with " << parms[3]);
        base_url = parms[3];
        UgrFileInfo::trimpath(base_url);

    } else {
        Error("UgrLocPlugin_dav", "Not enough parameters in the plugin line.");
        throw std::runtime_error("No correct parameter for this Plugin : Unable to load the plugin properly ");
    }
    load_configuration(CONFIG_PREFIX + name);

    params.setSSLCAcheck(ssl_check);
    checker_params.setSSLCAcheck(ssl_check);

}

void UgrLocPlugin_dav::load_configuration(const std::string & prefix) {
    Davix::DavixError * tmp_err = NULL;
    Davix::X509Credential cred;

    // get ssl check
    ssl_check = pluginGetParam<bool>(prefix, "ssl_check", true);
    Info(SimpleDebug::kLOW, "UgrLocPlugin_dav", "SSL CA check for davix is set to  " + std::string((ssl_check) ? "TRUE" : "FALSE"));
    // ca check
    const std::string ca_path = pluginGetParam<std::string>(prefix, "ca_path");
    if( ca_path.size() > 0){
        Info(SimpleDebug::kLOW, "UgrLocPlugin_dav", "CA Path added :  " << ca_path);
        params.addCertificateAuthorityPath(ca_path);
    }
    // get credential
    const std::string pkcs12_credential_path = pluginGetParam<std::string>(prefix,"cli_certificate");
    // get credential password
    const std::string pkcs12_credential_password = pluginGetParam<std::string>(prefix, "cli_password");
    if (pkcs12_credential_path.size() > 0) {
        Info(SimpleDebug::kLOW, "UgrLocPlugin_dav", " CLI CERT path is set to  " + pkcs12_credential_path);
        if (pkcs12_credential_password.size() > 0)
            Info(SimpleDebug::kLOW, "UgrLocPlugin_dav", " CLI CERT passwrd defined  ");
        if (cred.loadFromFileP12(pkcs12_credential_path, pkcs12_credential_password, &tmp_err) < 0) {
            Info(SimpleDebug::kHIGH, "UgrLocPlugin_dav", "Error: impossible to load credential "
                    + pkcs12_credential_path + " :" + tmp_err->getErrMsg());
            Davix::DavixError::clearError(&tmp_err);
        } else {
            params.setClientCertX509(cred);
        }
    }
    // auth login
    const std::string login = pluginGetParam<std::string>(prefix,"auth_login");
    // auth password
    const std::string password = pluginGetParam<std::string>(prefix,"auth_passwd");
    if (password.size() > 0 && login.size() > 0) {
        Info(SimpleDebug::kLOW, "UgrLocPlugin_dav", "login and password setup for authentication");
        params.setClientLoginPassword(login, password);
    }



    // timeout management
    long timeout;
    struct timespec spec_timeout;
    if ((timeout =pluginGetParam<long>(prefix, config_timeout_conn_key, 120)) != 0) {
        Info(SimpleDebug::kLOW, "UgrLocPlugin_dav", " Connection timeout is set to : " << timeout);
        spec_timeout.tv_sec = timeout;
        params.setConnectionTimeout(&spec_timeout);
    }
    if ((timeout = pluginGetParam<long>(prefix, config_timeout_ops_key, 120)) != 0) {
        spec_timeout.tv_sec = timeout;
        params.setOperationTimeout(&spec_timeout);
        Info(SimpleDebug::kLOW, "UgrLocPlugin_dav", " Operation timeout is set to : " << timeout);
    }

    checker_params = params; // clone the parameters for the checker
    spec_timeout.tv_sec = this->availInfo.time_interval_ms / 1000;
    spec_timeout.tv_nsec = (this->availInfo.time_interval_ms - spec_timeout.tv_sec) * 1000000;
    checker_params.setOperationTimeout(&spec_timeout);
    checker_params.setConnectionTimeout(&spec_timeout);
}

void UgrLocPlugin_dav::runsearch(struct worktoken *op, int myidx) {
    struct stat st;
    Davix::DavixError * tmp_err = NULL;
    static const char * fname = "UgrLocPlugin_dav::runsearch";
    std::string canonical_name;
    std::string xname;
    bool bad_answer = true;
    DAVIX_DIR* d = NULL;
    bool listerror = false;

    // We act using the identity of this service, hence we don't need to invoke
    // getIdMap/setUserblahblah
    if (op == NULL || op->fi == NULL) {
        Error(fname, " Bad request Handle : search aborted");
        return;
    }


    if (op->wop == wop_CheckReplica) {

        // Do the default name translation for this plugin (prefix xlation)
        if (doNameXlation(op->repl, xname)) {
            unique_lock<mutex> l(*(op->fi));
            op->fi->notifyLocationNotPending();
            return;
        }// Then prepend the URL prefix
        else canonical_name = base_url + xname;
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

        // Then prepend the URL prefix
        canonical_name = base_url + xname;
    }

    memset(&st, 0, sizeof (st));



    switch (op->wop) {

        case LocationPlugin::wop_Stat:
            LocPluginLogInfoThr(SimpleDebug::kHIGH, fname, "invoking davix_Stat(" << canonical_name << ")");
            pos.stat(&params, canonical_name, &st, &tmp_err);
            break;

        case LocationPlugin::wop_Locate:
            LocPluginLogInfoThr(SimpleDebug::kHIGH, fname, "invoking Locate(" << canonical_name << ")");
            pos.stat(&params, canonical_name, &st, &tmp_err);
            break;
        case LocationPlugin::wop_CheckReplica:
            LocPluginLogInfoThr(SimpleDebug::kHIGH, fname, "invoking CheckReplica(" << canonical_name << ")");
            pos.stat(&params, canonical_name, &st, &tmp_err);
            break;
        case LocationPlugin::wop_List:
            LocPluginLogInfoThr(SimpleDebug::kHIGH, fname, " invoking davix_openDir(" << canonical_name << ")");
            d = pos.opendirpp(&params, canonical_name, &tmp_err);
            // if reach here -> valid opendir -> specify file as well
            op->fi->unixflags |= S_IFDIR;
            break;

        default:
            break;
    }

    if (!tmp_err) {
        bad_answer = false; // reach here -> request complete

        // Everything OK... keep the online status

        PluginEndpointStatus st;
        availInfo.getStatus(st);
        st.lastcheck = time(0);
        if (st.state == PLUGIN_ENDPOINT_ONLINE)
            availInfo.setStatus(st, true, (char *) name.c_str());



    } else {

        // Connection problem... it's offline for sure!
        if ((tmp_err->getStatus() == Davix::StatusCode::ConnectionTimeout) ||
                (tmp_err->getStatus() == Davix::StatusCode::ConnectionTimeout)) {
            PluginEndpointStatus st;
            availInfo.getStatus(st);
            st.lastcheck = time(0);
            st.state = PLUGIN_ENDPOINT_OFFLINE;
            availInfo.setStatus(st, true, (char *) name.c_str());
            // Propagate this fresh result to the extcache
            if (extCache)
                extCache->putEndpointStatus(&st, name);
        }


        LocPluginLogInfoThr(SimpleDebug::kHIGH, fname, " UgrDav plugin request Error : " << ((int) tmp_err->getStatus()) << " errMsg: " << tmp_err->getErrMsg());
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

            case LocationPlugin::wop_Locate:
            {
                UgrFileItem_replica itr;
                itr.name = canonical_name;
                itr.pluginID = myID;
                LocPluginLogInfoThr(SimpleDebug::kHIGHEST, fname, "Worker: Inserting replicas " << canonical_name);

                // We have modified the data, hence set the dirty flag
                op->fi->dirtyitems = true;

                // Process it with the Geo plugin, if needed
                if (geoPlugin) geoPlugin->setReplicaLocation(itr);

                if (!isReplicaXlator()) {
                    op->fi->addReplica(itr);
                } else {
                    req_checkreplica(op->fi, itr.name);
                }

                break;
            }
            case LocationPlugin::wop_CheckReplica:
            {
                UgrFileItem_replica itr;
                itr.name = canonical_name;


                itr.pluginID = myID;
                LocPluginLogInfoThr(SimpleDebug::kHIGHEST, fname, "Worker: Inserting replicas " << itr.name);

                // We have modified the data, hence set the dirty flag
                op->fi->dirtyitems = true;

                // Process it with the Geo plugin, if needed
                if (geoPlugin) geoPlugin->setReplicaLocation(itr);
                op->fi->addReplica(itr);

                break;
            }
            case LocationPlugin::wop_List:
            {
                dirent * dent;
                long cnt = 0;
                struct stat st2;
                while ((dent = pos.readdirpp(d, &st2, &tmp_err)) != NULL) {
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
                pos.closedirpp(d, NULL);


            }
                break;

            default:
                break;
        }


        if (tmp_err) {
            LocPluginLogInfoThr(SimpleDebug::kHIGH, fname, " UgrDav plugin request Error : " << ((int) tmp_err->getStatus()) << " errMsg: " << tmp_err->getErrMsg());
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
            case LocationPlugin::wop_CheckReplica:
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

void UgrLocPlugin_dav::do_Check(int myidx) {
    const char *fname = "UgrLocPlugin_dav::do_Check";

    struct timespec t1, t2;
    Davix::DavixError* tmp_err = NULL;

    PluginEndpointStatus st;
    st.errcode = 404;

    LocPluginLogInfo(SimpleDebug::kHIGH, fname, "Start checker for " << base_url << " with time " << availInfo.time_interval_ms);

    boost::shared_ptr<Davix::HttpRequest> req;

    // Measure the time needed
    clock_gettime(CLOCK_MONOTONIC, &t1);

    req = boost::shared_ptr<Davix::HttpRequest > (static_cast<Davix::HttpRequest*> (dav_core->createRequest(base_url, &tmp_err)));

    // Set decent timeout values for the operation
    req->setParameters(checker_params);

    if (req.get() != NULL) {
        req->setRequestMethod("HEAD");
        if (req->executeRequest(&tmp_err) == 0)
            st.errcode = req->getRequestCode();
    }

    // Prepare the text status message to display
    if (tmp_err) {
        std::ostringstream ss;
        ss << "HTTP status error on " << base_url << " " << tmp_err->getErrMsg();
        st.explanation = ss.str();
        st.errcode = -1;
    }

    // Finish measuring the time needed
    clock_gettime(CLOCK_MONOTONIC, &t2);


    // Calculate the latency
    struct timespec diff_time;
    timespec_sub(&t2, &t1, &diff_time);
    st.latency_ms = (diff_time.tv_sec)*1000 + (diff_time.tv_nsec) / 1000000L;


    // For DAV we can also check that the prefix directory is known
    if (st.errcode >= 200 && st.errcode < 400) {
        if (st.latency_ms > availInfo.max_latency_ms) {
            std::ostringstream ss;
            ss << "Latency of the endpoint " << st.latency_ms << "ms is superior to the limit " << availInfo.max_latency_ms << "ms";
            st.explanation = ss.str();

            st.state = PLUGIN_ENDPOINT_OFFLINE;

        } else {
            st.explanation = "";
            st.state = PLUGIN_ENDPOINT_ONLINE;
        }

    } else {
        if (st.explanation.empty()) {
            std::ostringstream ss;
            ss << "Server error reported : " << st.errcode;
            st.explanation = ss.str();
        }
        st.state = PLUGIN_ENDPOINT_OFFLINE;

    }

    st.lastcheck = time(0);
    availInfo.setStatus(st, true, (char *) name.c_str());


    // Propagate this fresh result to the extcache
    if (extCache)
        extCache->putEndpointStatus(&st, name);


    LocPluginLogInfo(SimpleDebug::kHIGHEST, fname, " End checker for " << base_url);

}







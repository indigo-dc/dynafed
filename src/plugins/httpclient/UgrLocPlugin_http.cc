/**
 * @file   UgrLocPlugin_http.cc
 * @brief  Plugin that talks to any Webdav compatible endpoint
 * @author Devresse Adrien
 * @date   Feb 2012
 */


#include "UgrLocPlugin_http.hh"
#include "../../PluginLoader.hh"
#include "../../ExtCacheHandler.hh"
#include <time.h>
#include "libs/time_utils.h"
#include "../utils/HttpPluginUtils.hh"

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
    return (LocationPlugin *)new UgrLocPlugin_http(dbginstance, cfginstance, parms);
}

UgrLocPlugin_http::UgrLocPlugin_http(SimpleDebug *dbginstance, Config *cfginstance, std::vector<std::string> &parms) :
LocationPlugin(dbginstance, cfginstance, parms), dav_core(new Davix::Context()), pos(dav_core.get()) {
    Info(SimpleDebug::kLOW, "UgrLocPlugin_http", "Creating instance named " << name);
    // try to get config
    const int params_size = parms.size();
    if (params_size > 3) {
        Info(SimpleDebug::kLOW, "UgrLocPlugin_http", "Try to bind UgrLocPlugin_http with " << parms[3]);
        base_url = parms[3];
        UgrFileInfo::trimpath(base_url);

    } else {
        Error("UgrLocPlugin_http", "Not enough parameters in the plugin line.");
        throw std::runtime_error("No correct parameter for this Plugin : Unable to load the plugin properly ");
    }
    load_configuration(CONFIG_PREFIX + name);
}

void UgrLocPlugin_http::load_configuration(const std::string & prefix) {
    params.setProtocol(Davix::RequestProtocol::Http);

    HttpUtils::configureSSLParams(name, prefix, params);
    HttpUtils::configureHttpAuth(name, prefix, params);
    HttpUtils::configureHttpTimeout(name, prefix, params);

    checker_params = params;
    struct timespec spec_timeout;
    spec_timeout.tv_sec = this->availInfo.time_interval_ms / 1000;
    spec_timeout.tv_nsec = (this->availInfo.time_interval_ms - spec_timeout.tv_sec) * 1000000;
    checker_params.setOperationTimeout(&spec_timeout);
    checker_params.setConnectionTimeout(&spec_timeout);
}

void UgrLocPlugin_http::runsearch(struct worktoken *op, int myidx) {
    struct stat st;
    Davix::DavixError * tmp_err = NULL;
    static const char * fname = "UgrLocPlugin_http::runsearch";
    std::string canonical_name = base_url;
    std::string xname;
    bool bad_answer = true;


    // We act using the identity of this service, hence we don't need to invoke
    // getIdMap/setUserblahblah
    if (op == NULL || op->fi == NULL) {
        Error(fname, " Bad request Handle : search aborted");
        return;
    }


    if( doParentQueryCheck(op->fi->name, op, myidx)){
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
            // force path finishing with '/' like a directory, impossible to get the type of a file in plain http
            if (canonical_name.at(canonical_name.length() - 1) == '/') {
                st.st_mode |= S_IFDIR;
            }
            break;

        case LocationPlugin::wop_Locate:
            LocPluginLogInfoThr(SimpleDebug::kHIGH, fname, "invoking Locate(" << canonical_name << ")");
            pos.stat(&params, canonical_name, &st, &tmp_err);
            break;

        case LocationPlugin::wop_CheckReplica:
            LocPluginLogInfoThr(SimpleDebug::kHIGH, fname, "invoking CheckReplica(" << canonical_name << ")");
            pos.stat(&params, canonical_name, &st, &tmp_err);
            break;

        default:
            break;
    }

    if (!tmp_err) {
        bad_answer = false; // reach here -> request complete
    } else {
        LocPluginLogInfoThr(SimpleDebug::kHIGH, fname, " UgrHttp plugin request Error : " << ((int) tmp_err->getStatus()) << " errMsg: " << tmp_err->getErrMsg());
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

                op->fi->addReplica(itr);

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

            default:
                break;
        }


        if (tmp_err) {
            LocPluginLogInfoThr(SimpleDebug::kHIGH, fname, " UgrHttp plugin request Error : " << ((int) tmp_err->getStatus()) << " errMsg: " << tmp_err->getErrMsg());
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

            default:
                break;
        }

    }

}

void UgrLocPlugin_http::do_Check(int myidx) {
    const char *fname = "UgrLocPlugin_http::do_Check";

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


    // For HTTP we CANNOT check that the prefix directory is known
    if (st.errcode >= 200) {
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



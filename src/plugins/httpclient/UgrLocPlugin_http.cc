/**
 * @file   UgrLocPlugin_http.cc
 * @brief  Plugin that talks to any Webdav compatible endpoint
 * @author Devresse Adrien
 * @date   Feb 2012
 */


#include "UgrLocPlugin_http.hh"
#include "../../PluginLoader.hh"
#include "../../ExtCacheHandler.hh"
#include "../../Config.hh"
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



enum CredType{
    ProxyCred,
    PemCred,
    Pkcs12Cred
};

static CredType parseCredType(const std::string & cred_type){
    if(strcasecmp(cred_type.c_str(), "PEM") ==0)
        return PemCred;
    if(strcasecmp(cred_type.c_str(), "proxy") ==0)
        return ProxyCred;
    // default pkcs12
    return Pkcs12Cred;
}

/**  ssl_check : TRUE | FALSE   - enable or disable the CA check for the server certificate
*
*  cli_private_key : path      - path to the private key to use for this endpoint
*  cli_certificate : path      - path to the credential to use for this endpoint
*  cli_password : password     - password to use for this credential
*  auth_login : login		   - login to use for basic HTTP authentification
*  auth_passwd : password	   - password to use for the basic HTTP authentification
* */
static void configureSSLParams(const std::string & plugin_name,
                              const std::string & prefix,
                              Davix::RequestParams & params){

    Davix::DavixError * tmp_err = NULL;
    Davix::X509Credential cred;
    int ret = 0;

    // get ssl check
    const bool ssl_check = pluginGetParam<bool>(prefix, "ssl_check", true);
    Info(SimpleDebug::kLOW, plugin_name, "SSL CA check for davix is set to  " + std::string((ssl_check) ? "TRUE" : "FALSE"));
    params.setSSLCAcheck(ssl_check);
    // ca check
    const std::string ca_path = pluginGetParam<std::string>(prefix, "ca_path");
    if( ca_path.size() > 0){
        Info(SimpleDebug::kLOW, plugin_name, "CA Path added :  " << ca_path);
        params.addCertificateAuthorityPath(ca_path);
    }

    // setup cli type
    const std::string credential_type_str = pluginGetParam<std::string>(prefix, "cli_type", "pkcs12");
    const CredType credential_type = parseCredType(credential_type_str);
    if(credential_type != Pkcs12Cred)
        Info(SimpleDebug::kLOW, plugin_name, " CLI cert type defined to " << credential_type);
    // setup private key
    const std::string private_key_path = pluginGetParam<std::string>(prefix, "cli_private_key");
    if (private_key_path.size() > 0)
        Info(SimpleDebug::kLOW, plugin_name, " CLI priv key defined");
    // setup credential
    const std::string credential_path = pluginGetParam<std::string>(prefix, "cli_certificate");
    if (credential_path.size() > 0)
        Info(SimpleDebug::kLOW, plugin_name, " CLI CERT path is set to " + credential_path);
    // setup credential password
    const std::string credential_password = pluginGetParam<std::string>(prefix, "cli_password");
    if (credential_password.size() > 0)
        Info(SimpleDebug::kLOW, plugin_name, " CLI CERT password defined");

    if (credential_path.size() > 0) {
        switch(credential_type){
            case ProxyCred:
            case PemCred:
                ret= cred.loadFromFilePEM(private_key_path, credential_path, credential_password, &tmp_err);
            break;
            default:
                ret= cred.loadFromFileP12(credential_path, credential_password, &tmp_err);
        }
        if(ret >= 0){
            params.setClientCertX509(cred);
        }else {
            Info(SimpleDebug::kHIGH, plugin_name, "Error: impossible to load credential "
                    + credential_path + " :" + tmp_err->getErrMsg());
            Davix::DavixError::clearError(&tmp_err);
        }
     }

}


static void configureHttpAuth(const std::string & plugin_name,
                              const std::string & prefix,
                              Davix::RequestParams & params){

    // auth login
    const std::string login = pluginGetParam<std::string>(prefix, "auth_login");
    // auth password
    const std::string password = pluginGetParam<std::string>(prefix, "auth_passwd");
    if (password.size() > 0 && login.size() > 0) {
        Info(SimpleDebug::kLOW, plugin_name, "login and password setup for authentication");
        params.setClientLoginPassword(login, password);
    }
}


static void configureHttpTimeout(const std::string & plugin_name,
                                 const std::string & prefix,
                                 Davix::RequestParams & params){
    // timeout management
    long timeout;
    struct timespec spec_timeout;
    if ((timeout =pluginGetParam<long>(prefix, "conn_timeout", 15)) != 0) {
        Info(SimpleDebug::kLOW, plugin_name, "Connection timeout is set to : " << timeout);
        spec_timeout.tv_sec = timeout;
        params.setConnectionTimeout(&spec_timeout);
    }
    if ((timeout = pluginGetParam<long>(prefix, "ops_timeout", 15)) != 0) {
        spec_timeout.tv_sec = timeout;
        params.setOperationTimeout(&spec_timeout);
        Info(SimpleDebug::kLOW, plugin_name, "Operation timeout is set to : " << timeout);
    }
}


static void configureFlags(const std::string & plugin_name,
                           const std::string & prefix,
                           int & flags){
    const bool metalink_support = pluginGetParam<bool>(prefix, "metalink_support", false);
    flags = ((metalink_support)?( flags | UGR_HTTP_FLAG_METALINK):( flags & ~(UGR_HTTP_FLAG_METALINK)));
    Info(SimpleDebug::kLOW, plugin_name, " Metalink support " << metalink_support);
}

static void configureHeader(const std::string & plugin_name,
                            const std::string & prefix,
                            Davix::RequestParams & params) {

    char *s = 0;
    int p = 0;
    while(1) {
        std::ostringstream ss;
        ss << prefix << "." << "custom_header";
        CFG->ArrayGetString(ss.str().c_str(), s, p++);
        if (s) {
            Info(SimpleDebug::kLOW, plugin_name, " Configuring additional headers #" << p << ":" << s);
            vector<string> vs = tokenize(s, ":");
            if (vs.size() > 1) params.addHeader(vs[0], vs[1]);
        }
        else break;

    }
}



UgrLocPlugin_http::UgrLocPlugin_http(UgrConnector & c, std::vector<std::string> & parms) :
    LocationPlugin(c, parms), flags(0), dav_core(), pos(&dav_core) {
    Info(SimpleDebug::kLOW, "UgrLocPlugin_[http/dav]", "Creating instance named " << name);
    // try to get config
    const int params_size = parms.size();
    if (params_size > 3) {
        Info(SimpleDebug::kLOW, "UgrLocPlugin_[http/dav]", "Try to bind UgrLocPlugin_[http/dav] with " << parms[3]);
        base_url_endpoint = Davix::Uri(parms[3]);

    } else {
        Error("UgrLocPlugin_[http/dav]", "Not enough parameters in the plugin line.");
        throw std::runtime_error("No correct parameter for this Plugin : Unable to load the plugin properly ");
    }
    load_configuration(CONFIG_PREFIX + name);
    params.setProtocol(Davix::RequestProtocol::Http);
}

void UgrLocPlugin_http::load_configuration(const std::string & prefix) {
    configureSSLParams(name, prefix, params);
    configureHttpAuth(name, prefix, params);
    configureHttpTimeout(name, prefix, params);
    configureFlags(name, prefix, flags);
    configureHeader(name, prefix, params);

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
    std::string canonical_name = base_url_endpoint.getString();
    std::string xname;
    std::vector<Davix::File> replica_vec;
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
        else canonical_name.append(xname);
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
        canonical_name.append(xname);
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
            if(flags & UGR_HTTP_FLAG_METALINK){
                LocPluginLogInfoThr(SimpleDebug::kHIGH, fname, "invoking Locate with metalink support");
                Davix::File f(dav_core, canonical_name);
                replica_vec = f.getReplicas(&params, &tmp_err);
                if(tmp_err){
                    LocPluginLogInfoThr(SimpleDebug::kHIGH, fname, "Impossible to use Metalink, code " << ((int)tmp_err->getStatus()) << " error "<< tmp_err->getErrMsg());
                }
            }

            if( (flags & UGR_HTTP_FLAG_METALINK) == false || tmp_err != NULL){
                Davix::DavixError::clearError(&tmp_err);
                if(pos.stat(&params, canonical_name, &st, &tmp_err) >=0)
                    replica_vec.push_back(Davix::File(dav_core, canonical_name));
            }
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
                for(std::vector<Davix::File>::iterator it = replica_vec.begin(); it != replica_vec.end(); ++it){
                    UgrFileItem_replica itr;
                    itr.name = HttpUtils::protocolHttpNormalize(it->getUri().getString());
                    HttpUtils::pathHttpNomalize(itr.name);
                    itr.pluginID = myID;
                    LocPluginLogInfoThr(SimpleDebug::kHIGHEST, fname, "Worker: Inserting replicas " << itr.name);

                    // We have modified the data, hence set the dirty flag
                    op->fi->dirtyitems = true;

                    op->fi->addReplica(itr);
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

void UgrLocPlugin_http::run_Check(int myidx) {
    do_CheckInternal(myidx, "UgrLocPlugin_http::do_Check");
}

void UgrLocPlugin_http::do_CheckInternal(int myidx, const char* fname){

    struct timespec t1, t2;
    Davix::DavixError* tmp_err = NULL;

    PluginEndpointStatus st;
    st.errcode = 404;

    LocPluginLogInfo(SimpleDebug::kHIGH, fname, "Start checker for " << base_url_endpoint << " with time " << availInfo.time_interval_ms);
    // Measure the time needed
    clock_gettime(CLOCK_MONOTONIC, &t1);

    Davix::HeadRequest req(dav_core, base_url_endpoint, &tmp_err);

    if( tmp_err != NULL){
        Error(fname, "Status Checker: Impossible to initiate Query to" << base_url_endpoint << ", Error: "<< tmp_err->getErrMsg());
        return;
    }

    // Set decent timeout values for the operation
    req.setParameters(checker_params);

    if (req.executeRequest(&tmp_err) == 0)
        st.errcode = req.getRequestCode();

    // Prepare the text status message to display
    if (tmp_err) {
        std::ostringstream ss;
        ss << "HTTP status error on " << base_url_endpoint << " " << tmp_err->getErrMsg();
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


    LocPluginLogInfo(SimpleDebug::kHIGHEST, fname, " End checker for " << base_url_endpoint);

}



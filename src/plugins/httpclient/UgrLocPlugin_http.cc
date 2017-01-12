/*
 *  Copyright (c) CERN 2013
 *
 *  Copyright (c) Members of the EMI Collaboration. 2011-2013
 *  See  http://www.eu-emi.eu/partners for details on the copyright
 *  holders.
 *
 *  Licensed under the Apache License, Version 2.0
 *  See the LICENSE file for further information
 *
 */





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

struct X509SecParams{
    //
    CredType type;
    std::string key_path;
    std::string key_passwd;
    std::string cred_path;
};

static int X509CredCallback(const Davix::SessionInfo & info, Davix::X509Credential& cred, X509SecParams sec, std::string plugin_name){

    int ret =0;
    Davix::DavixError* tmp_err=NULL;

    switch(sec.type){
        case ProxyCred:
        case PemCred:
            ret= cred.loadFromFilePEM(sec.key_path, sec.cred_path, sec.key_passwd, &tmp_err);
        break;
        default:
            ret= cred.loadFromFileP12(sec.cred_path, sec.key_passwd, &tmp_err);
    }

    if( ret < 0){
        throw Davix::DavixException(&tmp_err);
    }
    return 0;
}


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

    // get ssl check
    const bool ssl_check = pluginGetParam<bool>(prefix, "ssl_check", true);
    Info(UgrLogger::Lvl1, plugin_name, "SSL CA check for davix is set to  " + std::string((ssl_check) ? "TRUE" : "FALSE"));
    params.setSSLCAcheck(ssl_check);
    // ca check
    const std::string ca_path = pluginGetParam<std::string>(prefix, "ca_path");
    if( ca_path.size() > 0){
        Info(UgrLogger::Lvl1, plugin_name, "CA Path added :  " << ca_path);
        params.addCertificateAuthorityPath(ca_path);
    }

    // setup cli type
    X509SecParams sec;
    const std::string credential_type_str = pluginGetParam<std::string>(prefix, "cli_type", "pkcs12");
    sec.type = parseCredType(credential_type_str);
    if(sec.type != Pkcs12Cred)
        Info(UgrLogger::Lvl1, plugin_name, " CLI cert type defined to " << sec.type);
    // setup private key
    sec.key_path = pluginGetParam<std::string>(prefix, "cli_private_key");
    if (sec.key_path.size() > 0)
        Info(UgrLogger::Lvl1, plugin_name, " CLI priv key defined");
    // setup credential
    sec.cred_path = pluginGetParam<std::string>(prefix, "cli_certificate");
    if (sec.cred_path.size() > 0)
        Info(UgrLogger::Lvl1, plugin_name, " CLI CERT path is set to " + sec.cred_path);
    // setup credential password
    sec.key_passwd = pluginGetParam<std::string>(prefix, "cli_password");
    if (sec.key_passwd.size() > 0)
        Info(UgrLogger::Lvl1, plugin_name, " CLI CERT password defined");

    if (sec.key_path.size() > 0) {
        using namespace boost;
        params.setClientCertFunctionX509(bind(X509CredCallback, _1 , _2, sec, plugin_name));
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
        Info(UgrLogger::Lvl1, plugin_name, "login and password setup for authentication");
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
        Info(UgrLogger::Lvl1, plugin_name, "Connection timeout is set to : " << timeout);
        spec_timeout.tv_sec = timeout;
        spec_timeout.tv_nsec =0;
        params.setConnectionTimeout(&spec_timeout);
    }
    if ((timeout = pluginGetParam<long>(prefix, "ops_timeout", 15)) != 0) {
        spec_timeout.tv_sec = timeout;
        spec_timeout.tv_nsec = 0;
        params.setOperationTimeout(&spec_timeout);
        Info(UgrLogger::Lvl1, plugin_name, "Operation timeout is set to : " << timeout);
    }

}


static void configureFlags(const std::string & plugin_name,
                           const std::string & prefix,
                           int & flags,
                           Davix::RequestParams & params){
    const bool metalink_support = pluginGetParam<bool>(prefix, "metalink_support", false);
    if(metalink_support){
        flags |= UGR_HTTP_FLAG_METALINK;
    }else{
        flags &= ~(UGR_HTTP_FLAG_METALINK);
        params.setMetalinkMode(Davix::MetalinkMode::Disable);
    }
    Info(UgrLogger::Lvl1, plugin_name, " Metalink support " << metalink_support);
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
            Info(UgrLogger::Lvl1, plugin_name, " Configuring additional headers #" << p << ":" << s);
            vector<string> vs = tokenize(s, ":");
            if (vs.size() > 1) params.addHeader(vs[0], vs[1]);
        }
        else break;

    }
}



UgrLocPlugin_http::UgrLocPlugin_http(UgrConnector & c, std::vector<std::string> & parms) :
    LocationPlugin(c, parms), flags(0), dav_core(), pos(&dav_core) {
    Info(UgrLogger::Lvl1, "UgrLocPlugin_[http/dav]", "Creating instance named " << name);
    // try to get config
    const int params_size = parms.size();
    if (params_size > 3) {
        Info(UgrLogger::Lvl1, "UgrLocPlugin_[http/dav]", "Try to bind UgrLocPlugin_[http/dav] with " << parms[3]);
        base_url_endpoint = Davix::Uri(parms[3]);
        checker_url = base_url_endpoint;
    } else {
        Error("UgrLocPlugin_[http/dav]", "Not enough parameters in the plugin line.");
        throw std::runtime_error("Incorrect parameters for this Plugin : Unable to load the plugin.");
    }
    load_configuration(getConfigPrefix() + name);
    params.setProtocol(Davix::RequestProtocol::Http);

    params.setOperationRetry(1);
}

void UgrLocPlugin_http::load_configuration(const std::string & prefix) {
    configureSSLParams(name, prefix, params);
    configureHttpAuth(name, prefix, params);
    configureHttpTimeout(name, prefix, params);
    configureFlags(name, prefix, flags, params);
    configureHeader(name, prefix, params);

    checker_params = params;
    struct timespec spec_timeout;
    spec_timeout.tv_sec = max(this->availInfo.time_interval_ms / 1000, 1);
    spec_timeout.tv_nsec = 0;
    checker_params.setOperationRetry(1);
    checker_params.setOperationTimeout(&spec_timeout);
    checker_params.setConnectionTimeout(&spec_timeout);
    // disable KeepAlive for checker
    checker_params.setKeepAlive(false);
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

    // Doublecheck if we are disabled. If so, quickly close the pending requests
    bool imdisabled = !availInfo.isOK();

    if (op->wop == wop_CheckReplica) {

        // Do the default name translation for this plugin (prefix xlation)
        if (imdisabled || doNameXlation(op->repl, xname, op->wop, op->altpfx)) {
            unique_lock<mutex> l(*(op->fi));
            op->fi->notifyLocationNotPending();
            return;
        }// Then prepend the URL prefix
        else canonical_name.append(xname);
    } else {

        // Do the default name translation for this plugin (prefix xlation)
        if (imdisabled || doNameXlation(op->fi->name, xname, op->wop, op->altpfx)) {
            unique_lock<mutex> l(*(op->fi));
            switch (op->wop) {
                case LocationPlugin::wop_Stat:
		    LocPluginLogInfoThr(UgrLogger::Lvl3, fname, "Short-circuit on Stat() " << canonical_name << ")");
                    op->fi->notifyStatNotPending();
                    break;

                case LocationPlugin::wop_Locate:
		    LocPluginLogInfoThr(UgrLogger::Lvl3, fname, "Short-circuit on Locate() " << canonical_name << ")");
                    op->fi->notifyLocationNotPending();
                    break;

                case LocationPlugin::wop_List:
		    LocPluginLogInfoThr(UgrLogger::Lvl3, fname, "Short-circuit on List() " << canonical_name << ")");
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
            LocPluginLogInfoThr(UgrLogger::Lvl3, fname, "invoking davix_Stat(" << canonical_name << ")");
            pos.stat(&params, canonical_name, &st, &tmp_err);
            // force path finishing with '/' like a directory, impossible to get the type of a file in plain http
            if (canonical_name.at(canonical_name.length() - 1) == '/') {
                st.st_mode |= S_IFDIR;
            }
            break;

        case LocationPlugin::wop_Locate:
            LocPluginLogInfoThr(UgrLogger::Lvl3, fname, "invoking Locate(" << canonical_name << ")");
            if(flags & UGR_HTTP_FLAG_METALINK){
                LocPluginLogInfoThr(UgrLogger::Lvl3, fname, "invoking Locate with metalink support");
                Davix::File f(dav_core, canonical_name);
                replica_vec = f.getReplicas(&params, &tmp_err);
                if(tmp_err){
                    LocPluginLogInfoThr(UgrLogger::Lvl3, fname, "Impossible to use Metalink, code " << ((int)tmp_err->getStatus()) << " error "<< tmp_err->getErrMsg());
                }
            }

            if( (flags & UGR_HTTP_FLAG_METALINK) == false || tmp_err != NULL){
                Davix::DavixError::clearError(&tmp_err);
                if(pos.stat(&params, canonical_name, &st, &tmp_err) >=0)
                    replica_vec.push_back(Davix::File(dav_core, canonical_name));
            }
            break;

        case LocationPlugin::wop_CheckReplica:
            LocPluginLogInfoThr(UgrLogger::Lvl3, fname, "invoking CheckReplica(" << canonical_name << ")");
            pos.stat(&params, canonical_name, &st, &tmp_err);
            break;

        default:
            break;
    }

    if (!tmp_err) {
        bad_answer = false; // reach here -> request complete
    } else {
        LocPluginLogInfoThr(UgrLogger::Lvl3, fname, " UgrHttp plugin request Error : " << ((int) tmp_err->getStatus()) << " errMsg: " << tmp_err->getErrMsg());
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

            case LocationPlugin::wop_Locate:
            {
                for(std::vector<Davix::File>::iterator it = replica_vec.begin(); it != replica_vec.end(); ++it){
                    UgrFileItem_replica itr;
                    itr.name = HttpUtils::protocolHttpNormalize(it->getUri().getString());
                    HttpUtils::pathHttpNomalize(itr.name);
                    itr.pluginID = getID();
                    LocPluginLogInfoThr(UgrLogger::Lvl4, fname, "Worker: Inserting replicas " << itr.name);

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

                itr.pluginID = getID();
                LocPluginLogInfoThr(UgrLogger::Lvl4, fname, "Worker: Inserting replicas " << itr.name);

                // We have modified the data, hence set the dirty flag
                op->fi->dirtyitems = true;

                op->fi->addReplica(itr);

                break;
            }

            default:
                break;
        }


        if (tmp_err) {
            LocPluginLogInfoThr(UgrLogger::Lvl3, fname, " UgrHttp plugin request Error : " << ((int) tmp_err->getStatus()) << " errMsg: " << tmp_err->getErrMsg());
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
            case LocationPlugin::wop_CheckReplica:
                LocPluginLogInfoThr(UgrLogger::Lvl4, fname, "Notify End Locate");
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
    st.errcode = -1;

    LocPluginLogInfo(UgrLogger::Lvl3, fname, "Start checker for " << checker_url << " with time " << availInfo.time_interval_ms);
    // Measure the time needed
    clock_gettime(CLOCK_MONOTONIC, &t1);

    Davix::HeadRequest req(dav_core, checker_url, &tmp_err);

    if( tmp_err != NULL){
        Error(fname, "Status Checker: Impossible to initiate Query to" << checker_url << ", Error: "<< tmp_err->getErrMsg());
        Davix::DavixError::clearError(&tmp_err);
        return;
    }

    // Set decent timeout values for the operation
    req.setParameters(checker_params);

    req.executeRequest(&tmp_err);
    st.errcode = req.getRequestCode();

    // Finish measuring the time needed
    clock_gettime(CLOCK_MONOTONIC, &t2);

    // Calculate the latency
    struct timespec diff_time;
    timespec_sub(&t2, &t1, &diff_time);
    st.latency_ms = (diff_time.tv_sec)*1000 + (diff_time.tv_nsec) / 1000000L;

    // We assume that the prefix directory was set correctly, and we worry only
    // about the functionality of the endpoint
    // Special case: azure returns 400 when statting the base of a bucket
    if ( ((st.errcode >= 200) && (st.errcode < 400)) || (st.errcode == 404) ||
          (st.errcode == 400 && !checker_params.getAzureKey().empty()) ) {
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
        std::ostringstream ss;
        ss << "Error when contacting '" << checker_url << "'. Status code: " << req.getRequestCode() << ". ";
        if(tmp_err) ss << "DavixError: '" << tmp_err->getErrMsg() << "'";
        st.explanation = ss.str();

        st.state = PLUGIN_ENDPOINT_OFFLINE;
    }

    st.lastcheck = time(0);
    availInfo.setStatus(st, true, (char *) name.c_str());

    // Propagate this fresh result to the extcache
    if (extCache)
        extCache->putEndpointStatus(&st, name);

    Davix::DavixError::clearError(&tmp_err);
    LocPluginLogInfo(UgrLogger::Lvl4, fname, " End checker for " << base_url_endpoint);
}


int UgrLocPlugin_http::run_findNewLocation(const std::string & lfn, std::shared_ptr<NewLocationHandler> handler){
  std::string new_lfn(lfn);
  static const char * fname = "UgrLocPlugin_http::run_findNewLocation";
  std::string canonical_name(base_url_endpoint.getString());
  std::string xname;
  std::string alt_prefix;

  // do name translation
  if(doNameXlation(new_lfn, xname, wop_Nop, alt_prefix) != 0){
    LocPluginLogInfoThr(UgrLogger::Lvl4, fname, "can not be translated " << new_lfn);
    return 1;
  }



  canonical_name.append("/");
  canonical_name.append(xname);

  std::string new_Location = HttpUtils::protocolHttpNormalize(canonical_name);
  HttpUtils::pathHttpNomalize(new_Location);

  handler->addReplica(new_Location, getID());
  LocPluginLogInfoThr(UgrLogger::Lvl3, fname, "newLocation found with success " << new_Location);
  return 0;

}


int UgrLocPlugin_http::run_deleteReplica(const string & lfn, const std::shared_ptr<DeleteReplicaHandler> handler){
    std::string new_lfn(lfn);
    static const char * fname = "UgrLocPlugin_http::run_deleteReplica";
    std::string canonical_name(base_url_endpoint.getString());
    std::string xname;
    std::string alt_prefix;

    // do name translation
    if(doNameXlation(new_lfn, xname, wop_Nop, alt_prefix) != 0){
          LocPluginLogInfoThr(UgrLogger::Lvl4, fname, "can not be translated " << new_lfn);
          return 1;
    }


    if(concat_http_url_path(canonical_name, xname, canonical_name) == false){
        return 1;
    }


    try{
        LocPluginLogInfoThr(UgrLogger::Lvl3, fname, "Try Deletion for  " << canonical_name);
        Davix::File f(dav_core, canonical_name);
        f.deletion(&params);
        LocPluginLogInfoThr(UgrLogger::Lvl3, fname, "Deletion done with success for  " << canonical_name);

        UgrFileItem_replica rep;
        rep.name = canonical_name;
        rep.status = UgrFileItem_replica::Deleted;
        handler->addReplica(rep, getID());
        return 0;

    }catch(Davix::DavixException & e){
        LocPluginLogInfoThr(UgrLogger::Lvl3, fname, "Error on Deletion: " << e.what());
    }catch(...){
        LocPluginLogInfoThr(UgrLogger::Lvl3, fname, "Unknown Error on Deletion");
    }
    return 0;

}


int UgrLocPlugin_http::run_deleteDir(const string & lfn, const std::shared_ptr<DeleteReplicaHandler> handler){
    std::string new_lfn(lfn);
    static const char * fname = "UgrLocPlugin_http::run_deleteDir";
    std::string canonical_name(base_url_endpoint.getString());
    std::string xname;
    std::string alt_prefix;

    // do name translation
    if(doNameXlation(new_lfn, xname, wop_Nop, alt_prefix) != 0){
          LocPluginLogInfoThr(UgrLogger::Lvl4, fname, "can not be translated " << new_lfn);
          return 1;
    }


    if(concat_http_url_path(canonical_name, xname, canonical_name) == false){
        return 1;
    }


    try{
        LocPluginLogInfoThr(UgrLogger::Lvl3, fname, "Try Deletion for  " << canonical_name);
        Davix::File f(dav_core, canonical_name);
        f.deletion(&params);
        LocPluginLogInfoThr(UgrLogger::Lvl3, fname, "Deletion done with success for  " << canonical_name);

        UgrFileItem_replica rep;
        rep.name = canonical_name;
        rep.status = UgrFileItem_replica::Deleted;
        handler->addReplica(rep, getID());
        return 0;

    }catch(Davix::DavixException & e){
        LocPluginLogInfoThr(UgrLogger::Lvl3, fname, "Error on Deletion: " << e.what());
    }catch(...){
        LocPluginLogInfoThr(UgrLogger::Lvl3, fname, "Unknown Error on Deletion");
    }
    return 0;

}


// concat URI + path
bool UgrLocPlugin_http::concat_http_url_path(const std::string & base_uri, const std::string & path, std::string & canonical){
    //static const char * fname = "UgrLocPlugin_http::concat_http_url_path";
    // remove "//", not sure if this is the right thing to do, need to double check
    auto it = path.begin();
    while(*it == '/' && it < path.end())
        it++;
/*
    if(it == path.end()){
        LocPluginLogInfoThr(UgrLogger::Lvl3, fname, "bucket name, ignore " << path);
        return false;
    }
*/
    canonical = base_uri;
    canonical.append("/");
    canonical.append(it, path.end());
    return true;
}

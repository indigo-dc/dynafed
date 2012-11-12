/** 
 * @file   UgrLocPlugin_http.cc
 * @brief  Plugin that talks to any Webdav compatible endpoint
 * @author Devresse Adrien
 * @date   Feb 2012
 */


#include "UgrLocPlugin_http.hh"
#include "../../PluginLoader.hh"
#include <time.h>


const std::string CONFIG_PREFIX("glb.locplugin.");
const std::string config_timeout_conn_key("conn_timeout");
const std::string config_timeout_ops_key("ops_timeout");
const std::string config_endpoint_state_check("status_checking");
const std::string config_endpoint_checker_poll_frequency("status_checker_frequency");
const std::string config_endpoint_checker_max_latency("max_latency");

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
    g_logger_set_globalfilter(G_LOG_LEVEL_WARNING);
    return (LocationPlugin *)new UgrLocPlugin_http(dbginstance, cfginstance, parms);
}

/**
 * Davix callback for Ugr, Allow clicert authentification
 * */
int UgrLocPlugin_http::davix_credential_callback(davix_auth_t token, const davix_auth_info_t* t, void* userdata, Davix_error** err) {
    Davix::DavixError * tmp_err = NULL;
    int ret = -1;
    UgrLocPlugin_http* me = static_cast<UgrLocPlugin_http*> (userdata);

    Info(SimpleDebug::kLOW, "UgrLocPlugin_http", " Davix request for credential ..... ");

    switch (t->auth) {
        case DAVIX_CLI_CERT_PKCS12:
            ret = davix_auth_set_pkcs12_cli_cert(token, me->pkcs12_credential_path.c_str(),
                    (me->pkcs12_credential_password.size() == 0) ? NULL : me->pkcs12_credential_password.c_str(),
                    (Davix_error**)  &tmp_err);
            if (ret != 0) {
                Info(SimpleDebug::kLOW, "UgrLocPlugin_http", " Ugr davix plugin, Unable to set credential properly, Error : " << tmp_err->getErrMsg());
            }
            break;
        case DAVIX_LOGIN_PASSWORD:
            ret = davix_auth_set_login_passwd(token, me->login.c_str(), me->password.c_str(), (Davix_error**) &tmp_err);
            if (ret != 0) {
                Info(SimpleDebug::kLOW, "UgrLocPlugin_http", " Ugr davix plugin, Unable to set login/password Error :" << tmp_err->getErrMsg());
            }
            break;
        default:
            Davix::DavixError::setupError(&tmp_err, std::string("UgrLocPlugin_http::davix_credential_callback"), Davix::StatusCode::authentificationError, " Unsupported authentification required by davix ! bug ");
    }
    if (tmp_err)
        Davix::DavixError::propagateError( (Davix::DavixError**) err, tmp_err);
    return ret;
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
        throw std::runtime_error("No correct parameter for this Plugin : Unable to load the plugin properly ");
    }
    load_configuration(CONFIG_PREFIX + name);

    params.setSSLCAcheck(ssl_check);
    params.setAuthentificationCallback(this, &UgrLocPlugin_http::davix_credential_callback);
    params.setProtocol(DAVIX_PROTOCOL_HTTP);
   // dav_core->getSessionFactory()->set_parameters(params);

}

void UgrLocPlugin_http::load_configuration(const std::string & prefix) {
    Config * c = Config::GetInstance();
    std::string pref_dot = prefix + std::string(".");
    guint latency;

    // get ssl check
    ssl_check = c->GetBool(pref_dot + std::string("ssl_check"), true);
    Info(SimpleDebug::kLOW, "UgrLocPlugin_http", " SSL CA check for davix is set to  " + std::string((ssl_check) ? "TRUE" : "FALSE"));
    // get credential
    pkcs12_credential_path = c->GetString(pref_dot + std::string("cli_certificate"), "");
    if (pkcs12_credential_path.size() > 0) {
        Info(SimpleDebug::kLOW, "UgrLocPlugin_http", " CLI CERT path is set to  " + pkcs12_credential_path);
    }
    // get credential password
    pkcs12_credential_password = c->GetString(pref_dot + std::string("cli_password"), "");
    if (pkcs12_credential_password.size() > 0) {
        Info(SimpleDebug::kLOW, "UgrLocPlugin_http", " CLI CERT passwrd defined  ");
    }
    // auth login
    login = c->GetString(pref_dot + std::string("auth_login"), "");
    if (login.size() > 0) {
        Info(SimpleDebug::kLOW, "UgrLocPlugin_http", " basic auth login defined  ");
    }
    // auth password
    password = c->GetString(pref_dot + std::string("auth_passwd"), "");
    if (password.size() > 0) {
        Info(SimpleDebug::kLOW, "UgrLocPlugin_http", " basic auth password defined  ");
    }
    // get state checker
    state_checking = c->GetBool(pref_dot + config_endpoint_state_check, true);
    Info(SimpleDebug::kLOW, "UgrLocPlugin_http", " State checker : " << ((state_checking) ? "ENABLED" : "DISABLED"));

    state_checker_freq = c->GetLong(pref_dot + config_endpoint_checker_poll_frequency, 5000);
    Info(SimpleDebug::kLOW, "UgrLocPlugin_http", " State checker frequency : " << state_checker_freq);
    // get maximum latency
    latency = c->GetLong(pref_dot + config_endpoint_checker_max_latency, 10000);
    max_latency.tv_sec = latency /1000;
    max_latency.tv_nsec = (latency%1000)*1000000;
    Info(SimpleDebug::kLOW, "UgrLocPlugin_http", " Maximum Endpoint latency " << (int)max_latency.tv_sec << "s "<<(int) max_latency.tv_nsec << "ns"  );


    // timeout management
    long timeout;
    struct timespec spec_timeout;
    if ((timeout = c->GetLong(pref_dot + config_timeout_conn_key, 0)) != 0) {
        Info(SimpleDebug::kLOW, "UgrLocPlugin_http", " Connection timeout is set to : " << timeout);
        spec_timeout.tv_sec = timeout;
        params.setConnexionTimeout(&spec_timeout);
    }
    if ((timeout = c->GetLong(pref_dot + config_timeout_ops_key, 0)) != 0) {
        spec_timeout.tv_sec = timeout;
        params.setOperationTimeout(&spec_timeout);
        Info(SimpleDebug::kLOW, "UgrLocPlugin_http", " Operation timeout is set to : " << timeout);
    }
}

void UgrLocPlugin_http::check_availability(PluginEndpointStatus *status, UgrFileInfo *fi) {
    if (state_checking)
        state_checker->get_availability(status);
    else
        LocationPlugin::check_availability(status, fi);

}

int UgrLocPlugin_http::start() {
    if (state_checking) {
        state_checker = boost::shared_ptr<HttpAvailabilityChecker > (new HttpAvailabilityChecker(dav_core.get(), base_url, state_checker_freq, &max_latency));
    }
    return LocationPlugin::start();
}

void UgrLocPlugin_http::stop() {
    LocationPlugin::stop();
    if (state_checking) {
        state_checking = false;
        state_checker.reset();
    }

}

void UgrLocPlugin_http::runsearch(struct worktoken *op, int myidx) {
    struct stat st;
    Davix::DavixError * tmp_err=NULL;
    static const char * fname = "UgrLocPlugin_http::runsearch";
    std::string cannonical_name = base_url;
    bool bad_answer = true;

    // We act using the identity of this service, hence we don't need to invoke
    // getIdMap/setUserblahblah
    if (op == NULL || op->fi == NULL) {
        LocPluginLogInfoThr(SimpleDebug::kHIGH, fname, " Bad request Handle : FATAL");
        return;
    }


    //cannonical_name += "/";
    cannonical_name += op->fi->name;

    memset(&st, 0, sizeof (st));

	switch (op->wop) {

		case LocationPlugin::wop_Stat:
			LocPluginLogInfoThr(SimpleDebug::kHIGH, fname, "invoking davix_Stat(" << cannonical_name << ")");
			pos.stat(&params, cannonical_name, &st, &tmp_err);
			break;

		case LocationPlugin::wop_Locate:
			LocPluginLogInfoThr(SimpleDebug::kHIGH, fname, "invoking Locate(" << cannonical_name << ")");
			pos.stat(&params, cannonical_name, &st, &tmp_err);
			break;


		default:
			break;
	}
        
    if(!tmp_err){
		bad_answer = false; // reach here -> request complete
	}else{
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
				itr.name = cannonical_name;
				itr.pluginID = myID;
				LocPluginLogInfoThr(SimpleDebug::kHIGHEST, fname, "Worker: Inserting replicas " << cannonical_name);

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

			default:
				break;
		}


        if(tmp_err){
            LocPluginLogInfoThr(SimpleDebug::kHIGH, fname, " UgrHttp plugin request Error : "  <<((int) tmp_err->getStatus()) << " errMsg: " << tmp_err->getErrMsg());
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

            default:
                break;
        }

    }

}

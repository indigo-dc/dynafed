/** 
 * @file   UgrLocPlugin_dav.cc
 * @brief  Plugin that talks to any Webdav compatible endpoint
 * @author Devresse Adrien
 * @date   Feb 2012
 */

#include <davix_cpp.hpp>

#include "UgrLocPlugin_dav.hh"
#include "../../PluginLoader.hh"
#include <time.h>


const std::string CONFIG_PREFIX("glb.locplugin.");

using namespace boost;
using namespace std;


// ------------------------------------------------------------------------------------
// Plugin-related stuff
// ------------------------------------------------------------------------------------


/***
 * List of arguments for the dav plugin :
 *  1st one : URL prefix to contact
 *  2nd : credential path to use
 *  3rd : mode SSL CA check : TRUE or nothing > secure, FALSE -> disable CA check, insecure ( allow man in the middle attack) * 
 * 
 * */

// The hook function. GetLocationPluginClass must be given the name of this function
// for the plugin to be loaded

/**
 * Hook for the dav plugin Location plugin
 * 
 * 
 * */
extern "C" LocationPlugin *GetLocationPlugin(GetLocationPluginArgs) {
	g_logger_set_globalfilter(G_LOG_LEVEL_WARNING);
    return (LocationPlugin *)new UgrLocPlugin_dav(dbginstance, cfginstance, parms);
}

/**
 * Davix callback for Ugr, Allow clicert authentification
 * */
int UgrLocPlugin_dav::davix_credential_callback(davix_auth_t token, const davix_auth_info_t* t, void* userdata, GError** err) {
    GError * tmp_err = NULL;
    int ret=-1;
    UgrLocPlugin_dav* me = static_cast<UgrLocPlugin_dav*>(userdata);   
    
    Info(SimpleDebug::kLOW, "UgrLocPlugin_dav", " Davix request for credential ..... ");
	
	switch(t->auth){
		case DAVIX_CLI_CERT_PKCS12:
			ret= davix_set_pkcs12_auth(token, me->pkcs12_credential_path.c_str(), 
					(me->pkcs12_credential_password.size() == 0)?NULL:me->pkcs12_credential_password.c_str(), 
					&tmp_err);
			if (ret != 0) {
				Info(SimpleDebug::kLOW, " Ugr davix plugin, Unable to set credential properly, Error : %s", tmp_err->message);
			}
			break;
  		case DAVIX_LOGIN_PASSWORD:  
  			ret= davix_set_login_passwd_auth(token, me->login.c_str(), me->password.c_str(), &tmp_err);
			if (ret != 0) {
				Info(SimpleDebug::kLOW, " Ugr davix plugin, Unable to set login/password Error : %s", tmp_err->message);
			}
			break;
		default:
			g_set_error(&tmp_err,g_quark_from_string("UgrLocPlugin_dav::davix_credential_callback"), EFAULT, " Unsupported authentification required by davix ! bug ");
	}
	if(tmp_err)
		g_propagate_error(err, tmp_err);
    return ret;
}

UgrLocPlugin_dav::UgrLocPlugin_dav(SimpleDebug *dbginstance, Config *cfginstance, std::vector<std::string> &parms) :
LocationPlugin(dbginstance, cfginstance, parms), dav_core(Davix::session_create()) {
    Info(SimpleDebug::kLOW, "UgrLocPlugin_dav", "Creating instance named " << name);
    // try to get config
    const int params_size = parms.size();
    if (params_size > 3) {
        Info(SimpleDebug::kLOW, "UgrLocPlugin_dav", "Try to bind UgrLocPlugin_dav with " << parms[3]);
        base_url = parms[3];
    } else {
        throw std::runtime_error("No correct parameter for this Plugin : Unable to load the plugin properly ");
    }
    load_configuration(CONFIG_PREFIX + name);
    
    dav_core->getSessionFactory()->set_ssl_ca_check(ssl_check);
    dav_core->getSessionFactory()->set_authentification_controller(this, &UgrLocPlugin_dav::davix_credential_callback);

}

void UgrLocPlugin_dav::load_configuration(const std::string & prefix){
	Config * c = Config::GetInstance();
	std::string pref_dot = prefix + std::string(".");
	
	// get ssl check
	ssl_check = c->GetBool(pref_dot + std::string("ssl_check"), true);
	Info(SimpleDebug::kLOW, "UgrLocPlugin_dav", " SSL CA check for davix is set to  " + std::string((ssl_check)?"TRUE":"FALSE") );
	// get credential 
	pkcs12_credential_path = c->GetString(pref_dot + std::string("cli_certificate"), "");
	if(pkcs12_credential_path.size() > 0){
		Info(SimpleDebug::kLOW, "UgrLocPlugin_dav", " CLI CERT path is set to  " + pkcs12_credential_path);	
	}
	// get credential password
	pkcs12_credential_password = c->GetString(pref_dot + std::string("cli_password"), "");
	if(pkcs12_credential_password.size() > 0){	
		Info(SimpleDebug::kLOW, "UgrLocPlugin_dav", " CLI CERT passwrd defined  ");		
	}
	// auth login
	login = c->GetString(pref_dot + std::string("auth_login"), "");
	if(login.size() > 0){	
		Info(SimpleDebug::kLOW, "UgrLocPlugin_dav", " basic auth login defined  ");		
	}	
    // auth password
	password = c->GetString(pref_dot + std::string("auth_passwd"), "");
	if(password.size() > 0){	
		Info(SimpleDebug::kLOW, "UgrLocPlugin_dav", " basic auth password defined  ");		
	}	    
}

void UgrLocPlugin_dav::runsearch(struct worktoken *op, int myidx) {
    struct stat st;
    static const char * fname = "UgrLocPlugin_dav::runsearch";
    std::string cannonical_name = base_url;
    bool bad_answer = true;
    DAVIX_DIR* d = NULL;
    bool listerror = false;

    // We act using the identity of this service, hence we don't need to invoke
    // getIdMap/setUserblahblah
    if (op == NULL || op->fi == NULL) {
        LocPluginLogInfoThr(SimpleDebug::kHIGH, fname, " Bad request Handle : FATAL");
        return;
    }
    cannonical_name += "/";
    cannonical_name += op->fi->name;

    try {
        switch (op->wop) {

            case LocationPlugin::wop_Stat:
                LocPluginLogInfoThr(SimpleDebug::kHIGH, fname, "invoking davix_Stat(" << cannonical_name << ")");
                dav_core->stat(cannonical_name, &st);
                break;

            case LocationPlugin::wop_Locate:
                LocPluginLogInfoThr(SimpleDebug::kHIGH, fname, "invoking Locate(" << cannonical_name << ")");
                dav_core->stat(cannonical_name, &st);
                break;

            case LocationPlugin::wop_List:
                LocPluginLogInfoThr(SimpleDebug::kHIGH, fname, " invoking davix_openDir(" << cannonical_name << ")");
                d = dav_core->opendirpp(cannonical_name);
                // if reach here -> valid opendir -> specify file as well
                op->fi->unixflags |= S_IFDIR;
                break;

            default:
                break;
        }
        bad_answer = false; // reach here -> request complete
    } catch (Glib::Error & e) {
        LocPluginLogInfoThr(SimpleDebug::kHIGH, fname, " UgrDav plugin request Error : " << " name: " << g_quark_to_string(e.domain()) << " Catched exception: " << e.code() << " what: " << e.what());
    } catch (std::exception & e) {
        LocPluginLogErr(fname, " UgrDav plugin request Error : Unexcepted Error, FATAL  what: " << e.what());
    } catch (...) {
        LocPluginLogErr(fname, " UgrDav plugin request Error: Unknow Error Fatal  ");
    }





    UgrFileItem it;

    op->fi->lastupdtime = time(0);

    if (bad_answer == false) {
		try {		
			LocPluginLogInfoThr(SimpleDebug::kMEDIUM, fname, "Worker: inserting data for " << op->fi->name);
			switch (op->wop) {

				case LocationPlugin::wop_Stat:
					LocPluginLogInfoThr(SimpleDebug::kHIGHEST, fname, "Worker: stat info:" << st.st_size << " " << st.st_mode);
					op->fi->takeStat(st);
					break;

				case LocationPlugin::wop_Locate:
					it.name = cannonical_name;
					LocPluginLogInfoThr(SimpleDebug::kHIGHEST, fname, "Worker: Inserting replicas " << cannonical_name);

					// Process it with the Geo plugin, if needed
					if (geoPlugin) geoPlugin->setReplicaLocation(it);
				{
					// Lock the file instance
					unique_lock<mutex> l(*(op->fi));

					op->fi->subitems.insert(it);
				}

					break;

				case LocationPlugin::wop_List:
				{
					dirent * dent;
					long cnt = 0;
					struct stat st;
					while ((dent = dav_core->readdirpp(d, &st)) != NULL) {
						unique_lock<mutex> l(*(op->fi));


						if (cnt++ > CFG->GetLong("glb.maxlistitems", 20000)) {
							LocPluginLogInfoThr(SimpleDebug::kMEDIUM, fname, "Setting as non listable. cnt=" << cnt);
							listerror = true;
							op->fi->subitems.clear();
							break;
						}

						// create new items
						UgrFileItem it;
						LocPluginLogInfoThr(SimpleDebug::kHIGHEST, fname, "Worker: Inserting list " << dent->d_name);
						it.name = std::string(dent->d_name);
						it.location.clear();
						// populate answer
						op->fi->subitems.insert(it);
						// add childrens
						string child = op->fi->name + "/" + it.name ;
						UgrFileInfo *fi = op->handler->getFileInfoOrCreateNewOne(child);
						LocPluginLogInfoThr(SimpleDebug::kHIGHEST, fname, "Worker: Inserting readdirpp stat info for  " << dent->d_name << ", flags " << st.st_mode << " size : " << st.st_size);						
						if (fi) fi->takeStat(st);
					}
					dav_core->closedirpp(d);


				}
					break;

				default:
					break;
			}


			// We have modified the data, hence set the dirty flag
			op->fi->dirty = true;
        
		} catch (Glib::Error & e) {
			LocPluginLogInfoThr(SimpleDebug::kHIGH, fname, " UgrDav plugin request Error : " << " name: " << g_quark_to_string(e.domain()) << " Catched exception: " << e.code() << " what: " << e.what());
		} catch (std::exception & e) {
			LocPluginLogErr(fname, " UgrDav plugin request Error : Unexcepted Error, FATAL  what: " << e.what());
		} catch (...) {
			LocPluginLogErr(fname, " UgrDav plugin request Error: Unknow Error Fatal  ");
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

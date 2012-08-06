#pragma once
#ifndef LOCATIONPLUGIN_SIMPLEHTTP_HH
#define LOCATIONPLUGIN_SIMPLEHTTP_HH


/** 
 * @file   UgrLocPlugin_dav.hh
 * @brief  Plugin that talks to any Webdav compatible endpoint
 * @author Devresse Adrien
 * @date   Feb 2012
 */

#include <davix_cpp.hpp>
#include <string>
#include <glibmm.h>
#include "davavailabilitychecker.hh"
#include "ugr_loc_plugin_dav_type.hh"
#include "../../LocationPlugin.hh"


typedef Glib::RWLock DavPluginRWMutex;
typedef Glib::RWLock::ReaderLock DavPluginReadLocker;
typedef Glib::RWLock::WriterLock DavPluginWriterLocker;

class DavAvailabilityChecker;

/**
 *  Dav plugin config parameters
 *  ssl_check : TRUE | FALSE   - enable or disable the CA check for the server certificate
 *  cli_certificate : path     - path to the credential to use for this endpoint
 *  cli_password : password    - password to use for this credential
 *  auth_login : login		   - login to use for basic HTTP authentification
 *  auth_passwd : password	   - password to use for the basic HTTP authentification
 * */


/** 
 * Location Plugin for Ugr, inherit from the LocationPlugin
 *  allow to do basic query to a webdav endpoint
 **/  
class UgrLocPlugin_dav : public LocationPlugin {
protected:


public:

	/**
	 * Follow the standard LocationPlugin construction
	 * 
	 * */
    UgrLocPlugin_dav(SimpleDebug *dbginstance, Config *cfginstance, std::vector<std::string> &parms);


    /**
     *  main executor for the plugin    
     **/
     virtual void runsearch(struct worktoken *op, int myidx);

     virtual void check_availability(PluginEndpointStatus *status, UgrFileInfo *fi);

protected:
	std::string base_url;
	std::string pkcs12_credential_path;
	std::string pkcs12_credential_password;
	bool ssl_check;
	std::string login;
	std::string password;
	
	boost::scoped_ptr<Davix::CoreInterface> dav_core;
    Davix::RequestParams params;

    //plugin state checker
    bool state_checking;
    boost::shared_ptr<DavAvailabilityChecker> state_checker;
    unsigned long state_checker_freq;
	
	void load_configuration(const std::string & prefix);

    // stop the plugin behavior
    virtual void stop();

    virtual int start();
	
	static int davix_credential_callback(davix_auth_t token, const davix_auth_info_t* t, void* userdata, GError** err); 	
};



// ------------------------------------------------------------------------------------
// Plugin-related stuff
// ------------------------------------------------------------------------------------

// The plugin functionality. This function invokes the plugin loader, looking for the
// plugin where to call the hook function
LocationPlugin *GetLocationPluginClass(char *pluginPath, GetLocationPluginArgs);



#endif


#pragma once
#ifndef LOCATIONPLUGIN_SIMPLEHTTP_HH
#define LOCATIONPLUGIN_SIMPLEHTTP_HH


/** 
 * @file   UgrLocPlugin_http.hh
 * @brief  Plugin that talks to any Webdav compatible endpoint
 * @author Devresse Adrien
 * @date   Feb 2012
 */

#include <davix.hpp>
#include <string>
#include <glibmm.h>
#include "../../LocationPlugin.hh"


class HttpAvailabilityChecker;

/**
 *  Http plugin config parameters
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
class UgrLocPlugin_http : public LocationPlugin {
protected:

    virtual void do_Check();
public:

    /**
     * Follow the standard LocationPlugin construction
     * 
     * */
    UgrLocPlugin_http(SimpleDebug *dbginstance, Config *cfginstance, std::vector<std::string> &parms);


    /**
     *  main executor for the plugin    
     **/
    virtual void runsearch(struct worktoken *op, int myidx);

    virtual void check_availability(PluginEndpointStatus *status, UgrFileInfo *fi);


    /// With http we cannot do listings, hence we shortcircuit the request

    virtual int do_List(UgrFileInfo *fi, LocationInfoHandler *handler) {
        return 0;
    }
protected:
    std::string base_url;
    std::string pkcs12_credential_path;
    std::string pkcs12_credential_password;
    bool ssl_check;
    std::string login;
    std::string password;

    boost::scoped_ptr<Davix::Context> dav_core;
    Davix::DavPosix pos;
    Davix::RequestParams params;
    Davix::RequestParams checker_params;

    void load_configuration(const std::string & prefix);

};



// ------------------------------------------------------------------------------------
// Plugin-related stuff
// ------------------------------------------------------------------------------------

// The plugin functionality. This function invokes the plugin loader, looking for the
// plugin where to call the hook function
LocationPlugin *GetLocationPluginClass(char *pluginPath, GetLocationPluginArgs);



#endif


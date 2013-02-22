#pragma once
#ifndef LOCATIONPLUGIN_WEBDAV_HH
#define LOCATIONPLUGIN_WEBDAV_HH


/** 
 * @file   UgrLocPlugin_dav.hh
 * @brief  Plugin that talks to any Webdav compatible endpoint
 * @author Devresse Adrien
 * @date   Feb 2012
 */

#include <davix.hpp>
#include <string>
#include <glibmm.h>
#include "../../LocationPlugin.hh"

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

    virtual void do_Check(int myidx);
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

protected:
    std::string base_url;

    bool ssl_check;

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


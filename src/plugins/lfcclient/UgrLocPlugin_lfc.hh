#pragma once
#ifndef LOCATIONPLUGIN_SIMPLEHTTP_HH
#define LOCATIONPLUGIN_SIMPLEHTTP_HH

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
 * @file   UgrLocPlugin_lfc.hh
 * @brief  Plugin that talks to any Webdav compatible endpoint
 * @author Devresse Adrien
 * @date   Feb 2012
 */

#include <gfal_api.h>
#include <string>
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
class UgrLocPlugin_lfc : public LocationPlugin {
protected:

public:

    /**
     * Follow the standard LocationPlugin construction
     * 
     * */
    UgrLocPlugin_lfc(UgrConnector & c, std::vector<std::string> & parms);


    /**
     *  main executor for the plugin    
     **/
    virtual void runsearch(struct worktoken *op, int myidx);

protected:
    std::string base_url;
    gfal2_context_t context;

    void load_configuration(const std::string & prefix);
    int getReplicasFromLFC(const std::string & url, const int myidx,
                           const boost::function<void (UgrFileItem_replica & it)> & inserter, GError** err);
    void insertReplicas(UgrFileItem_replica & it, struct worktoken *op);
};



// ------------------------------------------------------------------------------------
// Plugin-related stuff
// ------------------------------------------------------------------------------------

// The plugin functionality. This function invokes the plugin loader, looking for the
// plugin where to call the hook function
PluginInterface *GetPluginInterfaceClass(char *pluginPath, GetPluginInterfaceArgs);



#endif


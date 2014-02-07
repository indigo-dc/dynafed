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
public:

    /**
     * Follow the standard LocationPlugin construction
     * 
     * */
    UgrLocPlugin_http(UgrConnector & c, std::vector<std::string> & parms);
    virtual ~UgrLocPlugin_http(){}


    /**
     *  main executor for the plugin    
     **/
    virtual void runsearch(struct worktoken *op, int myidx);

    /// With http we cannot do listings, hence we shortcircuit the request

    virtual int do_List(UgrFileInfo *fi, LocationInfoHandler *handler) {
        return 0;
    }
protected:
    bool ssl_check;
    Davix::Uri base_url_endpoint;

    boost::scoped_ptr<Davix::Context> dav_core;
    Davix::DavPosix pos;
    Davix::RequestParams params;
    Davix::RequestParams checker_params;

    virtual void run_Check(int myidx);

    void load_configuration(const std::string & prefix);
    void do_CheckInternal(int myidx, const char* fname);


};







#endif


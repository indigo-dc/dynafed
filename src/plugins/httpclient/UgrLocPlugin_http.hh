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
 * @file   UgrLocPlugin_http.hh
 * @brief  Plugin that talks to any Webdav compatible endpoint
 * @author Devresse Adrien
 * @date   Feb 2012
 */

#include <davix.hpp>
#include <string>
#include "../../LocationPlugin.hh"


#define UGR_HTTP_FLAG_METALINK (0x01)


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


    virtual int run_findNewLocation(const std::string & new_lfn, std::shared_ptr<NewLocationHandler> handler);

    virtual int run_deleteReplica(const std::string &lfn, std::shared_ptr<DeleteReplicaHandler> handler);

    virtual int run_deleteDir(const std::string &lfn, std::shared_ptr<DeleteReplicaHandler> handler);

    virtual int run_mkDirMinusPonSiteFN(const std::string &sitefn, std::shared_ptr<HandlerTraits> handler);

protected:
    int flags;
    Davix::Uri base_url_endpoint;
    Davix::Uri checker_url;

    Davix::Context dav_core;
    Davix::DavPosix pos;
    Davix::RequestParams params;
    Davix::RequestParams checker_params;

    virtual void run_Check(int myidx);

    void load_configuration(const std::string & prefix);
    void do_CheckInternal(int myidx, const char* fname);

    bool concat_http_url_path(const std::string & base_uri, const std::string & path, std::string & canonical);


};







#endif

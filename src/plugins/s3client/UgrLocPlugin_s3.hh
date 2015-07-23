#pragma once
#ifndef LOCATIONPLUGIN_S3_HH
#define LOCATIONPLUGIN_S3_HH

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
 * @file   UgrLocPlugin_s3.hh
 * @brief  Plugin that talks to any S3 compatible endpoint
 * @author Devresse Adrien
 * @date   Feb 2012
 */

#include <davix.hpp>
#include <string>
#include "../../LocationPlugin.hh"
#include "../httpclient/UgrLocPlugin_http.hh"

/**
 *  s3 plugin config parameters
 *
 */

/** 
 * Location Plugin for Ugr, inherit from the LocationPlugin
 *  allow to do basic query to a S3 endpoint
 **/
class UgrLocPlugin_s3 : public UgrLocPlugin_http {
protected:

    virtual void do_Check(int myidx);
public:

    ///
    /// Follow the standard LocationPlugin construction
    ///
    ///
    UgrLocPlugin_s3(UgrConnector & c, std::vector<std::string> & parms);
    virtual ~UgrLocPlugin_s3(){}

    ///
    /// main executor for the plugin
    ///
    virtual void runsearch(struct worktoken *op, int myidx);


    virtual int do_List(UgrFileInfo *fi, LocationInfoHandler *handler);

    virtual int run_findNewLocation(const std::string & new_lfn, std::shared_ptr<NewLocationHandler> handler);


    virtual int run_deleteReplica(const std::string &lfn, std::shared_ptr<DeleteReplicaHandler> handler);

private:
    void configure_S3_parameter(const std::string & str);
    bool concat_s3_url_path(const std::string & base_uri, const std::string & path, std::string & canonical);
};





#endif


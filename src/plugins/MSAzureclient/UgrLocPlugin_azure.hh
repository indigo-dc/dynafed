#pragma once
#ifndef LOCATIONPLUGIN_AZURE_HH
#define LOCATIONPLUGIN_AZURE_HH

/*
 *  Copyright (c) CERN 2015
 *  Author: Fabrizio Furano (CERN IT-SDC)
 * 
 *  Licensed under the Apache License, Version 2.0
 *  See the LICENSE file for further information
 * 
 */


/** 
 * @file   UgrLocPlugin_azure.hh
 * @brief  Plugin that talks to a Microsoft azure endpoint
 * @author Fabrizio Furano
 * @date   Nov 2015
 */

#include <davix.hpp>
#include <string>
#include "../s3client/UgrLocPlugin_s3.hh"


/** 
 * Location Plugin for Ugr, inherit from the LocationPlugin
 *  allow to do basic query to an Azure endpoint
 **/
class UgrLocPlugin_Azure : public UgrLocPlugin_s3 {
public:

    ///
    /// Follow the standard LocationPlugin construction
    ///
    ///
    UgrLocPlugin_Azure(UgrConnector & c, std::vector<std::string> & parms);
    virtual ~UgrLocPlugin_Azure(){}
    
    virtual Davix::Uri signURI(const Davix::RequestParams & params, const std::string & method, const Davix::Uri & url, Davix::HeaderVec headers, const time_t expirationTime);
    
    virtual int run_mkDirMinusPonSiteFN(const std::string &sitefn, std::shared_ptr<HandlerTraits> handler);
    
private:

    void configure_Azure_parameters(const std::string & str);
    virtual bool concat_url_path(const std::string & base_uri, const std::string & path, std::string & canonical);
};





#endif


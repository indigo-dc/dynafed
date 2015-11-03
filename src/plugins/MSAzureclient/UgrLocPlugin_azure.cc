

/*
 *  Copyright (c) CERN 2015
 *  Author: Fabrizio Furano (CERN IT-SDC)
 * 
 *  Licensed under the Apache License, Version 2.0
 *  See the LICENSE file for further information
 * 
 */


/** 
 * @file   UgrLocPlugin_Azure.cc
 * @brief  Plugin that talks to a Microsoft Azure endpoint
 * @author Fabrizio Furano
 * @date   Not 2015
 */


#include "UgrLocPlugin_azure.hh"
#include "../../PluginLoader.hh"
#include "../../ExtCacheHandler.hh"
#include "../utils/HttpPluginUtils.hh"
#include <time.h>
#include "libs/time_utils.h"

using namespace boost;
using namespace std;


// ------------------------------------------------------------------------------------
// Plugin-related stuff
// ------------------------------------------------------------------------------------

UgrLocPlugin_Azure::UgrLocPlugin_Azure(UgrConnector & c, std::vector<std::string> & parms) :
    UgrLocPlugin_s3(c, parms) {
    Info(UgrLogger::Lvl1, "UgrLocPlugin_Azure", "UgrLocPlugin_Azure: Starting Azure access");
    
    configure_Azure_parameters(getConfigPrefix() + name);
    
    params.setProtocol(Davix::RequestProtocol::Azure);
    checker_params.setProtocol(Davix::RequestProtocol::Azure);
}



void UgrLocPlugin_Azure::configure_Azure_parameters(const std::string & prefix){

    
    signature_validity = (time_t)pluginGetParam<long>(prefix, "azure.signaturevalidity", 3600);
    Info(UgrLogger::Lvl1, name, " Azure signature validity is " << signature_validity);
    

    params.setAzureKey( pluginGetParam<std::string>(prefix, "azure.key") );
    checker_params.setAzureKey( pluginGetParam<std::string>(prefix, "azure.key") );

}

// concat URI + path, if it correspond to a bucket name, return false -> error
bool UgrLocPlugin_Azure::concat_url_path(const std::string & base_uri, const std::string & path, std::string & canonical){
    static const char * fname = "UgrLocPlugin_azure::concat_azure_url_path";
    // Azure does not support //
    auto it = path.begin();
    while(*it == '/' && it < path.end())
        it++;

    if(it == path.end()){
        LocPluginLogInfoThr(UgrLogger::Lvl3, fname, "bucket name, ignore " << path);
        return false;
    }

    canonical = base_uri;
    canonical.append("/");
    canonical.append(it, path.end());
    return true;
}



Davix::Uri UgrLocPlugin_Azure::signURI(const Davix::RequestParams & params, const std::string & method, const Davix::Uri & url, Davix::HeaderVec headers, const time_t expirationTime) {

    return Davix::Azure::signURI(params.getAzureKey(), method, url, expirationTime);
}






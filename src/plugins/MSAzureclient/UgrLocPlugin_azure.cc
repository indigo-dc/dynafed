

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
    checker_url = base_url_endpoint;
    checker_url.ensureTrailingSlash();
}



void UgrLocPlugin_Azure::configure_Azure_parameters(const std::string & prefix){


    signature_validity = (time_t)pluginGetParam<long>(prefix, "azure.signaturevalidity", 3600);
    Info(UgrLogger::Lvl1, name, " Azure signature validity is " << signature_validity);

    // Now abort everything if the signature validity clashes with the settings of the cache
    // This thing is very important, hence no cache parameter means bad  
    time_t ttl = (time_t)UgrCFG->GetLong("extcache.memcached.ttl", 1000000);
    if ( signature_validity < ttl-60 ) {
      Error(name, " The given signature validity of " << signature_validity <<
      " is not compatible with the expiration time of the external cache extcache.memcached.ttl (" << ttl << ")");
      throw 1;
    }
    
    ttl = (time_t)UgrCFG->GetLong("infohandler.itemmaxttl", 1000000);
    if ( signature_validity < ttl-60 ) {
      Error(name, " The given signature validity of " << signature_validity <<
      " is not compatible with the expiration time of the internal cache infohandler.itemmaxttl (" << ttl << ")");
      throw 1;
    }
    
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



int UgrLocPlugin_Azure::run_mkDirMinusPonSiteFN(const std::string &sitefn, std::shared_ptr<HandlerTraits> handler){
  const char *fname = "UgrLocPlugin_Azure::run_mkDirMinusPonSiteFN";
  
  
  LocPluginLogInfoThr(UgrLogger::Lvl4, fname, "Of course Azure does not need to prepare parent directories for " << sitefn);
  
  
  return 0;
}

Davix::Uri UgrLocPlugin_Azure::signURI(const Davix::RequestParams & params, const std::string & method, const Davix::Uri & url, Davix::HeaderVec headers, const time_t expirationTime) {

    return Davix::Azure::signURI(params.getAzureKey(), method, url, expirationTime);
}

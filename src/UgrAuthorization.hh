/*
 *  Copyright (c) CERN 2015
 *
 *
 *  Licensed under the Apache License, Version 2.0
 *  See the LICENSE file for further information
 * 
 */


/** @file   UgrAuthorization.hh
 * @brief  Base class that gives the authorization functionalities of Ugr. The default rules are a simple rule-based scheme.
 * @author Fabrizio Furano
 * @date   Nov 2015
 */


#include "PluginInterface.hh"




class UgrAuthorizationPlugin : public PluginInterface {
public:
    UgrAuthorizationPlugin( UgrConnector & c, std::vector<std::string> & parms);   
    virtual ~UgrAuthorizationPlugin();
    
    
    virtual bool isallowed(const char *fname,
                           const std::string &clientName,
                           const std::string &remoteAddress,
                           const std::vector<std::string> &fqans,
                           const std::vector< std::pair<std::string, std::string> > &keys,
                           const char *reqresource, const char reqmode);
};







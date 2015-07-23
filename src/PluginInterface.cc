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

#include "PluginInterface.hh"
#include "UgrConnector.hh"

PluginInterface::PluginInterface(UgrConnector & c, std::vector<std::string> & parms) :
    _c(c),
    myID(std::numeric_limits<int>::max()),
    _parms(parms)
{
    UgrLogger::set(&(getConn().getLogger()));
}


const std::string & PluginInterface::getPluginName() const{
    static const std::string generic_name("PluginInterface");
    return generic_name;
}


FilterPlugin::FilterPlugin(UgrConnector &c, std::vector<std::string> &parms) :
    PluginInterface(c, parms)
{

}


void FilterPlugin::hookNewReplica(UgrFileItem_replica &replica){

}

int FilterPlugin::applyFilterOnReplicaList(UgrReplicaVec&replica, const UgrClientInfo &cli_info){
    return 0;
}

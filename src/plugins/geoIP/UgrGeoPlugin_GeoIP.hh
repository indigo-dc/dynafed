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




/** @file   GeoPlugin.hh
 * @brief  Base class for an UGR plugin that assigns GPS coordinates to FileItems based on the server that they specify
 * @author Fabrizio Furano
 * @date   Mar 2012
 */
#ifndef GEOIPPLUGIN_HH
#define GEOIPPLUGIN_HH

#include <UgrConnector.hh>
#include "../PluginInterface.hh"
#include "GeoIP.h"

/** GeoPlugin_GeoIP
 * Plugin which parses a replica name and figures out where the server is.
 * Any implementation is supposed to be thread-safe, possibly without serializations.
 *
 */
class UgrGeoPlugin_GeoIP : public FilterPlugin{
protected:
    GeoIP *gi;
    float fuzz;
    unsigned seed;
public:

    UgrGeoPlugin_GeoIP(UgrConnector & c, std::vector<std::string> & parms);
    virtual ~UgrGeoPlugin_GeoIP();


    virtual void hookNewReplica(UgrFileItem_replica &replica);

    virtual int applyFilterOnReplicaList(UgrReplicaVec& replica, const UgrClientInfo & cli_info);

protected:
    /// Perform initialization
    int init(std::vector<std::string> &parms);

    /// Sets, wherever needed the geo information in the replica
    void setReplicaLocation(UgrFileItem_replica &it);

    /// Gets latitude and longitude of a client
    void getAddrLocation(const std::string &clientip, float &ltt, float &lng);
};




#endif


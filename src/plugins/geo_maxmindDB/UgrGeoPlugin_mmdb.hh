/*
 *  Copyright (c) CERN 2017
 *
 *  Licensed under the Apache License, Version 2.0
 *  See the LICENSE file for further information
 * 
 */




/** @file  UgrGeoPlugin_mmdb.hh
 * @brief  An UGR filter plugin that assigns geographical coordinates to FileItems based on the server that they specify
 * @author Fabrizio Furano
 * @date   Apr 2017
 */
#ifndef MMDBLUGIN_HH
#define MMDBLUGIN_HH

#include <UgrConnector.hh>
#include "../PluginInterface.hh"
#include "maxminddb.h"

/** UgrGeoPlugin_mmdb
 * Plugin which parses a replica name and figures out where the server is.
 * This implementation uses the MaxMindDB API, which obsoletes GeoIP
 */
class UgrGeoPlugin_mmdb : public FilterPlugin{
protected:
    MMDB_s mmdb;
    bool mmdb_ok;
    float fuzz;
    unsigned int seed;
public:

  UgrGeoPlugin_mmdb(UgrConnector & c, std::vector<std::string> & parms);
  virtual ~UgrGeoPlugin_mmdb();


    virtual void hookNewReplica(UgrFileItem_replica &replica);

    virtual int applyFilterOnReplicaList(UgrReplicaVec& replica, const UgrClientInfo & cli_info);

protected:
    /// Ugly func to shuffle replica entries
    void ugrgeorandom_shuffle( UgrReplicaVec::iterator first,
                                               UgrReplicaVec::iterator last );
    /// Perform initialization
    int init(std::vector<std::string> &parms);

    /// Sets, wherever needed the geo information in the replica
    void setReplicaLocation(UgrFileItem_replica &it);

    /// Gets latitude and longitude of a client
    void getAddrLocation(const std::string &clientip, float &ltt, float &lng);
};




#endif


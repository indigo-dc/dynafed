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
public:

    UgrGeoPlugin_GeoIP(UgrConnector & c, std::vector<std::string> & parms);
    virtual ~UgrGeoPlugin_GeoIP();

    virtual int filterReplicaList(std::deque<UgrFileItem_replica> & replica, const UgrClientInfo & cli_info);

protected:
    /// Perform initialization
    int init(std::vector<std::string> &parms);

    /// Sets, wherever needed the geo information in the replica
    void setReplicaLocation(UgrFileItem_replica &it);

    /// Gets latitude and longitude of a client
    void getAddrLocation(const std::string &clientip, float &ltt, float &lng);
};




#endif


/** @file   GeoPlugin.hh
 * @brief  Base class for an UGR plugin that assigns GPS coordinates to FileItems based on the server that they specify
 * @author Fabrizio Furano
 * @date   Mar 2012
 */
#ifndef GEOIPPLUGIN_HH
#define GEOIPPLUGIN_HH


#include "../../GeoPlugin.hh"
#include "GeoIP.h"

/** GeoPlugin_GeoIP
 * Plugin which parses a replica name and figures out where the server is.
 * Any implementation is supposed to be thread-safe, possibly without serializations.
 *
 */
class UgrGeoPlugin_GeoIP {
protected:
    GeoIP *gi;
public:

    UgrGeoPlugin_GeoIP(SimpleDebug *dbginstance, Config *cfginstance, std::vector<std::string> &parms);
    virtual ~UgrGeoPlugin_GeoIP();

    /// Perform initialization
    virtual int init(std::vector<std::string> &parms);

    /// Sets, wherever needed the geo information in the replica
    virtual void setReplicaLocation(UgrFileItem &it);

    /// Gets latitude and longitude of a client
    virtual void getAddrLocation(std::string &clientip, float &ltt, float &lng);
};




#endif


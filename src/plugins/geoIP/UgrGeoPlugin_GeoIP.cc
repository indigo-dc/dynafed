/** @file   GeoPlugin.hh
 * @brief  Base class for an UGR plugin that assigns GPS coordinates to FileItems based on the server that they specify
 * @author Fabrizio Furano
 * @date   Mar 2012
 */


#include "UgrGeoPlugin_GeoIP.hh"

#include "GeoIPCity.h"

using namespace std;

UgrGeoPlugin_GeoIP::UgrGeoPlugin_GeoIP(SimpleDebug *dbginstance, Config *cfginstance, std::vector<std::string> &parms) {
    SimpleDebug::Instance()->Set(dbginstance);
    CFG->Set(cfginstance);

    const char *fname = "UgrGeoPlugin::UgrGeoPlugin_GeoIP";
    Info(SimpleDebug::kLOW, fname, "Creating instance.");

    gi = 0;
};

UgrGeoPlugin_GeoIP::~UgrGeoPlugin_GeoIP() {

}; 

// Init the geoIP stuff, try to get the best info that is available
// The format for parms is
// /usr/lib64/libugrgeoplugin_geoip.so <name> <path to the GeoIP db>
int UgrGeoPlugin_GeoIP::init(std::vector<std::string> &parms) {
    const char *fname = "UgrGeoPlugin::Init";

    if (parms.size() < 3) {
        Error(fname, "Too few parameters.");
        return 1;
    }

    gi = GeoIP_open(parms[2].c_str(), GEOIP_INDEX_CACHE);
    if (!gi) {
        Error(fname, "Error opening GeoIP database: " << parms[2].c_str());
        return 2;
    }


    return 0;
};

/// Sets, wherever needed the geo information in the replica
void UgrGeoPlugin_GeoIP::setReplicaLocation(UgrFileItem &it) {
    const char *fname = "UgrGeoPlugin::setReplicasLocation";

    // Get the server name from the name field
    Info(SimpleDebug::kHIGHEST, fname, "Got name: " << it.name);
    // skip delimiters at beginning.
    string::size_type lastPos = it.name.find_first_not_of(" :/\\", 0);
    if (lastPos == string::npos) return;

    // find first ":".
    string::size_type pos = it.name.find_first_of(":", lastPos);
    if (pos == string::npos) return;

    // skip slashes
    lastPos = it.name.find_first_not_of(":/", pos);
    if (lastPos == string::npos) return;

    // find slash after hostname
    pos = it.name.find_first_of("/\\", lastPos);
    if (pos == string::npos) return;

    string srv = it.name.substr(lastPos, pos-lastPos);
    Info(SimpleDebug::kHIGHEST, fname, "pos:" << pos << " lastpos: " << lastPos);
    Info(SimpleDebug::kHIGHEST, fname, "Got server: " << srv);

    GeoIPRecord *gir = GeoIP_record_by_name(gi, (const char *)srv.c_str());

    if (gir == NULL) {
        Error(fname, "GeoIP_record_by_name failed: " << srv.c_str());
        return;
    }

    Info(SimpleDebug::kHIGH, fname, "Got geo info: " << srv << " " << gir->country_name << " " << gir->city << " " << gir->latitude << " " << gir->longitude);
    it.latitude = gir->latitude;
    it.longitude = gir->longitude;

    if (gir->city)
        it.location = gir->city;
    else if (gir->country_name)
        it.location = gir->country_name;

    GeoIPRecord_delete(gir);
    return;
};



void UgrGeoPlugin_GeoIP::getAddrLocation(std::string &clientip, float &ltt, float &lng) {
    const char *fname = "UgrGeoPlugin::getAddrLocation";
    GeoIPRecord *gir = GeoIP_record_by_name(gi, (const char *)clientip.c_str());

    if (gir == NULL) {
        Error(fname, "GeoIP_record_by_name failed: " << clientip.c_str());
        ltt = lng = 0.0;
        return;
    }

    ltt = gir->latitude;
    lng = gir->longitude;

    GeoIPRecord_delete(gir);
}



// ------------------------------------------------------------------------------------
// Plugin-related stuff
// ------------------------------------------------------------------------------------



/// The plugin hook function. GetLocationPluginClass must be given the name of this function
/// for the plugin to be loaded
extern "C" GeoPlugin *GetGeoPlugin(GetGeoPluginArgs) {
    return (GeoPlugin *)new UgrGeoPlugin_GeoIP(dbginstance, cfginstance, parms);
}


/** @file   GeoPlugin.hh
 * @brief  Base class for an UGR plugin that assigns GPS coordinates to FileItems based on the server that they specify
 * @author Fabrizio Furano
 * @date   Mar 2012
 */


#include "UgrGeoPlugin_GeoIP.hh"
#include "GeoIPCity.h"


GeoPlugin_GeoIP::GeoPlugin_GeoIP(SimpleDebug *dbginstance, Config *cfginstance, std::vector<std::string> &parms) {
    SimpleDebug::Instance()->Set(dbginstance);
    CFG->Set(cfginstance);

    const char *fname = "GeoPlugin::GeoPlugin_GeoIP";
    Info(SimpleDebug::kLOW, fname, "Creating instance.");
    
};

GeoPlugin_GeoIP::~GeoPlugin_GeoIP() {

}; 

int GeoPlugin_GeoIP::init() {
    return 0;
};

/// Sets, wherever needed the geo information in the replica
void GeoPlugin_GeoIP::setLocation(UgrFileItem &it) {
    it.latitude = 0;
    it.longitude = 0;
    it.location = "";
};



// ------------------------------------------------------------------------------------
// Plugin-related stuff
// ------------------------------------------------------------------------------------



/// The plugin hook function. GetLocationPluginClass must be given the name of this function
/// for the plugin to be loaded
extern "C" GeoPlugin *GetGeoPlugin(GetGeoPluginArgs) {
    return (GeoPlugin *)new GeoPlugin_GeoIP(dbginstance, cfginstance, parms);
}


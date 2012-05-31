/** @file   GeoPlugin.hh
 * @brief  Base class for an UGR plugin that assigns GPS coordinates to FileItems based on the server that they specify
 * @author Fabrizio Furano
 * @date   Mar 2012
 */
#ifndef GEOIPPLUGIN_HH
#define GEOIPPLUGIN_HH


#include "../../GeoPlugin.hh"

/** GeoPlugin_GeoIP
 * Plugin which parses a replica name and figures out where the server is.
 * Any implementation is supposed to be thread-safe, possibly without serializations.
 *
 */
class GeoPlugin_GeoIP {

protected:

public:

   GeoPlugin_GeoIP(SimpleDebug *dbginstance, Config *cfginstance, std::vector<std::string> &parms);
   virtual ~GeoPlugin_GeoIP();

   /// Perform initialization
   virtual int init();

   /// Sets, wherever needed the geo information in the replica
   virtual void setLocation(UgrFileItem &it);
};



// ------------------------------------------------------------------------------------
// Plugin-related stuff
// ------------------------------------------------------------------------------------

/// The set of args that have to be passed to the plugin hook function
#define GetGeoPluginArgs SimpleDebug *dbginstance, Config *cfginstance, std::vector<std::string> &parms

/// The plugin functionality. This function invokes the plugin loader, looking for the
/// plugin where to call the hook function
GeoPlugin *GetGeoPluginClass(char *pluginPath, GetGeoPluginArgs);



#endif
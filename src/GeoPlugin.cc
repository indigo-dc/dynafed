/** @file   GeoPlugin.hh
 * @brief  Base class for an UGR plugin that assigns GPS coordinates to FileItems based on the server that they specify
 * @author Fabrizio Furano
 * @date   Mar 2012
 */


#include "GeoPlugin.hh"
#include "PluginLoader.hh"


GeoPlugin::GeoPlugin(SimpleDebug *dbginstance, Config *cfginstance, std::vector<std::string> &parms) {
    SimpleDebug::Instance()->Set(dbginstance);
    CFG->Set(cfginstance);

    const char *fname = "GeoPlugin::GeoPlugin";
    Info(SimpleDebug::kLOW, fname, "Creating instance.");
    
};

GeoPlugin::~GeoPlugin() {

};

int GeoPlugin::init(std::vector<std::string> &parms) {
    return 0;
};

/// Sets, wherever needed the geo information in the replica
void GeoPlugin::setReplicaLocation(UgrFileItem_replica &it) {
    it.latitude = 0;
    it.longitude = 0;
    it.location = "";
};


void GeoPlugin::getAddrLocation(std::string &clientip, float &ltt, float &lng) {

}






// ------------------------------------------------------------------------------------
// Plugin-related stuff
// ------------------------------------------------------------------------------------



// The plugin functionality. This function invokes the plugin loader, looking for the
// plugin where to call the hook function
GeoPlugin *GetGeoPluginClass(char *pluginPath, GetGeoPluginArgs) {
    const char *fname = "GetGeoPluginClass_local";
    PluginLoader *myLib = 0;
    GeoPlugin * (*ep)(GetGeoPluginArgs);

    // If we have no plugin path then return NULL
    if (!pluginPath || !strlen(pluginPath)) {
        Info(SimpleDebug::kMEDIUM, fname, "No plugin to load.");
        return NULL;
    }

    // Create a plugin object (we will throw this away without deletion because
    // the library must stay open but we never want to reference it again).
    if (!myLib) {
        Info(SimpleDebug::kMEDIUM, fname, "Loading plugin " << pluginPath);
        if (!(myLib = new PluginLoader(pluginPath))) {
            Info(SimpleDebug::kLOW, fname, "Failed loading plugin " << pluginPath);
            return NULL;
        }
    } else {
        Info(SimpleDebug::kMEDIUM, fname, "Plugin " << pluginPath << "already loaded.");
    }

    // Now get the entry point of the object creator
    Info(SimpleDebug::kMEDIUM, fname, "Getting entry point for plugin " << pluginPath);
    ep = (GeoPlugin * (*)(GetGeoPluginArgs))(myLib->getPlugin("GetGeoPlugin"));
    if (!ep) {
        Info(SimpleDebug::kLOW, fname, "Could not get entry point for plugin " << pluginPath);
        return NULL;
    }

    // Get the Object now
    Info(SimpleDebug::kMEDIUM, fname, "Getting class instance for plugin " << pluginPath);
    GeoPlugin *c = ep(dbginstance, cfginstance, parms);
    if (!c) {
        Info(SimpleDebug::kLOW, fname, "Could not get class instance for plugin " << pluginPath);
        return NULL;
    }

    if (c->init(parms)) {
       Error(fname, "Error initializing Geo plugin " << pluginPath);
       return NULL;
    }
    return c;

}


/// The plugin hook function. GetLocationPluginClass must be given the name of this function
/// for the plugin to be loaded
extern "C" GeoPlugin *GetGeoPlugin(GetGeoPluginArgs) {
    return (GeoPlugin *)new GeoPlugin(dbginstance, cfginstance, parms);
}


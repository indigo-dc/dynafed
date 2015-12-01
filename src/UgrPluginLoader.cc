
/*
 *  Copyright (c) CERN 2015
 *
 *  Licensed under the Apache License, Version 2.0
 *  See the LICENSE file for further information
 * 
 */

#include "UgrPluginLoader.hh"
#include "PluginLoader.hh"

// The plugin functionality. This function invokes the plugin loader, looking for the
// plugin where to call the hook function

PluginInterface *GetPluginInterfaceClass(char *pluginPath, GetPluginInterfaceArgs) {
    const char *fname = "GetPluginInterfaceClass_local";
    PluginLoader *myLib = 0;
    PluginInterface * (*ep)(GetPluginInterfaceArgs);

    // If we have no plugin path then return NULL
    if (!pluginPath || !strlen(pluginPath)) {
        Info(UgrLogger::Lvl2, fname, "No plugin to load.");
        return NULL;
    }

    // Create a plugin object (we will throw this away without deletion because
    // the library must stay open but we never want to reference it again).
    if (!myLib) {
        Info(UgrLogger::Lvl2, fname, "Loading plugin " << pluginPath);
        if (!(myLib = new PluginLoader(pluginPath))) {
            Info(UgrLogger::Lvl1, fname, "Failed loading plugin " << pluginPath);
            return NULL;
        }
    } else {
        Info(UgrLogger::Lvl2, fname, "Plugin " << pluginPath << "already loaded.");
    }

    // Now get the entry point of the object creator
    Info(UgrLogger::Lvl2, fname, "Getting entry point for plugin " << pluginPath);
    ep = (PluginInterface * (*)(GetPluginInterfaceArgs))(myLib->getPlugin("GetPluginInterface"));
    if (!ep) {
        Info(UgrLogger::Lvl1, fname, "Could not get entry point for plugin " << pluginPath);
        return NULL;
    }

    // Get the Object now
    Info(UgrLogger::Lvl2, fname, "Getting class instance for plugin " << pluginPath);
    PluginInterface *p = ep(c, parms);
    if (!p)
        Info(UgrLogger::Lvl1, fname, "Could not get class instance for plugin " << pluginPath);
    return p;

}





// ------------------------------------------------------------------------------------
// Plugin-related stuff
// ------------------------------------------------------------------------------------


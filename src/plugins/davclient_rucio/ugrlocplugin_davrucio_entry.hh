#ifndef UGRLOCPLUGIN_DAV_ENTRY_HH
#define UGRLOCPLUGIN_DAV_ENTRY_HH

/**
 *  Copyright (c) CERN 2014
 *
 *  Licensed under the Apache License, Version 2.0
 */


#include "UgrLocPlugin_davrucio.hh"

// ------------------------------------------------------------------------------------
// Plugin-related stuff
// ------------------------------------------------------------------------------------

// The plugin functionality. This function invokes the plugin loader, looking for the
// plugin where to call the hook function
PluginInterface *GetPluginInterfaceClass(char *pluginPath, GetPluginInterfaceArgs);



#endif // UGRLOCPLUGIN_DAV_ENTRY_HH

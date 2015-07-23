#ifndef UGRLOCPLUGIN_DAV_ENTRY_HH
#define UGRLOCPLUGIN_DAV_ENTRY_HH

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



#include "UgrLocPlugin_dav.hh"

// ------------------------------------------------------------------------------------
// Plugin-related stuff
// ------------------------------------------------------------------------------------

// The plugin functionality. This function invokes the plugin loader, looking for the
// plugin where to call the hook function
PluginInterface *GetPluginInterfaceClass(char *pluginPath, GetPluginInterfaceArgs);



#endif // UGRLOCPLUGIN_DAV_ENTRY_HH

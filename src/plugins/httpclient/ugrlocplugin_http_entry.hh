#ifndef UGRLOCPLUGIN_HTTP_ENTRY_H
#define UGRLOCPLUGIN_HTTP_ENTRY_H

#include "UgrLocPlugin_http.hh"

// ------------------------------------------------------------------------------------
// Plugin-related stuff
// ------------------------------------------------------------------------------------

// The plugin functionality. This function invokes the plugin loader, looking for the
// plugin where to call the hook function
LocationPlugin *GetLocationPluginClass(char *pluginPath, GetLocationPluginArgs);

#endif // UGRLOCPLUGIN_HTTP_ENTRY_H

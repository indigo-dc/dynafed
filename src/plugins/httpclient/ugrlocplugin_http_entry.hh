#ifndef UGRLOCPLUGIN_HTTP_ENTRY_H
#define UGRLOCPLUGIN_HTTP_ENTRY_H

#include "UgrLocPlugin_http.hh"

// ------------------------------------------------------------------------------------
// Plugin-related stuff
// ------------------------------------------------------------------------------------

// The plugin functionality. This function invokes the plugin loader, looking for the
// plugin where to call the hook function
PluginInterface *GetPluginInterfaceClass(char *pluginPath, GetPluginInterfaceArgs);

#endif // UGRLOCPLUGIN_HTTP_ENTRY_H

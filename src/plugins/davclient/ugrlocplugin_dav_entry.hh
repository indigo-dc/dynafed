#ifndef UGRLOCPLUGIN_DAV_ENTRY_HH
#define UGRLOCPLUGIN_DAV_ENTRY_HH


#include "UgrLocPlugin_dav.hh"

// ------------------------------------------------------------------------------------
// Plugin-related stuff
// ------------------------------------------------------------------------------------

// The plugin functionality. This function invokes the plugin loader, looking for the
// plugin where to call the hook function
PluginInterface *GetPluginInterfaceClass(char *pluginPath, GetPluginInterfaceArgs);



#endif // UGRLOCPLUGIN_DAV_ENTRY_HH

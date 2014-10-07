#ifndef UGRLOCPLUGIN_S3_ENTRY_HH
#define UGRLOCPLUGIN_S3_ENTRY_HH


#include "UgrLocPlugin_s3.hh"

// ------------------------------------------------------------------------------------
// Plugin-related stuff
// ------------------------------------------------------------------------------------

// The plugin functionality. This function invokes the plugin loader, looking for the
// plugin where to call the hook function
PluginInterface *GetPluginInterfaceClass(char *pluginPath, GetPluginInterfaceArgs);



#endif // UGRLOCPLUGIN_S3_ENTRY_HH

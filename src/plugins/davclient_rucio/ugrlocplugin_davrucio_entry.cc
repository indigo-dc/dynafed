#include "ugrlocplugin_davrucio_entry.hh"

/**
 *  Copyright (c) CERN 2014
 *
 *  Licensed under the Apache License, Version 2.0
 */



// The hook function. GetPluginInterfaceClass must be given the name of this function
// for the plugin to be loaded

/**
 * Hook for the dav plugin Location plugin
 * */
extern "C" PluginInterface *GetPluginInterface(GetPluginInterfaceArgs) {
    davix_set_log_level(DAVIX_LOG_WARNING);
    return (PluginInterface*)new UgrLocPlugin_davrucio(c, parms);
}

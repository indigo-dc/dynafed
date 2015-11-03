#include "ugrlocplugin_azure_entry.hh"

/*
 *  Copyright (c) CERN 2015
 *  Author: Fabrizio Furano (CERN IT-SDC)
 *
 *
 *  Licensed under the Apache License, Version 2.0
 *  See the LICENSE file for further information
 * 
 */


// The hook function. GetPluginInterfaceClass must be given the name of this function
// for the plugin to be loaded

/**
 * Hook for the azure plugin Location plugin
 * */
extern "C" PluginInterface *GetPluginInterface(GetPluginInterfaceArgs) {
    davix_set_log_level(DAVIX_LOG_WARNING);
    return (PluginInterface*)new UgrLocPlugin_Azure(c, parms);
}

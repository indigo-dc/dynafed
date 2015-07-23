#include "ugrlocplugin_dav_entry.hh"

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


// The hook function. GetPluginInterfaceClass must be given the name of this function
// for the plugin to be loaded

/**
 * Hook for the dav plugin Location plugin
 * */
extern "C" PluginInterface *GetPluginInterface(GetPluginInterfaceArgs) {
    davix_set_log_level(DAVIX_LOG_WARNING);
    return (PluginInterface*)new UgrLocPlugin_dav(c, parms);
}

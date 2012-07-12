#pragma once
#ifndef PLUGINENDPOINTSTATUSMANAGER_HH
#define PLUGINENDPOINTSTATUSMANAGER_HH
/**
 * @file   PluginEndpointStatusManager.hh
 * @brief  Convenience logic and function for endpoint status management ( online / offline )
 * @author Devresse Adrien
 */

#include "LocationPlugin.hh"
#include "UgrConnector.hh"

///
/// return true if a plugin is available for common operation 
///
bool checkpluginAvailability(LocationPlugin * plugin, UgrFileInfo *fi);

#endif

/**
 * @file   PluginEndpointStatusManager.cc
 * @brief  Convenience logic and function for endpoint status management ( online / offline )
 * @author Devresse Adrien
 */

#include "PluginEndpointStatusManager.hh"

bool checkpluginAvailability(LocationPlugin * plug, UgrFileInfo *fi){
	PluginEndpointStatus status;
	plug->check_availability(&status, fi);
	if(status.state != PLUGIN_ENDPOINT_ONLINE){
		Info(SimpleDebug::kLOW, "UgrConnector::checkpluginAvailability", "plugin " << plug->get_Name() << " is not available " 
		 << (int) status.state << " error: " << status.explanation);
		return false;
	}
	return true;
}

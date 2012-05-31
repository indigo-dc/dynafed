/** 
 * @file   UgrLocPlugin_dav.hh
 * @brief  Plugin that talks to any Webdav compatible endpoint
 * @author Devresse Adrien
 * @date   Feb 2012
 */

#ifndef LOCATIONPLUGIN_SIMPLEHTTP_HH
#define LOCATIONPLUGIN_SIMPLEHTTP_HH



#include "../../LocationPlugin.hh"




/** 
 * Location Plugin for Ugr, inherit from the LocationPlugin
 *  allow to do basic query to a webdav endpoint
 **/  
class UgrLocPlugin_dav : public LocationPlugin {
protected:


public:

	/**
	 * Follow the standard LocationPlugin construction
	 * 
	 * */
    UgrLocPlugin_dav(SimpleDebug *dbginstance, Config *cfginstance, std::vector<std::string> &parms) :
    LocationPlugin(dbginstance, cfginstance, parms), dav_core(Davix::session_create()) {

        Info(SimpleDebug::kLOW, "UgrLocPlugin_dav", "Creating instance named " << name);
		// try to get config
		if(parms.size() > 3){
	        Info(SimpleDebug::kLOW, "UgrLocPlugin_dav", "Try to bind UgrLocPlugin_dav with " << parms[3]);		
			base_url= parms[3];
		}else{
	        Info(SimpleDebug::kLOW, "UgrLocPlugin_dav", "No correct parameter for this Plugin : Unable to load properly ");		
		}
    };


    /**
     *     
     **/
     virtual void runsearch(struct worktoken *op, int myidx);
protected:
	std::string base_url;
	boost::scoped_ptr<Davix::Composition> dav_core;
};



// ------------------------------------------------------------------------------------
// Plugin-related stuff
// ------------------------------------------------------------------------------------

// The plugin functionality. This function invokes the plugin loader, looking for the
// plugin where to call the hook function
LocationPlugin *GetLocationPluginClass(char *pluginPath, GetLocationPluginArgs);



#endif


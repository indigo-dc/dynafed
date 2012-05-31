/** 
 * @file   UgrLocPlugin_dav.cc
 * @brief  Plugin that talks to any Webdav compatible endpoint
 * @author Devresse Adrien
 * @date   Feb 2012
 */
 
 
#include "UgrLocPlugin_dav.hh"
#include "../../PluginLoader.hh"
#include <time.h>


using namespace boost;
using namespace std;


// ------------------------------------------------------------------------------------
// Plugin-related stuff
// ------------------------------------------------------------------------------------


// The hook function. GetLocationPluginClass must be given the name of this function
// for the plugin to be loaded
extern "C" LocationPlugin *GetLocationPlugin(GetLocationPluginArgs) {
    return (LocationPlugin *)new UgrLocPlugin_dav(dbginstance, cfginstance, parms);
}


void UgrLocPlugin_dav::runsearch(struct worktoken *op, int myidx){
	LocationPlugin::runsearch(op, myidx);
}

/** @file   UgrLocPlugin_simpleHTTP.cc
 * @brief  Plugin that talks to an HTTP server, simple version
 * @author Fabrizio Furano
 * @date   Jan 2012
 */
#include "UgrLocPlugin_simpleHTTP.hh"
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
    return (LocationPlugin *)new UgrLocPlugin_simpleHTTP(dbginstance, cfginstance, parms);
}

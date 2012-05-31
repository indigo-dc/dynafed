#ifndef LOCATIONPLUGIN_SIMPLEHTTP_HH
#define LOCATIONPLUGIN_SIMPLEHTTP_HH

/* LocationPlugin_simpleHTTP
 * Plugin that talks to an HTTP server, simple version
 *
 *
 * by Fabrizio Furano, CERN, Jan 2012
 */

#include "../../LocationPlugin.hh"




class UgrLocPlugin_simpleHTTP;
void simpleHTTPworker(UgrLocPlugin_simpleHTTP* plugin);



class UgrLocPlugin_simpleHTTP: public LocationPlugin {
protected:


public:

   UgrLocPlugin_legacylfc(SimpleDebug *dbginstance, Config *cfginstance, std::vector<std::string> &parms):
        LocationPlugin(dbginstance, cfginstance, parms) {

        Info(SimpleDebug::kLOW, "UgrLocPlugin_simpleHTTP", "Creating instance named " << name);
            
   };


   // Start the async stat process
   //virtual int do_Stat(UgrFileInfo *fi);

   // Start the async location process
   //virtual int do_Locate(UgrFileInfo *fi);

   // Start the async listing process
   //virtual int do_List(UgrFileInfo *fi);


};



// ------------------------------------------------------------------------------------
// Plugin-related stuff
// ------------------------------------------------------------------------------------

// The plugin functionality. This function invokes the plugin loader, looking for the
// plugin where to call the hook function
LocationPlugin *GetLocationPluginClass(char *pluginPath, GetLocationPluginArgs);



#endif


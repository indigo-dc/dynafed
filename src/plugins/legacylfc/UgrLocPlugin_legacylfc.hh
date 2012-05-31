#ifndef LOCATIONPLUGIN_LEGACYLFC_HH
#define LOCATIONPLUGIN_LEGACYLFC_HH

/* LocationPlugin_legacyLFC
 * Plugin that talks to a legacy LFC, using the LFC client
 *
 *
 * by Fabrizio Furano, CERN, Nov 2011
 */

#include "../../LocationPlugin.hh"

class UgrLocPlugin_legacylfc: public LocationPlugin {
public:

   UgrLocPlugin_legacylfc(SimpleDebug *dbginstance, Config *cfginstance, std::vector<std::string> &parms):
        LocationPlugin(dbginstance, cfginstance, parms) {
            Info(SimpleDebug::kLOW, "UgrLocPlugin_legacylfc", "Created instance named " << name);
   };


   // Start the async stat process
   virtual int do_Stat(UgrFileInfo *fi);
   // Waits max a number of seconds for a locate process to be complete
   virtual int do_waitStat(UgrFileInfo *fi, int tmout=5);

   // Start the async location process
   virtual int do_Locate(UgrFileInfo *fi);
   // Waits max a number of seconds for a locate process to be complete
   virtual int do_waitLocate(UgrFileInfo *fi, int tmout=5);

   // Start the async listing process
   virtual int do_List(UgrFileInfo *fi);
   // Waits max a number of seconds for a list process to be complete
   virtual int do_waitList(UgrFileInfo *fi, int tmout=5);

};



// ------------------------------------------------------------------------------------
// Plugin-related stuff
// ------------------------------------------------------------------------------------

// The plugin functionality. This function invokes the plugin loader, looking for the
// plugin where to call the hook function
LocationPlugin *GetLocationPluginClass(char *pluginPath, GetLocationPluginArgs);



#endif


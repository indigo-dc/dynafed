#ifndef LOCATIONPLUGIN_HH
#define LOCATIONPLUGIN_HH

/* LocationPlugin
 * Base class for a plugin which gathers info about files from some source
 * If not properly subclassed, this acts as a default fake plugin, putting test data
 * as responses
 *
 *
 * by Fabrizio Furano, CERN, Oct 2011
 */

#include "Config.hh"
#include "SimpleDebug.hh"
#include "LocationInfo.hh"

#include <string>
#include <vector>
#include <map>
#include <boost/thread.hpp>

class LocationPlugin {
protected:
    // The name assigned to this plugin from the creation
    char *name;
public:

   LocationPlugin(SimpleDebug *dbginstance, Config *cfginstance, std::vector<std::string> &parms) {
      SimpleDebug::Instance()->Set(dbginstance);
      CFG->Set(cfginstance);

      if (parms.size() > 1)
        name = strdup(parms[1].c_str());
      else name = strdup("Unnamed");
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

// The set of args that have to be passed to the plugin hook function
#define GetLocationPluginArgs SimpleDebug *dbginstance, Config *cfginstance, std::vector<std::string> &parms

// The plugin functionality. This function invokes the plugin loader, looking for the
// plugin where to call the hook function
LocationPlugin *GetLocationPluginClass(char *pluginPath, GetLocationPluginArgs);



#endif


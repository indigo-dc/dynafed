#ifndef LOCATIONPLUGIN_LEGACYLFC_HH
#define LOCATIONPLUGIN_LEGACYLFC_HH

/* LocationPlugin_legacyLFC
 * Plugin that talks to a legacy LFC, using the LFC client
 *
 *
 * by Fabrizio Furano, CERN, Nov 2011
 */

#include "../../LocationPlugin.hh"




class UgrLocPlugin_legacylfc;
void lfcworker(UgrLocPlugin_legacylfc* plugin);



class UgrLocPlugin_legacylfc: public LocationPlugin {
protected:
    // A simple worker queue
    std::deque<UgrFileInfo *> wrkqueue;
    // A mutex for this queue
    boost::mutex qmtx;
    // A condvar to sleep into
    boost::condition_variable qcond;
    // The worker thread
    boost::thread *worker;

    friend void lfcworker(UgrLocPlugin_legacylfc* plugin);

    void runitem(UgrFileInfo *fi);
public:

   UgrLocPlugin_legacylfc(SimpleDebug *dbginstance, Config *cfginstance, std::vector<std::string> &parms):
        LocationPlugin(dbginstance, cfginstance, parms) {

            // This plugin is a fake one, that spawns a thread which populates the result after some time
            worker = new boost::thread(lfcworker, this);
            if (worker) {
                Info(SimpleDebug::kLOW, "UgrLocPlugin_legacylfc", "Created instance named " << name);
            }
            else {
                Error("UgrLocPlugin_legacylfc", "Unable to create worker thread.")
            }
   };


   // Start the async stat process
   virtual int do_Stat(UgrFileInfo *fi);

   // Start the async location process
   virtual int do_Locate(UgrFileInfo *fi);

   // Start the async listing process
   virtual int do_List(UgrFileInfo *fi);


};



// ------------------------------------------------------------------------------------
// Plugin-related stuff
// ------------------------------------------------------------------------------------

// The plugin functionality. This function invokes the plugin loader, looking for the
// plugin where to call the hook function
LocationPlugin *GetLocationPluginClass(char *pluginPath, GetLocationPluginArgs);



#endif


/** @file   UgrLocPlugin_dmlite.hh
 * @brief  Plugin that talks to dmlite
 * @author Fabrizio Furano
 * @date   Feb 2012
 */

#ifndef LOCATIONPLUGIN_DMLITE_HH
#define LOCATIONPLUGIN_DMLITE_HH


#include "../../LocationPlugin.hh"
#include <dmlite/cpp/dmlite.h>
#include <dmlite/cpp/dm_exceptions.h>

/** LocationPlugin_dmlite
 * Plugin that talks to a dmlite catalog
 *
 *
 * by Fabrizio Furano, CERN, Feb 2012
 */
class UgrLocPlugin_dmlite : public LocationPlugin {
protected:

    dmlite::PluginManager *pluginManager;
    dmlite::CatalogFactory *catalogfactory;

    /// Mutex for protecting dmlite
    boost::mutex dmlitemutex;

    std::map<int, dmlite::StackInstance *> simap;
public:

    UgrLocPlugin_dmlite(SimpleDebug *dbginstance, Config *cfginstance, std::vector<std::string> &parms);

    // Start the async stat process
    //virtual int do_Stat(UgrFileInfo *fi);

    // Start the async location process
    //virtual int do_Locate(UgrFileInfo *fi);

    // Start the async listing process
    //virtual int do_List(UgrFileInfo *fi);

    virtual void runsearch(struct worktoken *op, int myidx);
};



// ------------------------------------------------------------------------------------
// Plugin-related stuff
// ------------------------------------------------------------------------------------

// The plugin functionality. This function invokes the plugin loader, looking for the
// plugin where to call the hook function
LocationPlugin *GetLocationPluginClass(char *pluginPath, GetLocationPluginArgs);



#endif


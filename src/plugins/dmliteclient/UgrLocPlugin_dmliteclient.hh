/** @file   UgrLocPlugin_dmlite.hh
 * @brief  Plugin that talks to dmlite
 * @author Fabrizio Furano
 * @date   Feb 2012
 */

#ifndef LOCATIONPLUGIN_DMLITE_HH
#define LOCATIONPLUGIN_DMLITE_HH


#include "../../LocationPlugin.hh"
#include <dmlite/cpp/dmlite.h>
#include <dmlite/cpp/inode.h>

#include <queue>

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

    std::queue<dmlite::StackInstance *> siqueue;
    
    dmlite::StackInstance *GetStackInstance(int myidx, bool cancreate = true);
    void ReleaseStackInstance(dmlite::StackInstance *inst);
    
    virtual void do_Check(int myidx);
public:

    UgrLocPlugin_dmlite(UgrConnector & c, std::vector<std::string> & parms);

    // Start the async stat process
    //virtual int do_Stat(UgrFileInfo *fi);

    // Start the async location process
    //virtual int do_Locate(UgrFileInfo *fi);

    // Start the async listing process
    //virtual int do_List(UgrFileInfo *fi);

    virtual void runsearch(struct worktoken *op, int myidx);
    
    static void takeStat(UgrFileInfo *fi, dmlite::ExtendedStat &st);
};



// ------------------------------------------------------------------------------------
// Plugin-related stuff
// ------------------------------------------------------------------------------------

// The plugin functionality. This function invokes the plugin loader, looking for the
// plugin where to call the hook function
PluginInterface *GetPluginInterfaceClass(char *pluginPath, GetPluginInterfaceArgs);



#endif


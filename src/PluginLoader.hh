#ifndef PLUGINLOADER_HH
#define PLUGINLOADER_HH


/* PluginLoader
 * A class that loads a plugin
 * Taken from XrdSysPlugin by A.Hanushevsky
 *
 *
 * by Fabrizio Furano, CERN, Oct 2010
 */

#include <string.h>

class PluginLoader {
public:

   // Gets a pointer to the function that instantiates the
   //  main class defined in the plugin.
   void *getPlugin(const char *pname, int errok=0);
   
   PluginLoader(const char *path) {
      libPath = strdup(path); libHandle = 0;
   }
   
   ~PluginLoader();
   
private:
   
   const char  *libPath;
   void        *libHandle;
};





#endif




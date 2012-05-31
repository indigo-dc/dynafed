/** @file   PluginLoader.hh
 * @brief  A helper class that loads a plugin
 * @author Fabrizio Furano
 * @date   Oct 2010
 */


#include "PluginLoader.hh"
#include "SimpleDebug.hh"
#include <dlfcn.h>
#include <stdlib.h>

PluginLoader::~PluginLoader()
{
   if (libHandle) dlclose(libHandle);
   if (libPath) free((char *)libPath);
}



// Gets a pointer to the function that instantiates the
//  main class defined in the plugin.
void *PluginLoader::getPlugin(const char *pname, int errok)
{
   void *ep;

// Open the plugin library if not already opened
//
   if (!libHandle && !(libHandle = dlopen(libPath, RTLD_NOW | RTLD_GLOBAL))) {
      Error("PluginLoader::getPlugin", "Unable to open" << libPath << " - dlerror: " << dlerror());
      return 0;
   }

// Get the plugin object creator
//
   if (!(ep = dlsym(libHandle, pname)) && !errok) {
      Error("PluginLoader::getPlugin", "Unable to find " << pname << " in " << pname << " - dlerror: " << dlerror());
      return 0;
   }

// All done
//
   return ep;
}
                                                          

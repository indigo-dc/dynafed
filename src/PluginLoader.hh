/*
 *  Copyright (c) CERN 2013
 *
 *  Copyright (c) Members of the EMI Collaboration. 2011-2013
 *  See  http://www.eu-emi.eu/partners for details on the copyright
 *  holders.
 *
 *  Licensed under the Apache License, Version 2.0
 *  See the LICENSE file for further information
 * 
 */

/** @file   PluginLoader.hh
 * @brief  A helper class that loads a plugin
 * @author Fabrizio Furano
 * @date   Oct 2010
 */


#ifndef PLUGINLOADER_HH
#define PLUGINLOADER_HH




#include <string.h>

/** PluginLoader
 * A class that loads a plugin.
 * A plugin is a shared library that:
 *  - exports a hook function with predetermined parameters
 *  - when called, the hook function returns an instance of the class that implements the plugin functionality
 *
 * Straightforward stuff, taken from XrdSysPlugin by A.Hanushevsky
 *
 */
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




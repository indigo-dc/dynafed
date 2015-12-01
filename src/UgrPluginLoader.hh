
/*
 *  Copyright (c) CERN 2015
 *
 *  Licensed under the Apache License, Version 2.0
 *  See the LICENSE file for further information
 * 
 */

#include "PluginInterface.hh"
#include <boost/filesystem.hpp>




/// The set of args that have to be passed to the plugin hook function
#define GetPluginInterfaceArgs UgrConnector & c, std::vector<std::string> &parms


// The plugin functionality. This function invokes the plugin loader, looking for the
// plugin where to call the hook function
PluginInterface *GetPluginInterfaceClass(char *pluginPath, GetPluginInterfaceArgs);




// ------------------------------------------------------------------------------------
// Plugin-related stuff
// ------------------------------------------------------------------------------------






template<class T>
void ugr_load_plugin(UgrConnector & c,
                       const std::string & fname,
                       const boost::filesystem::path & plugin_dir,
                       const std::string & key_list_config,
                       std::vector<T*> & v_plugin){
    char buf[1024];
    int i = 0;
    v_plugin.clear();
    do {
        CFG->ArrayGetString(key_list_config.c_str(), buf, i);
        if (buf[0]) {
            T* filter = NULL;
            std::vector<std::string> parms = tokenize(buf, " ");
           boost::filesystem::path plugin_path(parms[0].c_str()); // if not abs path -> load from plugin dir
           // Get the entry point for the plugin that implements the product-oriented technicalities of the calls
           // An empty string does not load any plugin, just keeps the default behavior

           if (!plugin_path.has_root_directory()) {
               plugin_path = plugin_dir;
               plugin_path /= parms[0];
           }
            try{

                Info(UgrLogger::Lvl1, fname, "Attempting to load plugin "<< typeid(T).name() << plugin_path.string());
                PluginInterface *prod = static_cast<PluginInterface*>(GetPluginInterfaceClass((char *) plugin_path.string().c_str(),
                        c,
                        parms));
                filter = dynamic_cast<T*>(prod);
            }catch(...){
                // silent exception
                filter = NULL;
            }
            if (filter) {
                filter->setID(v_plugin.size());
                v_plugin.push_back(filter);
            }else{
                Error(fname, "Impossible to load plugin " << plugin_path << " of type " << typeid(T).name() << std::endl;);
                exit(255);
            }
        }
        i++;
    } while (buf[0]);
}


template<class T>
void ugr_unload_plugin(std::vector<T*> & v_plugin){
    for(typename std::vector<T*>::iterator it = v_plugin.begin(); it != v_plugin.end(); ++it){
        delete *it;
    }
}


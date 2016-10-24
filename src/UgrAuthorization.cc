/*
 *  Copyright (c) CERN 2015
 *
 *
 *  Licensed under the Apache License, Version 2.0
 *  See the LICENSE file for further information
 * 
 */


/** @file   UgrAuthorization.hh
 * @brief  Base class that gives the authorization functionalities of Ugr. The default rules are a simple rule-based scheme.
 * @author Fabrizio Furano
 * @date   Nov 2015
 */


#include "UgrAuthorization.hh"
#include <errno.h>


UgrAuthorizationPlugin::UgrAuthorizationPlugin( UgrConnector & c, std::vector<std::string> & parms): PluginInterface(c, parms) {
    
}

UgrAuthorizationPlugin::~UgrAuthorizationPlugin() {};




// Implement a simple authorization scheme
// glb.allowusers[] user /path rwl
// glb.allowgroups[] group /path rwl
//
// where:
//  r = capability of reading information like stat
//  w = capability of writing
//  l = capability of listing
//  d = capability of deleting
bool UgrAuthorizationPlugin::isallowed(const char *fname,
                          const std::string &clientName,
                          const std::string &remoteAddress,
                          const std::vector<std::string> &fqans,
                          const std::vector<std::string> &keys,
                          const char *reqresource, const char reqmode) {
    // Simple authorization 
    // If any of the simple rules matches then we let the request pass
    int i;
    bool haddirectives = false;
    unsigned int l;
    unsigned int buflen;
    
    Info(UgrLogger::Lvl4, fname, "isallowed. res: " << reqresource);
    
    // Check usernames
    i = 0;
    do {
        char buf[1024];
        CFG->ArrayGetString("glb.allowusers", buf, i);
        if (!buf[0]) break;
        if ( (buflen = strlen(buf)) < 4) {
          Error(fname, "UgrAuthorization::isallowed Bad allowusers directive: '" << buf << "'");
          break;
        }
        
        haddirectives = true;
        
        
        // Now tokenize the buffer into user, resource, modes ... space separated
        char user[256], resource[256], modes[256];
        user[0] = '\0';
        resource[0] = '\0';
        modes[0] = '\0';
        char *p1, *p2;    
        
        // Look for the space that separates userDN from permissions
        if ( (p1=strchr(buf, ' ')) ) {
            // A space was found
          
            // If the name starts with a double quote, look for a closing double quote instead of a space
            if (buf[0] == '"') {
                if ( (p1=strchr(buf+1, '"')) ) {
                  // Compute the length of the quoted string. This is the userDN
                  l = (unsigned int)(p1-buf-1);
                  
                  if ( l+2 > buflen ) {
                    Error(fname, "UgrAuthorization::isallowed Bad allowusers directive: '" << buf << "'");
                    break;
                  }
                  
                  strncpy(user, buf+1, l);
                  user[l] = '\0';
                  
                  // p1 is pointing to the closing double quote now
                  // We advance it by one, and it should point to a space. If not then it's an error.
                  if ( *(++p1) != ' ')
                    Error(fname, "UgrAuthorization::isallowed Syntax error in allowusers directive, missing separator space after closing quote: '" << buf << "'");
                  
                }
                else {
                    Error(fname, "UgrAuthorization::isallowed Mismatched quotes in allowusers directive: '" << buf << "'");
                }
                
            }
            else {
                // If no double quote, then just look for a space
                l = (unsigned int)(p1-buf);
                
                if ( l+2 > buflen ) {
                  Error(fname, "UgrAuthorization::isallowed Bad allowusers directive: '" << buf << "'");
                  break;
                }
                
                strncpy(user, buf, l);
                user[l] = '\0';
            }
            
            // Here p1 points to the space after the userDN, we look for the next one. No quotes supported in the dir name
            if ( (p2=strchr(p1+1, ' ')) ) {
                l = (unsigned int)(p2-p1-1);
                
                if ( p2-buf+3 > buflen ) {
                  Error(fname, "UgrAuthorization::isallowed Bad allowusers directive: '" << buf << "'");
                  break;
                }
                
                strncpy(resource, p1+1, l);
                resource[l] = '\0';
                
                strncpy(modes, p2+1, 255);
            }
        }
        
        if (!user[0] || !resource[0] || !modes[0]) {
            Error(fname, "UgrAuthorization::isallowed Invalid allowusers directive: '" << buf << "'");
        }
        
        Info(UgrLogger::Lvl4, fname, "UgrAuthorization::isallowed Checking user. clientName:'" << clientName << "' user:'" << user <<
        "' reqresource:'" << reqresource << "' resource:'" << resource << "' reqmode:'" << reqmode << "' modes:" << modes );
        if ( !clientName.compare(user) && 
            !strncmp(resource, reqresource, strlen(resource)) &&
            strchr(modes, reqmode) ) {
            
            // This user has been explicitely allowed
            Info(UgrLogger::Lvl3, fname, "UgrAuthorization::isallowed User allowed. clientName:" << clientName << " resource:" << resource );
        return true;
        
            }
            
            ++i;
    } while (1);
    
    // We are here if no matching userallow directives have been found
    // Check groups
    i = 0;
    do {
        char buf[1024];
        bool iswildcard = false;
        
        CFG->ArrayGetString("glb.allowgroups", buf, i);
        if (!buf[0]) break;  
        haddirectives = true;
        
        // Now tokenize the buffer into user, resource, modes ... space separated
        char group[256], resource[256], modes[256];
        group[0] = '\0';
        resource[0] = '\0';
        modes[0] = '\0';
        char *p1, *p2;
        
        if ( (p1=strchr(buf, ' ')) ) {
            l = (unsigned int)(p1-buf);
            strncpy(group, buf, l);
            if ( (l > 2) && (group[l-1] == '*') ) iswildcard = true;
            group[l] = '\0';
            
            if ( (p2=strchr(p1+1, ' ')) ) {
                l = (unsigned int)(p2-p1-1);
                strncpy(resource, p1+1, l);
                resource[l] = '\0';
                
                strncpy(modes, p2+1, 255);
            }
        }
        
        if (!group[0] || !resource[0] || !modes[0]) {
            Error("UgrAuthorization::isallowed", "invalid allowgroups directive: '" << buf << "'");
        }
        
        Info(UgrLogger::Lvl4, "isallowed", "Checking group. reqresource:'" << reqresource << "' resource:'" << resource << "' reqmode:'" << reqmode << "' modes:" << modes );
        
        if ( !strncmp(resource, reqresource, strlen(resource)) &&
            strchr(modes, reqmode) ) {
            
            for (unsigned int j = 0; j < fqans.size(); j++ ) {
                Info(UgrLogger::Lvl4, "isallowed", "Checking group. fqan:'" << fqans[j] << "' group:'" << group <<
                "' reqresource:'" << reqresource << "' resource:'" << resource << "' reqmode:'" << reqmode << "' modes:" << modes );
                
                if (iswildcard) {
                    if (!strncmp(fqans[j].c_str(), group, strlen(group)-1)) {  
                        Info(UgrLogger::Lvl3, "isallowed", "Group allowed. group:" << group << " resource:" << resource );
                        return true;
                    }
                }
                else {
                    if (!strcmp(fqans[j].c_str(), group)) {  
                        Info(UgrLogger::Lvl3, "isallowed", "Group allowed. group:" << group << " resource:" << resource );
                        return true;
                    }
                }
            }
            
            }
            
            ++i;
    } while (1);
    
    if (haddirectives) {
        Info(UgrLogger::Lvl3, "isallowed", "Denied." );
        return false;
    }
    
    Info(UgrLogger::Lvl3, "isallowed", "No auth directives, hence allowed." );
    return true;
}




// ------------------------------------------------------------------------------------
// Plugin-related stuff
// ------------------------------------------------------------------------------------




// The plugin hook function. GetPluginInterfaceClass must be given the name of this function
// for the plugin to be loaded

// Note, we need these two lines in a real plugin. This is the default one, linked with the main libs.
//extern "C" PluginInterface *GetPluginInterface(GetPluginInterfaceArgs) {
//    return (PluginInterface *)new UgrAuthorizationPlugin(c, parms);
//}

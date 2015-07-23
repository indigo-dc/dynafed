

/**
 *  Copyright (c) CERN 2014
 *
 *  Licensed under the Apache License, Version 2.0
 *  See the LICENSE file for further information
 */


/** 
 * @file   UgrLocPlugin_davrucio.cc
 * @brief  Plugin that talks to any Webdav compatible endpoint, applying the Rucio replica name hash-based xlation
 * @author Fabrizio Furano
 * @date   Oct 2014
 */

#include "UgrLocPlugin_davrucio.hh"
#include "../../PluginLoader.hh"
#include "../../ExtCacheHandler.hh"
#include "../utils/HttpPluginUtils.hh"
#include <time.h>
#include "libs/time_utils.h"
#include <openssl/md5.h>

using namespace boost;
using namespace std;


// ------------------------------------------------------------------------------------
// Plugin-related stuff
// ------------------------------------------------------------------------------------



UgrLocPlugin_davrucio::UgrLocPlugin_davrucio(UgrConnector & c, std::vector<std::string> & parms) :
    UgrLocPlugin_dav(c, parms) {
      
    const char *fname = "UgrLocPlugin_davrucio::UgrLocPlugin_davrucio";

    // Get the xlatepfx_ruciohash from the config
    // It works like xlatepfx, hence it creates an alias of the namespace
    // The difference is that if this prefix is recognized, then ALSO the rucio hashing is applied
    std::string pfx = "locplugin.";
    pfx += name;

    std::string s = pfx;
    s += ".xlatepfx_ruciohash";

    std::string v;
    v = CFG->GetString(s.c_str(), (char *) "");

    if (v.size() > 0) {


        vector<string> parms = tokenize(v, " ");
        if (parms.size() < 2) {
            Error(fname, "Bad xlatepfx_ruciohash: '" << v << "'");
        } else {
            unsigned int i;
            for (i = 0; i < parms.size() - 1; i++)
                xlatepfxruciohash_from.push_back(parms[i]);
	    
            xlatepfxruciohash_to = parms[parms.size()-1];
	    UgrFileInfo::trimpath(xlatepfxruciohash_to);
	    
            for (i = 0; i < parms.size() - 1; i++) {
		UgrFileInfo::trimpath(xlatepfxruciohash_from[i]);
                Info(UgrLogger::Lvl1, fname, name << " Translating prefixes with Rucio hashing '" << xlatepfxruciohash_from[i] << "' -> '" << xlatepfxruciohash_to << "'");
            }
        }
    }
    Info(UgrLogger::Lvl1, "UgrLocPlugin_[davrucio]", "UgrLocPlugin_[davrucio]: WebDav ENABLED");
    
}

/// Perform the sophisticated rucio-friendly name translation
/// If 'from' matches any of the xlatepfx_ruciohash then
///   translate this pfx
///   add the hashes 
///   return 0 (OK)
/// Otherwise invoke the inherited doNameXlation and return its return value
int UgrLocPlugin_davrucio::doNameXlation(std::string &from, std::string &to, workOp op, std::string &altpfx) {
    const char *fname = "LocationPlugin_davrucio::doNameXlation";
    int r = 1;
    size_t i;
    const size_t xtlate_size = xlatepfxruciohash_from.size();

    if(xtlate_size == 0){ // no rucio processing required
      return LocationPlugin::doNameXlation(from, to, op, altpfx);
    }
    else {
      for (i = 0; i < xtlate_size; i++) {
	if ((xlatepfxruciohash_from[i].size() > 0) &&
	  ((from.size() == 0) || (from.compare(0, xlatepfxruciohash_from[i].length(), xlatepfxruciohash_from[i]) == 0))) {
	  
	  if (from.size() == 0)
	    to = xlatepfxruciohash_to;
	  else {
	    if (xlatepfxruciohash_to == "/") to = from.substr(xlatepfxruciohash_from[i].length());
	    else
	      to = xlatepfxruciohash_to + from.substr(xlatepfxruciohash_from[i].length());
	  }
	  
	  r = 0;
	break;
	
	  }
      }
      
      if (r) to = from;
    }

    LocPluginLogInfo(UgrLogger::Lvl3, fname, "xlated pfx: " << from << "->" << to);

    // If r is nonzero then a xlatepfxruciohash translation was specified, AND no matching prefix was found
    // In this case we proceed with the normal name xlation, as this query has not been recognized as a Rucio one
    if (r) {
      LocPluginLogInfo(UgrLogger::Lvl3, fname, "No match on xlated pfx: " << from);
      return LocationPlugin::doNameXlation(from, to, op, altpfx);
    }
    
    // We are here if the prefix xlationruciohash for the query succeeded, and the path has been translated
    // AND there was some xlationruciohash to apply
    //

    
    // We are here, hence the path has been fully translated. We need to add the rucio hashes
    // IMPORTANT... here we are assuming that the query is directly hashable, hence of the
    // format /rucio/scope:file
    
    if (to.size() >= 14) {
      
      char md5string[33];
      unsigned char md5digest[16];
      string tmp = to.substr(strlen("/rucio/"));
      
      // Find the last slash and change it into ':'
      size_t p = tmp.rfind("/");
      if (p != string::npos) tmp[p] = ':';
      
      // Change all the slashes into dots
      for (unsigned int i = 1; i < tmp.size(); i++)
	if (tmp[i] == '/') tmp[i] = '.';
	else if (tmp[i] == ':') break;
	
	MD5((const unsigned char*)tmp.c_str(), tmp.size(), md5digest);
      
      for(int i = 0; i < 16; ++i)
	sprintf(&md5string[i*2], "%02x", (unsigned int)md5digest[i]);
      
      LocPluginLogInfo(UgrLogger::Lvl4, fname, "Rucio MD5 of " << tmp << " is:" << md5string);
      
      // Create the hash part, in the format /XX/YY ready to be inserted
      char tmp2[16];
      tmp2[0] = '\0';
      strcat(tmp2, "/");
      strncat(tmp2, md5string, 2);
      strcat(tmp2, "/");
      strncat(tmp2, md5string+2, 2);
      
      // Now insert the hashes into the path, before the last slash,
      // that is assumed to delimit the filename
      p = to.rfind("/");
      if (p != string::npos) to.insert(p, tmp2);
      
      LocPluginLogInfo(UgrLogger::Lvl4, fname, "final hashed pfx: " << from << "->" << to);
    }
    
    
        // We may have a xlation suggestion coming from the pfxmultiply, in the format of a bare prefix
    // to add to this query. An usage of this is to prepend the names of spacetokens
    // where to multiply to query to
    if (altpfx.size() > 1) {
      to.insert(0, altpfx);
    }
     
    LocPluginLogInfo(UgrLogger::Lvl2, fname, "final xlated pfx: " << from << "->" << to);
  return 0;
}






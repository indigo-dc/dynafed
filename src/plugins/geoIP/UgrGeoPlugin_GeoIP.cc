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


/** @file   GeoPlugin.hh
 * @brief  Base class for an UGR plugin that assigns GPS coordinates to FileItems based on the server that they specify
 * @author Fabrizio Furano
 * @date   Mar 2012
 */




/*
      Equirectangular approximation

      
      If performance is an issue and accuracy less important, for small distances Pythagoras’
      theorem can be used on an equirectangular projection:

      Formula:
        x = Δλ ⋅ cos φm
        y = Δφ
        d = R ⋅ √x² + y²
        
      JavaScript:     
        var x = (λ2-λ1) * Math.cos((φ1+φ2)/2);
        var y = (φ2-φ1);
        var d = Math.sqrt(x*x + y*y) * R;
        
      This uses just one trig and one sqrt function – as against half-a-dozen
      trig functions for cos law, and 7 trigs + 2 sqrts for haversine.
      Accuracy is somewhat complex: along meridians there are no errors, otherwise they depend on distance,
      bearing, and latitude, but are small enough for many purposes
      (and often trivial compared with the spherical approximation itself).
      */



#include "UgrGeoPlugin_GeoIP.hh"

#include "GeoIPCity.h"
#include <random>       // std::default_random_engine
#include <chrono>       // std::chrono::system_clock
#include <algorithm>    // std::shuffle

using namespace std;



UgrGeoPlugin_GeoIP::UgrGeoPlugin_GeoIP(UgrConnector & c, std::vector<std::string> & parms)  : FilterPlugin(c, parms){
    CFG->Set(&c.getConfig());

    const char *fname = "UgrGeoPlugin::UgrGeoPlugin_GeoIP";
    Info(UgrLogger::Lvl1, fname, "Creating instance.");

    gi = 0;
    init(parms);
    
    
    // Approximately 10km by default
    long ifuzz = CFG->GetLong("glb.filterplugin.geoip.fuzz", 10);
    fuzz = ifuzz / 6371000.0; // Radius of Earth in Km
    fuzz = fuzz * fuzz;
    Info(UgrLogger::Lvl4, "UgrFileItemGeoComp::applyFilterOnReplicaList", "Fuzz " << ifuzz << " normalized into " << fuzz);
    
    // obtain a time-based seed:
    seed = std::chrono::system_clock::now().time_since_epoch().count();

}

UgrGeoPlugin_GeoIP::~UgrGeoPlugin_GeoIP(){

}

// Init the geoIP stuff, try to get the best info that is available
// The format for parms is
// /usr/lib64/libugrgeoplugin_geoip.so <name> <path to the GeoIP db>
int UgrGeoPlugin_GeoIP::init(std::vector<std::string> &parms) {
    const char *fname = "UgrGeoPlugin::Init";

    if (parms.size() < 3) {
        Error(fname, "Too few parameters.");
        return 1;
    }

    gi = GeoIP_open(parms[2].c_str(), GEOIP_MEMORY_CACHE);
    if (!gi) {
        Error(fname, "Error opening GeoIP database: " << parms[2].c_str());
        return 2;
    }


    return 0;
}


void UgrGeoPlugin_GeoIP::hookNewReplica(UgrFileItem_replica &replica){
    setReplicaLocation(replica);

}

bool lessthan(const UgrFileItem_replica &i, const UgrFileItem_replica &j) { return (i.tempDistance < j.tempDistance); }

int UgrGeoPlugin_GeoIP::applyFilterOnReplicaList(UgrReplicaVec&replica, const UgrClientInfo &cli_info){
    float cli_latitude=0, cli_longitude=0;

    if(gi == NULL)
        return 0;
    
    if (replica.size() < 2) return 0;
    
    getAddrLocation(cli_info.ip, cli_latitude, cli_longitude);
    
    // Assign distances to all the objects
    for (UgrReplicaVec::iterator i = replica.begin(); i != replica.end(); i++) {
      
      float x, y;
      // Distance client->repl1
      x = (i->longitude-cli_longitude) * cos( (cli_latitude+i->latitude)/2 );
      y = (i->latitude-cli_latitude);
      i->tempDistance = x*x + y*y;
      
      Info(UgrLogger::Lvl4, "UgrGeoPlugin_GeoIP::applyFilterOnReplicaList",
        "GeoDistance " << "d1=("<< i->latitude << "," << i->longitude << ", d:" << i->tempDistance << ", " << i->location << ") " );

    }
    
    
    //UgrFileItemGeoComp comp_geo(fuzz);

    std::sort(replica.begin(), replica.end(), lessthan);

    // Shuffle the elements that are within the fuzz value
    if (fuzz > 0.0) {
      float d = -1;
      UgrReplicaVec::iterator b = replica.begin();
      for (UgrReplicaVec::iterator i = replica.begin(); i != replica.end(); i++) {
        if (d < 0) d = i->tempDistance;
          
        if (fabs(i->tempDistance - d) > fuzz) {
          
          shuffle (b, i, std::default_random_engine(seed));
  
          //std::random_shuffle ( b, i );
          d = i->tempDistance;
          b = i;
        }
        
      }
    }
    
    return 0;
}

/// Sets, wherever needed the geo information in the replica
void UgrGeoPlugin_GeoIP::setReplicaLocation(UgrFileItem_replica &it) {
    const char *fname = "UgrGeoPlugin::setReplicaLocation";

    // Get the server name from the name field
    Info(UgrLogger::Lvl4, fname, "Got name: " << it.name);
    // skip delimiters at beginning.
    string::size_type lastPos = it.name.find_first_not_of(" :/\\", 0);
    if (lastPos == string::npos) return;

    // find first ":".
    string::size_type pos = it.name.find_first_of(":", lastPos);
    if (pos == string::npos) return;

    // skip slashes
    lastPos = it.name.find_first_not_of(":/", pos);
    if (lastPos == string::npos) return;

    // find slash or : after hostname
    pos = it.name.find_first_of(":/\\", lastPos);
    if (pos == string::npos) return;

    string srv = it.name.substr(lastPos, pos-lastPos);
    Info(UgrLogger::Lvl4, fname, "pos:" << pos << " lastpos: " << lastPos);
    Info(UgrLogger::Lvl4, fname, "Got server: " << srv);

    GeoIPRecord *gir = GeoIP_record_by_name(gi, (const char *)srv.c_str());

    if (gir == NULL) {
        Error(fname, "GeoIP_record_by_name failed: " << srv.c_str());
        return;
    }

    Info(UgrLogger::Lvl3, fname, "Set geo info: " << it.name << srv << " " << gir->country_name << " " << gir->city << " " << gir->latitude << " " << gir->longitude);
    
    // Convert here into radians so we save a few operations later
    it.latitude = gir->latitude / 180.0 * M_PI;
    it.longitude = gir->longitude / 180.0 * M_PI;

    if (gir->city)
        it.location = gir->city;
    else if (gir->country_name)
        it.location = gir->country_name;

    GeoIPRecord_delete(gir);
    return;
}



void UgrGeoPlugin_GeoIP::getAddrLocation(const std::string &clientip, float &ltt, float &lng) {
    const char *fname = "UgrGeoPlugin::getAddrLocation";

    if (clientip.empty()) return;
    
    GeoIPRecord *gir = GeoIP_record_by_name(gi, (const char *)clientip.c_str());

    if (gir == NULL) {
        Error(fname, "GeoIP_record_by_name failed: " << clientip.c_str());
        ltt = lng = 0.0;
        return;
    }

    ltt = gir->latitude / 180.0 * M_PI;
    lng = gir->longitude / 180.0 * M_PI;

    
    GeoIPRecord_delete(gir);

    Info(UgrLogger::Lvl4, fname, clientip << " " << ltt << " " << lng);
}



// ------------------------------------------------------------------------------------
// Plugin-related stuff
// ------------------------------------------------------------------------------------



/// The plugin hook function. GetPluginInterfaceClass must be given the name of this function
/// for the plugin to be loaded
extern "C" PluginInterface * GetPluginInterface(GetPluginInterfaceArgs) {
    return (PluginInterface *)new UgrGeoPlugin_GeoIP(c, parms);
}

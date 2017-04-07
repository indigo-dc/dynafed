/*
 *  Copyright (c) CERN 2017
 *
 *  Licensed under the Apache License, Version 2.0
 *  See the LICENSE file for further information
 * 
 */



/** @file  UgrGeoPlugin_mmdb.cc
 * @brief  An UGR filter plugin that assigns geographical coordinates to FileItems based on the server that they specify
 * @author Fabrizio Furano
 * @date   Apr 2017
 */




/*
 *      Equirectangular approximation
 * 
 *      
 *      If performance is an issue and accuracy less important, for small distances Pythagoras’
 *      theorem can be used on an equirectangular projection:
 * 
 *      Formula:
 *        x = Δλ ⋅ cos φm
 *        y = Δφ
 *        d = R ⋅ √x² + y²
 *        
 *      JavaScript:     
 *        var x = (λ2-λ1) * Math.cos((φ1+φ2)/2);
 *        var y = (φ2-φ1);
 *        var d = Math.sqrt(x*x + y*y) * R;
 *        
 *      This uses just one trig and one sqrt function – as against half-a-dozen
 *      trig functions for cos law, and 7 trigs + 2 sqrts for haversine.
 *      Accuracy is somewhat complex: along meridians there are no errors, otherwise they depend on distance,
 *      bearing, and latitude, but are small enough for many purposes
 *      (and often trivial compared with the spherical approximation itself).
 */



#include "UgrGeoPlugin_mmdb.hh"

using namespace std;



UgrGeoPlugin_mmdb::UgrGeoPlugin_mmdb(UgrConnector & c, std::vector<std::string> & parms)  : FilterPlugin(c, parms){
  UgrCFG->Set(&c.getConfig());
  
  const char *fname = "UgrGeoPlugin_mmdb::UgrGeoPlugin_GeoIP";
  Info(UgrLogger::Lvl1, fname, "Creating instance.");
  
  mmdb_ok = false;
  memset (&mmdb, 0, sizeof(mmdb));
  
  init(parms);
  
  // Approximately 10km by default
  long ifuzz = UgrCFG->GetLong("glb.filterplugin.mmdb.fuzz", 10);
  fuzz = ifuzz / 6371.0; // Radius of Earth in Km
  fuzz = fuzz * fuzz;
  Info(UgrLogger::Lvl4, "UgrGeoPlugin_mmdb::UgrGeoPlugin_mmdb", "Fuzz " << ifuzz << " normalized into " << fuzz);
  
  seed = time(0);
}

UgrGeoPlugin_mmdb::~UgrGeoPlugin_mmdb(){
  
}

// Init the geoIP stuff, try to get the best info that is available
// The format for parms is
// /usr/lib64/libugrgeoplugin_geoip.so <name> <path to the GeoIP db>
int UgrGeoPlugin_mmdb::init(std::vector<std::string> &parms) {
  const char *fname = "UgrGeoPlugin_mmdb::Init";
  
  if (parms.size() < 3) {
    Error(fname, "Too few parameters.");
    return 1;
  }
  
  int status = MMDB_open(parms[2].c_str(), MMDB_MODE_MMAP, &mmdb);
  if (status != MMDB_SUCCESS) {
    Error(fname, "Error opening MMDB database: " << parms[2].c_str());
    return 2;
  }
  
  mmdb_ok = true;
  return 0;
}


void UgrGeoPlugin_mmdb::hookNewReplica(UgrFileItem_replica &replica){
  setReplicaLocation(replica);
  
}

// Local utility func to shuffle elements. Can't use std::shuffle here due
// to the need of supporting obsolete platform like SL5 and SL6
void UgrGeoPlugin_mmdb::ugrgeorandom_shuffle( UgrReplicaVec::iterator first,
                                              UgrReplicaVec::iterator last )
{
  UgrReplicaVec::iterator::difference_type i, n;
  n = last - first;
  for (i = n-1; i > 0; --i) {
    std::swap(first[i], first[rand_r(&seed) % (i+1)]);
  }
}
bool lessthan(const UgrFileItem_replica &i, const UgrFileItem_replica &j) { return (i.tempDistance < j.tempDistance); }

int UgrGeoPlugin_mmdb::applyFilterOnReplicaList(UgrReplicaVec&replica, const UgrClientInfo &cli_info){
  float cli_latitude=0, cli_longitude=0;
  
  if(!mmdb_ok) return 0;
  
  if (replica.size() < 2) return 0;
  
  getAddrLocation(cli_info.ip, cli_latitude, cli_longitude);
  
  // Assign distances to all the objects
  for (UgrReplicaVec::iterator i = replica.begin(); i != replica.end(); i++) {
    
    float x, y;
    // Distance client->repl1
    x = (i->longitude-cli_longitude) * cos( (cli_latitude+i->latitude)/2 );
    y = (i->latitude-cli_latitude);
    i->tempDistance = x*x + y*y;
    
    Info(UgrLogger::Lvl4, "UgrGeoPlugin_mmdb::applyFilterOnReplicaList",
         "GeoDistance " << "d1=("<< i->latitude << "," << i->longitude << ", d:" << i->tempDistance << ", " << i->location << ") " );
    
  }
  
  
  //UgrFileItemGeoComp comp_geo(fuzz);
  
  std::sort(replica.begin(), replica.end(), lessthan);
  
  // Shuffle the elements that are within the fuzz value
  if (fuzz > 0.0) {
    float d = -1;
    UgrReplicaVec::iterator b = replica.begin();
    for (UgrReplicaVec::iterator i = replica.begin(); ; i++) {
      
      if (i == replica.end()) {
        ugrgeorandom_shuffle (b, i);
        break;
      }
      
      if (d < 0) d = i->tempDistance;
      
      if (fabs(i->tempDistance - d) > fuzz) {
        
        ugrgeorandom_shuffle (b, i);
        
        //std::random_shuffle ( b, i );
        d = i->tempDistance;
        b = i;
      }
      
    }
  }
  
  return 0;
}

/// Sets, wherever needed the geo information in the replica
void UgrGeoPlugin_mmdb::setReplicaLocation(UgrFileItem_replica &it) {
  const char *fname = "UgrGeoPlugin_mmdb::setReplicaLocation";
  
  // Get the server name from the name field
  Info(UgrLogger::Lvl4, fname, "Got name: " << it.name);
  
  if (!mmdb_ok) return;
  
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
  
  // Do the mmdb lookup
  int gai_error, mmdb_error;
  MMDB_lookup_result_s result =
    MMDB_lookup_string(&mmdb,
                       (const char *)srv.c_str(), &gai_error, &mmdb_error);
  if (0 != gai_error) { 
    Error(fname, "MMDB_lookup_string failed. gai_error: " << gai_error);
    return;
  }
  if (MMDB_SUCCESS != mmdb_error) { 
    Error(fname, "MMDB_lookup_string failed. mmdb_error: " << gai_error);
    return;
  }
    
  if (!result.found_entry) { 
    Error(fname, "Can't find location for '" << srv);
    return;
  }
  
  // The lookup was successful
  it.location.clear();
  it.latitude = 0.0;
  it.longitude = 0.0;
  // now dig in to the result to get the city name
  MMDB_entry_data_s entry_data;
  
  int status = MMDB_get_value(&result.entry, &entry_data,
                 "city", "names", "en", NULL);
  if ((status == MMDB_SUCCESS) && (entry_data.has_data)) {
    it.location = entry_data.utf8_string;
    Info(UgrLogger::Lvl4, fname, "Got city: " << it.location);
  }

  // No city ? Try with the country name
  status = MMDB_get_value(&result.entry, &entry_data,
                              "country", "names", "en", NULL);
  if ((status == MMDB_SUCCESS) && (entry_data.has_data)) {
    if (it.location.length() > 0)
      it.location += ", ";
    it.location += entry_data.utf8_string;
    Info(UgrLogger::Lvl4, fname, "Got country: " << it.location);
  }

  // Now get the galactic coordinates
  double latitude = 0.0, longitude = 0.0;
  status = MMDB_get_value(&result.entry, &entry_data,
                          "location", "latitude", NULL);
  if ((status == MMDB_SUCCESS) && (entry_data.has_data)) {
    latitude += entry_data.double_value;
    Info(UgrLogger::Lvl4, fname, "Got latitude: " << it.latitude);
  }
  status = MMDB_get_value(&result.entry, &entry_data,
                          "location", "longitude", NULL);
  if ((status == MMDB_SUCCESS) && (entry_data.has_data)) {
    longitude += entry_data.double_value;
    Info(UgrLogger::Lvl4, fname, "Got longitude: " << it.longitude);
  }
  
  Info(UgrLogger::Lvl2, fname, "Set geo info: '" << it.name << "' srv: '"<< srv << "' loc: '" <<
    it.location << "' coords: " << it.latitude << " " << it.longitude);
  
  // Convert here into radians so we save a few operations later
  it.latitude = latitude / 180.0 * M_PI;
  it.longitude = longitude / 180.0 * M_PI;
  
  
  return;
}



void UgrGeoPlugin_mmdb::getAddrLocation(const std::string &clientip, float &ltt, float &lng) {
  const char *fname = "UgrGeoPlugin_mmdb::getAddrLocation";
  if (!mmdb_ok) return;
  if (clientip.empty()) return;
  
  std::string location;
  
  // Do the mmdb lookup
  int gai_error, mmdb_error;
  MMDB_lookup_result_s result =
    MMDB_lookup_string(&mmdb,
      (const char *)clientip.c_str(), &gai_error, &mmdb_error);
    
  if (0 != gai_error) { 
    Error(fname, "MMDB_lookup_string failed. gai_error: " << gai_error);
    return;
  }
  if (MMDB_SUCCESS != mmdb_error) { 
    Error(fname, "MMDB_lookup_string failed. mmdb_error: " << gai_error);
    return;
  }
  
  if (!result.found_entry) { 
    Error(fname, "Can't find location for '" << clientip);
    return;
  }
  
  // The lookup was successful
  ltt = 0.0;
  lng = 0.0;
  // now dig into the result to get the city name
  MMDB_entry_data_s entry_data;
  
  int status = MMDB_get_value(&result.entry, &entry_data,
                              "city", "names", "en", NULL);
  if ((status == MMDB_SUCCESS) && (entry_data.has_data)) {
    location = entry_data.utf8_string;
    Info(UgrLogger::Lvl4, fname, "Got city: " << location);
  }
  
  // No city ? Try with the country name
  status = MMDB_get_value(&result.entry, &entry_data,
                          "country", "names", "en", NULL);
  if ((status == MMDB_SUCCESS) && (entry_data.has_data)) {
    if (location.length() > 0)
      location += ", ";
    location += entry_data.utf8_string;
    Info(UgrLogger::Lvl4, fname, "Got country: " << location);
  }
  
  // Now get the galactic coordinates
  status = MMDB_get_value(&result.entry, &entry_data,
                          "location", "latitude", NULL);
  if ((status == MMDB_SUCCESS) && (entry_data.has_data)) {
    ltt = entry_data.double_value;
    Info(UgrLogger::Lvl4, fname, "Got latitude: " << ltt);
  }
  status = MMDB_get_value(&result.entry, &entry_data,
                          "location", "longitude", NULL);
  if ((status == MMDB_SUCCESS) && (entry_data.has_data)) {
    lng = entry_data.double_value;
    Info(UgrLogger::Lvl4, fname, "Got longitude: " << lng);
  }
  

  
  ltt = ltt / 180.0 * M_PI;
  lng = lng / 180.0 * M_PI;
  
  
  
  Info(UgrLogger::Lvl2, fname, clientip << " " << ltt << " " << lng << " '" << location << "'");
}



// ------------------------------------------------------------------------------------
// Plugin-related stuff
// ------------------------------------------------------------------------------------



/// The plugin hook function. GetPluginInterfaceClass must be given the name of this function
/// for the plugin to be loaded
extern "C" PluginInterface * GetPluginInterface(GetPluginInterfaceArgs) {
  return (PluginInterface *)new UgrGeoPlugin_mmdb(c, parms);
}

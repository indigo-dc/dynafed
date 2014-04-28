/** @file   GeoPlugin.hh
 * @brief  Base class for an UGR plugin that assigns GPS coordinates to FileItems based on the server that they specify
 * @author Fabrizio Furano
 * @date   Mar 2012
 */


#include "UgrGeoPlugin_GeoIP.hh"

#include "GeoIPCity.h"

using namespace std;

/// Instances of UgrFileItem may be kept in a quasi-sorted way.
/// This is the compare functor that sorts them by distance from a point
class UgrFileItemGeoComp {
private:
    float ltt, lng;
public:

    UgrFileItemGeoComp(float latitude, float longitude): ltt(latitude), lng(longitude) {
        //std::cout << "geocomp" << std::endl;
    };
    virtual ~UgrFileItemGeoComp(){};

    virtual bool operator()(const UgrFileItem_replica &s1, const UgrFileItem_replica &s2) {
        float x, y, d1, d2;

        //std::cout << "client" << ltt << " " << lng << std::endl;

        // Distance client->repl1
        x = (s1.longitude-lng) * cos( (ltt+s1.latitude)/2 );
        y = (s1.latitude-ltt);
        d1 = x*x + y*y;

        //std::cout << "d1 " << d1 << std::endl;

        // Distance client->repl2
        x = (s2.longitude-lng) * cos( (ltt+s2.latitude)/2 );
        y = (s2.latitude-ltt);
        d2 = x*x + y*y;

        //std::cout << "d2 " << d2 << std::endl;
        return (d1 < d2);
    }
};


UgrGeoPlugin_GeoIP::UgrGeoPlugin_GeoIP(UgrConnector & c, std::vector<std::string> & parms)  : FilterPlugin(c, parms){
    CFG->Set(&c.getConfig());

    const char *fname = "UgrGeoPlugin::UgrGeoPlugin_GeoIP";
    Info(SimpleDebug::kLOW, fname, "Creating instance.");

    gi = 0;
    init(parms);
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

int UgrGeoPlugin_GeoIP::filterReplicaList(std::deque<UgrFileItem_replica> &replica, const UgrClientInfo &cli_info){
    float cli_lattitude=0, cli_longittude=0;

    if(gi == NULL)
        return 0;
    getAddrLocation(cli_info.ip, cli_lattitude, cli_longittude);
    UgrFileItemGeoComp comp_geo(cli_lattitude, cli_longittude);

    for(std::deque<UgrFileItem_replica>::iterator it = replica.begin(); it != replica.end(); ++it){
        setReplicaLocation(*it);
    }
    std::sort(replica.begin(), replica.end(), comp_geo);

    return 0;
}

/// Sets, wherever needed the geo information in the replica
void UgrGeoPlugin_GeoIP::setReplicaLocation(UgrFileItem_replica &it) {
    const char *fname = "UgrGeoPlugin::setReplicaLocation";

    // Get the server name from the name field
    Info(SimpleDebug::kHIGHEST, fname, "Got name: " << it.name);
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
    Info(SimpleDebug::kHIGHEST, fname, "pos:" << pos << " lastpos: " << lastPos);
    Info(SimpleDebug::kHIGHEST, fname, "Got server: " << srv);

    GeoIPRecord *gir = GeoIP_record_by_name(gi, (const char *)srv.c_str());

    if (gir == NULL) {
        Error(fname, "GeoIP_record_by_name failed: " << srv.c_str());
        return;
    }

    Info(SimpleDebug::kLOW, fname, "Set geo info: " << it.name << srv << " " << gir->country_name << " " << gir->city << " " << gir->latitude << " " << gir->longitude);
    it.latitude = gir->latitude;
    it.longitude = gir->longitude;

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

    ltt = gir->latitude;
    lng = gir->longitude;

    
    GeoIPRecord_delete(gir);

    Info(SimpleDebug::kHIGHEST, fname, clientip << " " << ltt << " " << lng);
}



// ------------------------------------------------------------------------------------
// Plugin-related stuff
// ------------------------------------------------------------------------------------



/// The plugin hook function. GetPluginInterfaceClass must be given the name of this function
/// for the plugin to be loaded
extern "C" PluginInterface * GetPluginInterface(GetPluginInterfaceArgs) {
    return (PluginInterface *)new UgrGeoPlugin_GeoIP(c, parms);
}

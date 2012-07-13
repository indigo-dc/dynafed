#include "davavailabilitychecker.hh"


DavAvailabilityChecker::DavAvailabilityChecker(Davix::CoreInterface* davx, const std::string & _uri_ping) : 
			uri_ping(_uri_ping), dav_context(davx)
{
	last_state = PLUGIN_ENDPOINT_ONLINE;
	latency =0;
}


void DavAvailabilityChecker::get_availability(PluginEndpointStatus * status){
	status->state = last_state;
	status->latency = latency;
	status->explanation = explanation;
}	



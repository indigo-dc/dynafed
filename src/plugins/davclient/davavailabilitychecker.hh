
#ifndef DAVAVAILABILITYCHECKER_HH
#define DAVAVAILABILITYCHECKER_HH

/** 
 * @file   davavailabilitychecker.hh
 * @brief  poll the status of the http endpoint
 * @author Devresse Adrien
 */
 
#include "UgrLocPlugin_dav.hh"

class DavAvailabilityChecker
{
public:
	DavAvailabilityChecker(Davix::CoreInterface* davx, const std::string & uri_ping);
	
	void get_availability(PluginEndpointStatus * status);	
private:
	unsigned long time_interval;	
	std::string uri_ping;
	Davix::CoreInterface* dav_context;
	
	
	// stats
	PluginEndpointState last_state;
	unsigned long latency;
	/// string description
	std::string explanation;	
};

#endif /* DAVAVAILABILITYCHECKER_HH */ 

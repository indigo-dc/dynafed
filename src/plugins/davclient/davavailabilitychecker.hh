
#ifndef DAVAVAILABILITYCHECKER_HH
#define DAVAVAILABILITYCHECKER_HH

/** 
 * @file   davavailabilitychecker.hh
 * @brief  poll the status of the http endpoint
 * @author Devresse Adrien
 */

#include <davix_cpp.hpp>
#include "../../LocationPlugin.hh"
#include "UgrLocPlugin_dav.hh"

class DavAvailabilityChecker
{
public:
	DavAvailabilityChecker(Davix::CoreInterface* davx, const std::string & uri_ping);
    virtual ~DavAvailabilityChecker();

	void get_availability(PluginEndpointStatus * status);	
private:
	unsigned long time_interval;	
	std::string uri_ping;
	Davix::CoreInterface* dav_context;
	
	
	// stats
	PluginEndpointState last_state;
	unsigned long latency;
    std::string explanation;
    pthread_t runner;

    static void* polling_task(void* args);
};

#endif /* DAVAVAILABILITYCHECKER_HH */ 

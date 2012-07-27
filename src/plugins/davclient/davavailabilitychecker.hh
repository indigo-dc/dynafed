
#ifndef DAVAVAILABILITYCHECKER_HH
#define DAVAVAILABILITYCHECKER_HH

/** 
 * @file   davavailabilitychecker.hh
 * @brief  poll the status of the http endpoint
 * @author Devresse Adrien
 */

#include <signal.h>
#include <time.h>

#include <davix_cpp.hpp>
#include "../../LocationPlugin.hh"
#include "UgrLocPlugin_dav.hh"

class DavAvailabilityChecker
{
public:
    DavAvailabilityChecker(Davix::CoreInterface* davx, const std::string & uri_ping, unsigned long time_interval=5000);
    virtual ~DavAvailabilityChecker();

	void get_availability(PluginEndpointStatus * status);	
private:
	unsigned long time_interval;	
	std::string uri_ping;
	Davix::CoreInterface* dav_context;
    struct itimerspec timer_value;
	
	// stats
	PluginEndpointState last_state;
	unsigned long latency;
    std::string explanation;
    //

    // one timer element
    struct sigevent even;
    timer_t timer;
    pthread_mutex_t update_mutex;
    pthread_mutex_t income_mutex;


    static void polling_task(union sigval);
    void first_init_timer(timer_t * t, struct sigevent* even, pthread_mutex_t *update_mutex,
                            pthread_mutex_t * income_mutex, long time_interval);



};

#endif /* DAVAVAILABILITYCHECKER_HH */ 

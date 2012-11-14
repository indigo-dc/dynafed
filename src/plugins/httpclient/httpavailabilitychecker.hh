#pragma once
#ifndef DAVAVAILABILITYCHECKER_HH
#define DAVAVAILABILITYCHECKER_HH

/** 
 * @file   davavailabilitychecker.hh
 * @brief  poll the status of the http endpoint
 * @author Devresse Adrien
 */

#include <signal.h>
#include <time.h>
#include <glibmm.h>

#include <davix_cpp.hpp>
#include "../../LocationPlugin.hh"
#include "UgrLocPlugin_http.hh"
#include "ugr_loc_plugin_http_type.hh"

class HttpAvailabilityChecker {
public:
    HttpAvailabilityChecker(Davix::Context* davx, const std::string & uri_ping, unsigned long time_interval = 5000, struct timespec* max_latency = NULL);
    virtual ~HttpAvailabilityChecker();

    void get_availability(PluginEndpointStatus * status);
private:
    unsigned long time_interval;
    std::string uri_ping;
    Davix::Context* http_context;
    struct itimerspec timer_value;
    struct timespec max_latency;

    // stats
    PluginEndpointState last_state;
    unsigned long latency;
    std::string explanation;
    //

    // one timer element
    struct sigevent even;
    timer_t timer;
    HttpPluginRWMutex update_mutex;
    int state;


    static void polling_task(union sigval);
    void first_init_timer(timer_t * t, struct sigevent* even,
            long time_interval);
    
    void setLastState(PluginEndpointState newstate, int httpcode);



};

#endif /* DAVAVAILABILITYCHECKER_HH */ 

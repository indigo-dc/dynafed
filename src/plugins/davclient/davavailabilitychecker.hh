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
#include "UgrLocPlugin_dav.hh"
#include "ugr_loc_plugin_dav_type.hh"

class DavAvailabilityChecker {
public:
    DavAvailabilityChecker(Davix::Context* davx, const std::string & uri_ping, unsigned long time_interval = 5000, struct timespec* max_latency = NULL);
    virtual ~DavAvailabilityChecker();

    void get_availability(PluginEndpointStatus * status);
private:
    unsigned long time_interval;
    std::string uri_ping;
    Davix::Context* dav_context;
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
    DavPluginRWMutex update_mutex;
    int state;


    static void polling_task(union sigval);
    void first_init_timer(timer_t * t, struct sigevent* even,
            long time_interval);

    void setLastState(PluginEndpointState newstate, int httpcode);

};

#endif /* DAVAVAILABILITYCHECKER_HH */ 

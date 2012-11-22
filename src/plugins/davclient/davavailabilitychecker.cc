
#include "davavailabilitychecker.hh"
#include <libs/time_utils.h>

DavAvailabilityChecker::DavAvailabilityChecker(Davix::Context* davx, const Davix::RequestParams & params,
                     const std::string & _uri_ping, unsigned long _time_interval ,
                                               struct timespec* max_latency ) :
    uri_ping(_uri_ping),
    dav_context(davx),
    checker_params(params)
{
    last_state = PLUGIN_ENDPOINT_UNKNOWN;
    latency = 0;
    time_interval = _time_interval;
    Info(SimpleDebug::kMEDIUM, "DavAvailabilityChecker", "Launch state checker for  :" << uri_ping << " with frequency : " << time_interval);
    state = 0;
    first_init_timer(&timer, &even, time_interval);
    if (max_latency){
        Info(SimpleDebug::kMEDIUM, "DavAvailabilityChecker", "Launch state checker for  :" << uri_ping << " with frequency : " << time_interval);
        timespec_copy(&(this->max_latency), max_latency);
        checker_params.setConnexionTimeout(max_latency);
        checker_params.setOperationTimeout(max_latency);
    }else{
        timespec_clear(&(this->max_latency));
    }
}

DavAvailabilityChecker::~DavAvailabilityChecker() {
    //Info(SimpleDebug::kLOW,"DavAvailabilityChecker", "Destroyer for state checker called " <<  uri_ping);
    timer_delete(timer);
    while (g_atomic_int_compare_and_exchange(&state, 0, -1) == false) // check if destruction occures if not -> execute
        usleep(1);
}

void DavAvailabilityChecker::get_availability(PluginEndpointStatus * status) {
    DavPluginReadLocker locker(update_mutex);
    status->state = last_state;
    status->latency = latency;
    status->explanation = explanation;
}

void DavAvailabilityChecker::first_init_timer(timer_t * t, struct sigevent* even, long time_interval) {

    int res;
    struct timespec start, interval;
    memset(even, 0, sizeof (struct sigevent));
    memset(&start, 0, sizeof (struct timespec));
    memset(&interval, 0, sizeof (struct timespec));


    even->sigev_notify = SIGEV_THREAD;
    even->sigev_value.sival_ptr = this;
    even->sigev_notify_function = &DavAvailabilityChecker::polling_task;
    res = timer_create(CLOCK_MONOTONIC, even, t);
    if( res ==0){
        timer_value.it_interval.tv_sec = time_interval / 1000;
        timer_value.it_interval.tv_nsec = (time_interval % 1000)*1000000;
        timer_value.it_value.tv_nsec = 10;

        res = timer_settime(*t, 0, &timer_value,
                NULL);
    }
    if(res !=0)
        Info(SimpleDebug::kMEDIUM, "DavAvailabilityChecker", "WARNING : Status checker init for " << uri_ping << "failed, checker disabled");

}

void DavAvailabilityChecker::polling_task(union sigval args) {
    DavAvailabilityChecker* myself = static_cast<DavAvailabilityChecker*> (args.sival_ptr);
    
    struct timespec t1, t2;
    Davix::DavixError* tmp_err = NULL;
    int code = 404;

    if (g_atomic_int_compare_and_exchange(&myself->state, 0, 1) == false) // check if destruction occures if not -> execute
        return;
    Info(SimpleDebug::kHIGH, "DavAvailabilityChecker", " Start checker for " << myself->uri_ping << " with time " << myself->time_interval << std::endl);

    boost::shared_ptr<Davix::HttpRequest> req;


    // Measure the time needed
    clock_gettime(CLOCK_MONOTONIC, &t1);

    req = boost::shared_ptr<Davix::HttpRequest > (static_cast<Davix::HttpRequest*> (myself->dav_context->createRequest(myself->uri_ping, &tmp_err)));

    // Set decent timeout values for the operation
    req->set_parameters(myself->checker_params);

    if (req.get() != NULL) {
        req->setRequestMethod("HEAD");
        if (req->executeRequest(&tmp_err) == 0)
            code = req->getRequestCode();
    }

    // Prepare the text status message to display
    if (tmp_err) {
        std::ostringstream ss;
        ss << "HTTP status error on " << myself->uri_ping << " " << tmp_err->getErrMsg();
        myself->explanation = ss.str();
        code = -1;
    }

    // Finish measuring the time needed
    clock_gettime(CLOCK_MONOTONIC, &t2);

    {
        DavPluginWriterLocker locker(myself->update_mutex);
        struct timespec diff_time;
        timespec_sub(&t2, &t1, &diff_time);
        myself->latency = (diff_time.tv_sec)*1000 + (diff_time.tv_nsec) / 1000000L;

        // For DAV we can also check that the prefix directory is known
        if (code >= 200 && code < 400) {
            if (timespec_isset(&(myself->max_latency)) && timespec_cmp(&diff_time, &(myself->max_latency), >)) {
                std::ostringstream ss;
                ss << "Latency of the endpoint " << diff_time.tv_sec << " is superior to the limit " << myself->max_latency.tv_sec << std::endl;
                myself->explanation = ss.str();
                myself->setLastState(PLUGIN_ENDPOINT_OFFLINE, code);

            } else {
                myself->explanation = "";
                myself->setLastState(PLUGIN_ENDPOINT_ONLINE, code);

            }

        } else {
            if (myself->explanation.empty()) {
                std::ostringstream ss;
                ss << "Server error reported on " << myself->uri_ping << " with code : " << code << std::endl;
                myself->explanation = ss.str();
            }
            myself->setLastState(PLUGIN_ENDPOINT_OFFLINE, code);
        }
    }
    Info(SimpleDebug::kHIGHEST, "DavAvailabilityChecker", " End checker for " << myself->uri_ping);
    g_atomic_int_set(&myself->state, 0);
}

void DavAvailabilityChecker::setLastState(PluginEndpointState newstate, int httpcode) {
    // Set state, log the status change
    // A status change in an endpoint is logged at level kLOW
    // A status setting is logged at level kHIGHEST
    short lvl = SimpleDebug::kHIGH;
    const char *online = "ONLINE";
    const char *offline = "OFFLINE";
    const char *s = online;

    if (newstate != PLUGIN_ENDPOINT_ONLINE) s = offline;

    if (newstate != last_state) lvl = SimpleDebug::kLOW;

    Info(lvl, "DavAvailabilityChecker",
            " Status of " << uri_ping <<
            " checked: " << s << ", HTTP code: " << httpcode <<
            ", desc: " << explanation);


    last_state = newstate;
}

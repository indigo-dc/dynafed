#include "davavailabilitychecker.hh"
#include <ctime>
#include <libs/time_utils.h>




DavAvailabilityChecker::DavAvailabilityChecker(Davix::CoreInterface* davx, const std::string & _uri_ping,
                                                    unsigned long _time_interval, struct timespec* max_latency) :
			uri_ping(_uri_ping), dav_context(davx)
{
	last_state = PLUGIN_ENDPOINT_ONLINE;
	latency =0;
    time_interval = _time_interval;
    Info(SimpleDebug::kMEDIUM,"DavAvailabilityChecker", "Launch state checker for  :" <<  uri_ping << " with frequency : " << time_interval);
    state =0;
    first_init_timer(&timer,&even, time_interval);
    if(max_latency)
        timespec_copy(&(this->max_latency), max_latency);
    else
        timespec_clear(&(this->max_latency));
}

DavAvailabilityChecker::~DavAvailabilityChecker(){
    //Info(SimpleDebug::kLOW,"DavAvailabilityChecker", "Destroyer for state checker called " <<  uri_ping);
    timer_delete(timer);
    while( g_atomic_int_compare_and_exchange(&state,0,-1) == false) // check if destruction occures if not -> execute
        usleep(1);
}


void DavAvailabilityChecker::get_availability(PluginEndpointStatus * status){
    DavPluginReadLocker locker(update_mutex);
	status->state = last_state;
	status->latency = latency;
	status->explanation = explanation;
}	

void DavAvailabilityChecker::first_init_timer(timer_t * t, struct sigevent* even,  long time_interval){

    int res;
    struct timespec start, interval;
    memset(even, 0, sizeof(struct sigevent));
    memset(&start, 0, sizeof(struct timespec));
    memset(&interval, 0, sizeof(struct timespec));

    even->sigev_notify = SIGEV_THREAD;
    even->sigev_value.sival_ptr = this;
    even->sigev_notify_function = &DavAvailabilityChecker::polling_task;
    res = timer_create(CLOCK_MONOTONIC, even,t);
    g_assert(res == 0);

    timer_value.it_interval.tv_sec = time_interval/1000;
    timer_value.it_interval.tv_nsec = (time_interval%1000)*1000000;
    timer_value.it_value.tv_nsec=10;

    res = timer_settime(*t, 0,&timer_value,
                             NULL);
    g_assert(res == 0);

}


void DavAvailabilityChecker::polling_task(union sigval args){
    DavAvailabilityChecker* myself = static_cast<DavAvailabilityChecker*>(args.sival_ptr);
    struct timespec t1, t2;

    if( g_atomic_int_compare_and_exchange(&myself->state,0,1) == false) // check if destruction occures if not -> execute
        return;
    Info(SimpleDebug::kMEDIUM,"DavAvailabilityChecker", " Start checker for " << myself->uri_ping << " with time " << myself->time_interval  << std::endl);
    int code = 404;
    boost::shared_ptr<Davix::HttpRequest> req;
    clock_gettime(CLOCK_MONOTONIC, &t1);
    try{
        req = boost::shared_ptr<Davix::HttpRequest>( static_cast<Davix::HttpRequest*>(myself->dav_context->getSessionFactory()->create_request(myself->uri_ping)));
        req->set_requestcustom("HEAD");
        req->execute_sync();
        code = req->get_request_code();

    }catch(Glib::Error & e){
        std::ostringstream ss;
        ss << "HTTP status error on " << myself->uri_ping << " "<< e.what();
        myself->explanation = ss.str();
        code = -1;
    }catch(...){
        myself->explanation = "Unknow string Error";
        code = -2;
    }
    clock_gettime(CLOCK_MONOTONIC, &t2);

    {
        DavPluginWriterLocker locker(myself->update_mutex);
        struct timespec diff_time;
        timespec_sub(&t2,&t1,&diff_time);
        myself->latency = (diff_time.tv_sec)*1000 + (diff_time.tv_nsec)/1000000L;
        if(code >= 200 && code <400){
            if( timespec_isset(&(myself->max_latency)) && timespec_cmp(&diff_time, &(myself->max_latency),>)){
                std::ostringstream ss;
                ss << "Latency of the endpoint " << diff_time.tv_sec << " is superior to the limit "<< myself->max_latency.tv_sec << std::endl;
                Info(SimpleDebug::kMEDIUM,"DavAvailabilityChecker", " Status of " << myself->uri_ping <<  " checked : OFFLINE Too high latency : "<< myself->latency << " superior to " << (myself->max_latency.tv_sec*1000+myself->max_latency.tv_nsec/1000000L));
                myself->last_state = PLUGIN_ENDPOINT_OFFLINE;
                myself->explanation = ss.str();
            }else{
                Info(SimpleDebug::kMEDIUM,"DavAvailabilityChecker", " Status of " << myself->uri_ping <<  " checked : ONLINE, latency : "<< myself->latency);
                myself->last_state = PLUGIN_ENDPOINT_ONLINE;
                myself->explanation = "";
            }

        }else{
            if(myself->explanation.empty()){
                std::ostringstream ss;
                ss << "Server error reported on " << myself->uri_ping << " with code : "<< code << std::endl;
                myself->explanation= ss.str();
            }
            Info(SimpleDebug::kLOW,"DavAvailabilityChecker", " Status of " << myself->uri_ping <<  " checked : OFFLINE, HTTP error code "<< code << ", error : " << myself->explanation);
            myself->last_state = PLUGIN_ENDPOINT_OFFLINE;
        }
    }
    Info(SimpleDebug::kMEDIUM,"DavAvailabilityChecker", " End checker for " << myself->uri_ping );
    g_atomic_int_set(&myself->state,0);
}

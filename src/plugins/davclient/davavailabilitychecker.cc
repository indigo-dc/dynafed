#include "davavailabilitychecker.hh"





DavAvailabilityChecker::DavAvailabilityChecker(Davix::CoreInterface* davx, const std::string & _uri_ping, unsigned long _time_interval) :
			uri_ping(_uri_ping), dav_context(davx)
{
	last_state = PLUGIN_ENDPOINT_ONLINE;
	latency =0;
    time_interval = _time_interval;
    Info(SimpleDebug::kLOW,"DavAvailabilityChecker", "Launch state checker for  :" <<  uri_ping << " with frequency : " << time_interval);
    first_init_timer(&timer,&even, &update_mutex, &income_mutex, time_interval);
}

DavAvailabilityChecker::~DavAvailabilityChecker(){
    //Info(SimpleDebug::kLOW,"DavAvailabilityChecker", "Destroyer for state checker called " <<  uri_ping);
    timer_delete(timer);
    pthread_mutex_lock(&income_mutex);
    pthread_mutex_unlock(&income_mutex);
    pthread_mutex_destroy(&income_mutex);
    pthread_mutex_destroy(&update_mutex);
}


void DavAvailabilityChecker::get_availability(PluginEndpointStatus * status){
    pthread_mutex_lock(&update_mutex);
	status->state = last_state;
	status->latency = latency;
	status->explanation = explanation;
    pthread_mutex_unlock(&update_mutex);
}	

void DavAvailabilityChecker::first_init_timer(timer_t * t, struct sigevent* even, pthread_mutex_t *update_mutex, pthread_mutex_t * income_mutex, long time_interval){

    int res;
    struct timespec start, interval;
    memset(even, 0, sizeof(struct sigevent));
    memset(&start, 0, sizeof(struct timespec));
    memset(&interval, 0, sizeof(struct timespec));

    even->sigev_notify = SIGEV_THREAD;
    even->sigev_value.sival_ptr = this;
    even->sigev_notify_function = &DavAvailabilityChecker::polling_task;
    pthread_mutex_init(update_mutex, NULL);
    pthread_mutex_init(income_mutex, NULL);
    res = timer_create(CLOCK_MONOTONIC, even,t);
    g_assert(res == 0);

    interval.tv_sec = time_interval/1000;
    interval.tv_nsec = (time_interval%1000)*1000000;
    start.tv_nsec=10;
    timer_value.it_interval = interval;
    timer_value.it_value = start;

    res = timer_settime(*t, 0,&timer_value,
                             NULL);
    g_assert(res == 0);

}


void DavAvailabilityChecker::polling_task(union sigval args){
    DavAvailabilityChecker* myself = static_cast<DavAvailabilityChecker*>(args.sival_ptr);
    struct timespec t1, t2;

    if( pthread_mutex_trylock(&(myself->income_mutex))  != 0)
        return;
    Info(SimpleDebug::kLOW,"DavAvailabilityChecker", " Start checker for " << myself->uri_ping << " with time " << myself->time_interval );
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

    pthread_mutex_lock(&(myself->update_mutex));
    myself->latency = (t2.tv_sec - t1.tv_sec)*1000 + (t2.tv_nsec-t1.tv_nsec)/1000000L;
    if(code >= 200 && code <400){
        Info(SimpleDebug::kLOW,"DavAvailabilityChecker", " Status of " << myself->uri_ping <<  " checked : ONLINE, latency : "<< myself->latency);
        myself->last_state = PLUGIN_ENDPOINT_ONLINE;
        myself->explanation = "";

    }else{
        Info(SimpleDebug::kLOW,"DavAvailabilityChecker", " Status of " << myself->uri_ping <<  " checked : OFFLINE, HTTP error code "<< code << ", error : " << myself->explanation);
        myself->last_state = PLUGIN_ENDPOINT_OFFLINE;
    }
    pthread_mutex_unlock(&(myself->update_mutex));
    pthread_mutex_unlock(&(myself->income_mutex));
 Info(SimpleDebug::kLOW,"DavAvailabilityChecker", " End checker for " << myself->uri_ping );
}

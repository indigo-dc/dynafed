#include "davavailabilitychecker.hh"




DavAvailabilityChecker::DavAvailabilityChecker(Davix::CoreInterface* davx, const std::string & _uri_ping) : 
			uri_ping(_uri_ping), dav_context(davx)
{
	last_state = PLUGIN_ENDPOINT_ONLINE;
	latency =0;
    time_interval = 6000;
    pthread_create(&runner,NULL,&DavAvailabilityChecker::polling_task, this);
}

DavAvailabilityChecker::~DavAvailabilityChecker(){
    //Info(SimpleDebug::kLOW,"DavAvailabilityChecker", "Destroyer for state checker called " <<  uri_ping);
    pthread_cancel(runner);
    pthread_join(runner,NULL);
}


void DavAvailabilityChecker::get_availability(PluginEndpointStatus * status){
	status->state = last_state;
	status->latency = latency;
	status->explanation = explanation;
}	



void* DavAvailabilityChecker::polling_task(void *args){
    DavAvailabilityChecker* myself = static_cast<DavAvailabilityChecker*>(args);
    struct timespec sleep_time;
    struct timespec t1, t2;
    sleep_time.tv_sec = myself->time_interval/1000;
    sleep_time.tv_nsec= ( myself->time_interval%1000) *1000000L;


    while(1){
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
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
        myself->latency = (t2.tv_sec - t1.tv_sec)*1000 + (t2.tv_nsec-t1.tv_nsec)/1000000L;

        if(code >= 200 && code <300){
            Info(SimpleDebug::kLOW,"DavAvailabilityChecker", " Status of " << myself->uri_ping <<  " checked : ONLINE, latency : "<< myself->latency);
            myself->last_state = PLUGIN_ENDPOINT_ONLINE;
            myself->explanation = "";

        }else{
            Info(SimpleDebug::kLOW,"DavAvailabilityChecker", " Status of " << myself->uri_ping <<  " checked : OFFLINE, error : "<< myself->explanation);
            myself->last_state = PLUGIN_ENDPOINT_OFFLINE;
        }

        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
        pthread_yield();
        if( nanosleep(&sleep_time, NULL) != 0)
            return NULL;
    }
}

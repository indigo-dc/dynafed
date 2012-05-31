#ifndef HOSTSINFOHANDLER_HH
#define HOSTSINFOHANDLER_HH

/* HostsInfoHandler
 * Handling of the info that is kept per each host
 *
 *
 * by Fabrizio Furano, CERN, Oct 2011
 */

#include "Config.hh"

#include <string>
#include <map>
#include <boost/thread.hpp>


// This class acts like a repository of information about hosts or sites
class HostsInfoHandler: public boost::shared_mutex {


   void tick(time_t timenow) {};


};





#endif


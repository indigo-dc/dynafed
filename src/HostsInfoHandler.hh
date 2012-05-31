/** @file   HostsInfoHandler.hh
 * @brief  Handler of information about hosts and endpoints
 * @author Fabrizio Furano
 * @date   Oct 2011
 */

#ifndef HOSTSINFOHANDLER_HH
#define HOSTSINFOHANDLER_HH


#include "Config.hh"

#include <string>
#include <map>
#include <boost/thread.hpp>


/// A repository of information about hosts or sites.
class HostsInfoHandler: public boost::shared_mutex {


   void tick(time_t timenow) {};


};





#endif


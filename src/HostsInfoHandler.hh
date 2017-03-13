/*
 *  Copyright (c) CERN 2013
 *
 *  Copyright (c) Members of the EMI Collaboration. 2011-2013
 *  See  http://www.eu-emi.eu/partners for details on the copyright
 *  holders.
 *
 *  Licensed under the Apache License, Version 2.0
 *  See the LICENSE file for further information
 * 
 */



/** @file   HostsInfoHandler.hh
 * @brief  Handler of information about hosts and endpoints
 * @author Fabrizio Furano
 * @date   Oct 2011
 */

#ifndef HOSTSINFOHANDLER_HH
#define HOSTSINFOHANDLER_HH


#include <string>
#include <map>
#include <boost/thread.hpp>


/// A repository of information about hosts or sites.
class HostsInfoHandler: public boost::shared_mutex {


   void tick(time_t timenow) {};


};





#endif


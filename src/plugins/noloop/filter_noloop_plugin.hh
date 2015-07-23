#ifndef FILTERNOLOOPPLUGIN_HH
#define FILTERNOLOOPPLUGIN_HH


/*
 *  Copyright (c) CERN 2014
 *
 *  Licensed under the Apache License, Version 2.0
 *  See the LICENSE file for further information
 * 
 */

/**
 * @file   filter_noloop_plugin.hh
 * @brief  Exotic filter plugin that avoid redirection loops
 * @author Devresse Adrien
 * @date   Feb 2014
 */





#include "PluginInterface.hh"
#include <boost/asio.hpp>
#include <boost/thread.hpp>


enum AddrType{
     AddrIPv4,
     AddrIpv6
};

struct HostAddr{

};


class FilterNoLoopPlugin : public FilterPlugin
{
public:
    FilterNoLoopPlugin( UgrConnector & c, std::vector<std::string> & parms);
    virtual ~FilterNoLoopPlugin();




    virtual int applyFilterOnReplicaList(UgrReplicaVec&replica, const UgrClientInfo &cli_info);

private:
};

#endif // FILTERNOLOOPPLUGIN_HH

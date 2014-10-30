#ifndef FILTERNOLOOPPLUGIN_HH
#define FILTERNOLOOPPLUGIN_HH

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



    // sort a list of replica
    // return -1 if error,  0 if ok
    virtual int filterReplicaList(UgrReplicaVec&replica, const UgrClientInfo &cli_info);

private:
};

#endif // FILTERNOLOOPPLUGIN_HH

#include "PluginInterface.hh"
#include "UgrConnector.hh"

PluginInterface::PluginInterface(UgrConnector & c, std::vector<std::string> & parms) :
    _c(c),
    myID(std::numeric_limits<int>::max()),
    _parms(parms)
{
    UgrLogger::set(&(getConn().getLogger()));
}


const std::string & PluginInterface::getPluginName() const{
    static const std::string generic_name("PluginInterface");
    return generic_name;
}


FilterPlugin::FilterPlugin(UgrConnector &c, std::vector<std::string> &parms) :
    PluginInterface(c, parms)
{

}


void FilterPlugin::hookNewReplica(UgrFileItem_replica &replica){

}

int FilterPlugin::applyFilterOnReplicaList(UgrReplicaVec&replica, const UgrClientInfo &cli_info){
    return 0;
}

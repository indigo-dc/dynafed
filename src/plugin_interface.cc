#include "plugin_interface.hh"

PluginInterface::PluginInterface(UgrConnector & c, std::vector<std::string> & parms) :
    _c(c),
    _parms(parms)
{

}


const std::string & PluginInterface::getPluginName() const{
    static const std::string generic_name("PluginInterface");
    return generic_name;
}


FilterPlugin::FilterPlugin(UgrConnector &c, std::vector<std::string> &parms) :
    PluginInterface(c, parms)
{

}

int FilterPlugin::filterReplicaList(std::deque<UgrFileItem_replica> & list_raw){
    return 0;
}

int FilterPlugin::filterReplicaList(std::deque<UgrFileItem_replica> &replica, const UgrClientInfo &cli_info){
    return 0;
}

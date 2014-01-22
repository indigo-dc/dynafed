#include "plugin_interface.hh"

PluginInterface::PluginInterface(UgrConnector & c, std::vector<std::string> & parms) :
    _c(c),
    _parms(parms)
{

}

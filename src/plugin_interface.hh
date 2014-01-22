#ifndef PLUGININTERFACE_HH
#define PLUGININTERFACE_HH

#include <string>
#include <vector>
#include <boost/noncopyable.hpp>


class UgrConnector;

class PluginInterface : protected boost::noncopyable
{
public:
    PluginInterface( UgrConnector & c, std::vector<std::string> & parms);
    virtual ~PluginInterface(){}

protected:
    UgrConnector & getConn(){
        return _c;
    }

    std::vector<std::string> & getParms(){
        return _parms;
    }

private:
    UgrConnector &_c;
    std::vector<std::string> _parms;
};

#endif // PLUGININTERFACE_HH

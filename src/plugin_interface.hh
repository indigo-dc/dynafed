#ifndef PLUGININTERFACE_HH
#define PLUGININTERFACE_HH

#include <string>
#include <vector>
#include <deque>
#include <typeinfo>
#include <boost/noncopyable.hpp>


class UgrConnector;
class UgrFileItem_replica;
class UgrClientInfo;


class PluginInterface : protected boost::noncopyable
{
public:
    PluginInterface( UgrConnector & c, std::vector<std::string> & parms);
    virtual ~PluginInterface(){}

    virtual const std::type_info & getType() const{
        return typeid(*this);
    }

    virtual const std::string & getPluginName() const;

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


class FilterPlugin : public PluginInterface {
public:
    FilterPlugin( UgrConnector & c, std::vector<std::string> & parms);
    virtual ~FilterPlugin(){}

    // sort a list of replica
    // return -1 if error,  0 if ok
    //
    // Note: All these functions, in sequence are invoked for each request
    //
    virtual int filterReplicaList(std::deque<UgrFileItem_replica> & list_raw);

    virtual int filterReplicaList(std::deque<UgrFileItem_replica> & replica, const UgrClientInfo & cli_info);

};

#endif // PLUGININTERFACE_HH

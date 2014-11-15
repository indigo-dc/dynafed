#ifndef PLUGININTERFACE_HH
#define PLUGININTERFACE_HH

#include <string>
#include <vector>
#include <deque>
#include <typeinfo>
#include <boost/noncopyable.hpp>


#include "LocationInfo.hh"

class UgrConnector;
class LocationPlugin;
class ExtCacheHandler;
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


    inline int getID() const{
        return myID;
    }

    inline void setID(int id){
        myID = id;
    }

protected:
    UgrConnector & getConn(){
        return _c;
    }

    std::vector<std::string> & getParms(){
        return _parms;
    }


private:
    UgrConnector &_c;
    int myID;
    std::vector<std::string> _parms;


};


class FilterPlugin : public PluginInterface {
public:
    FilterPlugin( UgrConnector & c, std::vector<std::string> & parms);
    virtual ~FilterPlugin(){}


    /// Called as a Hook on each new replica discovery
    virtual void hookNewReplica(UgrFileItem_replica& replica);


    /// Has to be implemented by any filter plugin in order to sort / filter the replicas list
    ///  based on arbitrary criteria
    ///
    ///  This function is called on the replica list of each locate() operation
    virtual int applyFilterOnReplicaList(UgrReplicaVec& replica, const UgrClientInfo & cli_info);


};

#endif // PLUGININTERFACE_HH

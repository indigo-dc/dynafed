#include "filter_noloop_plugin.hh"
#include <SimpleDebug.hh>
#include <LocationInfo.hh>
#include <davix.hpp>

namespace Asio_ip = boost::asio::ip;
typedef Asio_ip::udp::resolver Resolver;
typedef Asio_ip::udp::resolver::query ResolverQuery;
typedef boost::system::error_code Error_code;


FilterNoLoopPlugin::FilterNoLoopPlugin(UgrConnector &c, std::vector<std::string> &parms) :
    FilterPlugin(c, parms)
{
  Info(UgrLogger::Lvl1, "FilterNoLoopPlugin", "Filter NoLoopPlugin loaded");
}

FilterNoLoopPlugin::~FilterNoLoopPlugin(){
}


static void callback_resolve_query(std::vector<Asio_ip::address> & vec, const std::string & hostname,
                                   const Error_code& ec, Resolver::iterator iter) {
    if (ec){
         Info(UgrLogger::Lvl3, "FilterNoLoopPlugin::callback_resolve_query", "Error during resolution " << ec);
         return;
    }
    Resolver::iterator end;
    for (; iter != end; ++iter) {
        Info(UgrLogger::Lvl1, "FilterNoLoopPlugin::callback_resolve_query", " resolution " << hostname << " to " << iter->endpoint().address());
        vec.push_back(iter->endpoint().address());
    }
}


static bool is_matching_address(const std::vector< std::vector<Asio_ip::address> > & rep_vec,
                                int* i,
                                const std::vector<Asio_ip::address> & cli_vec, const UgrFileItem_replica & elem){
    bool res = false;
    if(std::find_first_of(rep_vec[*i].begin(), rep_vec[*i].end(), cli_vec.begin(), cli_vec.end()) != rep_vec[*i].end()){
        Info(UgrLogger::Lvl1, "FilterNoLoopPlugin::callback_resolve_query", " Loop detected  on " << elem.name << " deletion ");
        res = true;
    }
    (*i)++;
    return res;
}


static int filter_internal_list(UgrReplicaVec&replica, const std::vector< std::vector<Asio_ip::address> > & rep_vec,
                                const std::vector<Asio_ip::address> & cli_vec){
    int i=0;
    Info(UgrLogger::Lvl1, "FilterNoLoopPlugin::is_matching_address", " size of replicas " << replica.size() << " size of rep vec" << rep_vec.size() << " size of cli_vec" << cli_vec.size());
    std::remove_if(replica.begin(), replica.end(), boost::bind(&is_matching_address, rep_vec, &i, cli_vec, _1));
    return 0;
}


int FilterNoLoopPlugin::applyFilterOnReplicaList(UgrReplicaVec&replica, const UgrClientInfo &cli_info){
    boost::asio::io_service ios;
    Resolver res(ios);
    ResolverQuery q_cli(cli_info.ip, "http");
    std::vector<Asio_ip::address> cli_vec;
    std::vector< std::vector<Asio_ip::address> > rep_vec;
    rep_vec.reserve(replica.size());
    cli_vec.reserve(2);

    res.async_resolve(q_cli, boost::bind(&callback_resolve_query, boost::ref (cli_vec), cli_info.ip, _1, _2));

    for(std::deque<UgrFileItem_replica>::iterator it = replica.begin(); it != replica.end(); ++it){
       rep_vec.resize(rep_vec.size()+1);
       Davix::Uri uri(it->name);
       if(uri.getStatus() != Davix::StatusCode::OK){
            Info(UgrLogger::Lvl1, "FilterNoLoopPlugin::applyFilterOnReplicaList", "Invalid replica content " << it->name);
            continue;
       }
       ResolverQuery q_cli(uri.getHost(), "http");
       res.async_resolve(q_cli, boost::bind(&callback_resolve_query, boost::ref(rep_vec.back()), uri.getHost(),  _1, _2));
    }
   ios.run();

   return filter_internal_list(replica, rep_vec, cli_vec);
}




/**
 * Hook for the noloop plugin plugin
 * */
extern "C" PluginInterface *GetPluginInterface(UgrConnector &c, std::vector<std::string> &parms) {
    return (PluginInterface *)new FilterNoLoopPlugin(c, parms);
}

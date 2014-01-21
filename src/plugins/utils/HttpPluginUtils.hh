#ifndef HTTPPLUGINUTILS_HPP
#define HTTPPLUGINUTILS_HPP

#include <cstring>
#include "PluginUtils.hh"
#include <davix.hpp>


namespace HttpUtils{



inline std::string protocolHttpNormalize(const std::string & url){
    if(url.compare(0,4,"http") == 0)
        return url;
    std::string res = url;
    std::string::iterator it = std::find(res.begin(),res.end(), ':');
    if( it == res.end())
        return res;
    if( it > res.begin() && *(it-1) == 's')
        res.replace(res.begin(), it, "https");
    else
        res.replace(res.begin(), it, "http");
    return res;
}



}


#endif // HTTPPLUGINUTILS_HPP

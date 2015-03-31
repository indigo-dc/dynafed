#ifndef HTTPPLUGINUTILS_HPP
#define HTTPPLUGINUTILS_HPP

#include <cstring>
#include "PluginUtils.hh"
#include <davix.hpp>
#include <algorithm>
#include <boost/bind.hpp>


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



inline bool compare_prec_char(char* prec, char b){
    if(b == *prec && b == '/')
           return true;
       *prec = b;
       return false;
}

// remove all duplicate // in url
inline void pathHttpNomalize(std::string & url){
    std::string::iterator it;
    std::string::iterator it2;
    it2 = std::find(url.begin(), url.end(), '?'); 
    if( ( it = std::find(url.begin(), it2, ':') ) != it2){
        it+=3;
        char c='\0';
        url.erase(std::remove_if(it, it2, boost::bind(compare_prec_char, &c, _1)), it2);
    }

}



}


#endif // HTTPPLUGINUTILS_HPP

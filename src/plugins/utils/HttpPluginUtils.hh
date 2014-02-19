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
    if( ( it = std::find(url.begin(), url.end(), ':') ) != url.end()){
        it+=3;
        char c='\0';
        std::string::iterator new_end= std::remove_if(it, url.end(), boost::bind(compare_prec_char, &c, _1));
        url.resize(new_end - url.begin());
    }

}



}


#endif // HTTPPLUGINUTILS_HPP

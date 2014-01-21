#ifndef PLUGINUTILS_HPP
#define PLUGINUTILS_HPP

#include <string>
#include <sstream>

#include <Config.hh>

template<typename T>
T pluginGetParam(const std::string & prefix, const std::string & key, const T & default_value = T()){
    return default_value;
}

// string params
template<>
inline bool pluginGetParam<bool>(const std::string & prefix, const std::string & key, const bool & default_value){
    std::ostringstream ss;
    ss << prefix << "." << key;
    return Config::GetInstance()->GetBool( ss.str(), default_value);
}

// boolean
template<>
inline std::string pluginGetParam<std::string>(const std::string & prefix, const std::string & key, const std::string &  default_value){
    std::ostringstream ss;
    ss << prefix << "." << key;
    return Config::GetInstance()->GetString( ss.str(), default_value);
}

// Long param
template<>
inline long pluginGetParam<long>(const std::string & prefix, const std::string & key, const long &  default_value){
    std::ostringstream ss;
    ss << prefix << "." << key;
    return Config::GetInstance()->GetLong( ss.str(), default_value);
}



#endif // PLUGINUTILS_HPP

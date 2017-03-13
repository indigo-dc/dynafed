#ifndef PLUGINUTILS_HPP
#define PLUGINUTILS_HPP


/*
 *  Copyright (c) CERN 2013
 *
 *  Copyright (c) Members of the EMI Collaboration. 2011-2013
 *  See  http://www.eu-emi.eu/partners for details on the copyright
 *  holders.
 *
 *  Licensed under the Apache License, Version 2.0
 *  See the LICENSE file for further information
 * 
 */


#include <string>
#include <sstream>

#include <UgrConfig.hh>

template<typename T>
T pluginGetParam(const std::string & prefix, const std::string & key, const T & default_value = T()){
    return default_value;
}

// string params
template<>
inline bool pluginGetParam<bool>(const std::string & prefix, const std::string & key, const bool & default_value){
    std::ostringstream ss;
    ss << prefix << "." << key;
    return UgrConfig::GetInstance()->GetBool( ss.str(), default_value);
}

// boolean
template<>
inline std::string pluginGetParam<std::string>(const std::string & prefix, const std::string & key, const std::string &  default_value){
    std::ostringstream ss;
    ss << prefix << "." << key;
    return UgrConfig::GetInstance()->GetString( ss.str(), default_value);
}

// Long param
template<>
inline long pluginGetParam<long>(const std::string & prefix, const std::string & key, const long &  default_value){
    std::ostringstream ss;
    ss << prefix << "." << key;
    return UgrConfig::GetInstance()->GetLong( ss.str(), default_value);
}



#endif // PLUGINUTILS_HPP

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


/** @file   SimpleDebug.hh
 * @brief  Definitions to use the dmlite logger
 * @author Fabrizio Furano
 * @date   Oct 2010
 */

#ifndef SIMPLEDEBUG_HH
#define SIMPLEDEBUG_HH

#include "UgrLogger.hh"

extern UgrLogger::bitmask ugrlogmask;
extern UgrLogger::component ugrlogname;


///
/// \brief The UgrSCode class
///
/// Contain status code and error report
class UgrCode{
public:
    enum Status{
        Ok=0,
        FileNotFound,
        PermissionDenied,
        OverwriteNotAllowed,
    };

    UgrCode() : code_(Ok), msg_() {}
    UgrCode(Status code, const std::string & msg) : code_(code), msg_(msg){}

    inline Status getCode() const{
        return code_;
    }

    inline const std::string & getString(){
        return msg_;
    }

    bool isOK(){
        return code_ == Ok;
    }

private:
    Status code_;
    std::string msg_;
};



// -------------------------------------------------------
// Some macros to make the logging more comfortable to use
// The idea is to use the comfortable cout-like syntax in the
//  error logging, in order to eliminate the annoying bugs due to a
//  quick and dirty parameter type conversion.
// By exploiting the cout-like syntax we eliminate this danger.
//
// The log level is a short integer, representing an increasing importance
//  of the message to log.

#define DebugLevel() UgrLogger::get()->getLevel()
#define DebugSetLevel(l) UgrLogger::get()->setLevel((UgrLogger::Level)l)

// Information logging
// The output of these strings is subject to the log level that was set
// In general:
//  lvl is a log level. This message will be printed only if the global logging level
//    is higher than lvl
//  where is a string. It's a good idea to put here the name of the function we're in
//    when requesting the logging
//  what is a string that describes the info to log.
//
#define Info(lvl, where, what) do {                                											\
	if (UgrLogger::get()->getLevel() >= lvl && UgrLogger::get()->isLogged(ugrlogmask)) 	\
	{    																	\
		std::ostringstream outs;                                   			\
		outs << ugrlogname << " " << where << " " << __func__ << " : " << what;                      			\
		UgrLogger::get()->log((UgrLogger::Level)lvl, outs.str());    				\
	}                                                             			\
}while(0)

// Error logging
// These error messages are printed regardless of the current local logging level
#define Error(where, what) do{                                											\
		std::ostringstream outs;                                   			\
		outs << ugrlogname << " " << where << " !! " << __func__ << " : " << what;                      			\
		UgrLogger::get()->log((UgrLogger::Level)0, outs.str());    				\
}while(0)



#endif

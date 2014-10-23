/** @file   SimpleDebug.hh
 * @brief  Definitions to use the dmlite logger
 * @author Fabrizio Furano
 * @date   Oct 2010
 */

#ifndef SIMPLEDEBUG_HH
#define SIMPLEDEBUG_HH

#include "dmlite/cpp/utils/logger.h"

extern Logger::bitmask ugrlogmask;
extern Logger::component ugrlogname;


// -------------------------------------------------------
// Some macros to make the logging more comfortable to use
// The idea is to use the comfortable cout-like syntax in the
//  error logging, in order to eliminate the annoying bugs due to a
//  quick and dirty parameter type conversion.
// By exploiting the cout-like syntax we eliminate this danger.
//
// The log level is a short integer, representing an increasing importance
//  of the message to log.

#define DebugLevel() Logger::get()->getLevel()
#define DebugSetLevel(l) Logger::get()->setLevel((Logger::Level)l)

// Information logging
// The output of these strings is subject to the log level that was set
// In general:
//  lvl is a log level. This message will be printed only if the global logging level
//    is higher than lvl
//  where is a string. It's a good idea to put here the name of the function we're in
//    when requesting the logging
//  what is a string that describes the info to log.
//
#define Info(lvl, where, what) Log(lvl, ugrlogmask, where, what)


// Error logging
// These error messages are printed regardless of the current local logging level
#define Error(where, what) Err(where, what)


#endif

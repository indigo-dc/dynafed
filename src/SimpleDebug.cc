/** @file   SimpleDebug.hh
 * @brief  Singleton used to handle the debug level and the log output plus some useful and handy macros.
 * @author Fabrizio Furano
 * @date   Oct 2010
 */

#include "SimpleDebug.hh"
#include <syslog.h>
#include <stdlib.h>
#include <iostream>


SimpleDebug *SimpleDebug::fgInstance = 0;


SimpleDebug* SimpleDebug::Instance() {
   // Create unique instance

  if (!fgInstance) {
     fgInstance = new SimpleDebug;
     if (!fgInstance) {
       abort();
     }
   }
   return fgInstance;
}


SimpleDebug::SimpleDebug() {
   // Constructor... initialize the syslog stuff
   openlog(syslogIdent.c_str(), LOG_PID | LOG_NDELAY, LOG_USER);
   stderr_print = true;

   fDbgLevel = kLOW;
}


SimpleDebug::~SimpleDebug() {
   // Destructor
   closelog();

   delete fgInstance;
   fgInstance = 0;
}


void SimpleDebug::DoLog(const char * s) {

   syslog(LOG_DEBUG, "UGR : %s", s);
   if(stderr_print)
       std::cerr << s << std::endl;
}



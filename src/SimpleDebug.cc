/* SimpleDebug
 * Singleton used to handle the debug level and the log output
 * plus some useful and handy macros.
 *
 * This implementation uses syslog to send the messages, and it is designed
 *  to handle logging for a single process. I.e. no mask-like
 *  behavior for different log sources was implemented here.
 *
 *
 * by Fabrizio Furano, CERN, Oct 2010
 */

#include "SimpleDebug.hh"
#include <syslog.h>
#include <stdlib.h>
#include <iostream>

SimpleDebug *SimpleDebug::fgInstance = 0;

//_____________________________________________________________________________
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

//_____________________________________________________________________________
SimpleDebug::SimpleDebug() {
   // Constructor... initialize the syslog stuff
   openlog(syslogIdent.c_str(), LOG_CONS | LOG_PID | LOG_PERROR, LOG_USER);

   fDbgLevel = kLOW;
}

//_____________________________________________________________________________
SimpleDebug::~SimpleDebug() {
   // Destructor
   closelog();

   delete fgInstance;
   fgInstance = 0;
}


void SimpleDebug::DoLog(const char * s) {

   syslog(LOG_DEBUG, "%s", s);
   //std::cout << s;
}

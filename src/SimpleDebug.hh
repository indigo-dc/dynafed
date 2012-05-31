#ifndef SIMPLEDEBUG_HH
#define SIMPLEDEBUG_HH

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


// The singleton instance
class SimpleDebug;

#include <sstream>

// -------------------------------------------------------
// Some macros to make the logging more comfortable to use
// The idea is to use the comfortable cout-like syntax in the
//  error logging, in order to eliminate the annoying bugs due to a
//  quick and dirty parameter type conversion.
// By exploiting the cout-like syntax we eliminate this danger.
//
// The log level is a short integer, representing an increasing importance
//  of the message to log.

#define DebugLevel() SimpleDebug::Instance()->GetLevel()
#define DebugSetLevel(l) SimpleDebug::Instance()->SetLevel(l)

// Information logging
// The output of these strings is subject to the log level that was set
// In general:
//  lvl is a log level. This message will be printed only if the global logging level
//    is higher than lvl
//  where is a string. It's a good idea to put here the name of the function we're in
//    when requesting the logging
//  what is a string that describes the info to log.
//
#define Info(lvl, where, what) {                                \
  SimpleDebug::Instance()->Lock();                              \
  if (SimpleDebug::GetLevel() >= lvl) {             \
     std::ostringstream outs;                                   \
     outs << where << ": " << what;                             \
     SimpleDebug::Instance()->TraceStream((short)lvl, outs);    \
  }                                                             \
  SimpleDebug::Instance()->Unlock();                            \
}                                                               \

                               
// Error logging
// These error messages are printed regardless of the current local logging level
#define Error(where, what) {                                                       \
  std::ostringstream outs;                                                         \
  outs << where << ": " << what;                                                   \
  SimpleDebug::Instance()->TraceStream((short)SimpleDebug::kSILENT, outs);         \
}                                                                                  \


// --------------------------------------------------------------


class SimpleDebug {
 private:
   short        fDbgLevel;
   std::string  syslogIdent;

   void DoLog(const char * s);



 protected:
   SimpleDebug();
   ~SimpleDebug();

  static SimpleDebug *fgInstance;

 public:

   // An helper enum to help following a coherent debug level schema
   // This is not enforced through strong typing,
   //  but it's anyway a good idea to comply to it
   enum {
      kSILENT     = 0,      // Log absolutely nothing

      kLOW        = 1,      // Log very basic things, startup and just very high level funcs
                            //  typical example: "Connected to XYZ"

      kMEDIUM     = 2,      // Log more things, important to understand what's going on
                            //  typical example: "Trying to connect to XYZ"

      kHIGH       = 3,      // Very verbose, log every significant function call

      kHIGHEST    = 4       // Log everything that can be logged, protocol dumps, etc.
   };

   static short        GetLevel() {
     return Instance()->fDbgLevel;
   }

   static SimpleDebug *Instance();
  static void Set(SimpleDebug *i) {
    fgInstance = i;
  }

   void SetIdent(const char *ident) {
      syslogIdent = ident;
   }

   void SetLevel(int l) {
      fDbgLevel = l;
   }

   void Lock() {};
   void Unlock() {};

   inline void TraceStream(short DbgLvl, std::ostringstream &s) {

      if (DbgLvl <= GetLevel())
         DoLog(s.str().c_str());

      s.str("");
   }

   inline void TraceString(short DbgLvl, char * s) {

      if (DbgLvl <= GetLevel())
         DoLog(s);
   }

};

#endif

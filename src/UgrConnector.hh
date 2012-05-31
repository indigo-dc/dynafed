#ifndef UGRCONNECTOR_HH
#define UGRCONNECTOR_HH

/* UgrConnector
 * Base class that gives the functionalities of a redirector
 *
 *
 * by Fabrizio Furano, CERN, Jul 2011
 */

#include "SimpleDebug.hh"
#include "Config.hh"
#include "LocationInfo.hh"
#include "LocationInfoHandler.hh"
#include "HostsInfoHandler.hh"
#include "LocationPlugin.hh"


#include <string>



// The class that allows to interact with the system
class UgrConnector {
private:
    // The thread that ticks
    boost::thread ticker;

protected:

   // This holds in memory at least the entries that are being processed
   // Eventually it may grow or demand a more scalable caching to an external entity
   // This has to be mutex-protected
   LocationInfoHandler locHandler;

   // This handles the information that we have about a host that participates to the thing
   // Having no info about a host is technically allowed
   // At app level, this will probably be forbidden
   // This has to be mutex-protected
   HostsInfoHandler hostHandler;

   // The location plugins that we have loaded
   //
   // E.g. SEMsg, NativeLFC, WhateverDB, WhateverMessaging, VanillaHTTP
   // Each plugin is able to modify the info in LocHandler and HostHandler, asynchronously
   // When a location process is started, all the plugins are triggered in parallel
   std::vector<LocationPlugin *> locPlugins;

   // Start the async stat process
   // In practice, trigger all the location plugins, possibly together,
   // so they act concurrently
   int do_Stat(UgrFileInfo *fi);
   // Waits max a number of seconds for a locate process to be complete
   int do_waitStat(UgrFileInfo *fi, int tmout=5);

   // Start the async location process
   // In practice, trigger all the location plugins, possibly together,
   // so they act concurrently
   int do_Locate(UgrFileInfo *fi);
   // Waits max a number of seconds for a locate process to be complete
   int do_waitLocate(UgrFileInfo *fi, int tmout=5);

   // Start the async listing process
   // In practice, trigger all the location plugins, possibly together,
   // so they act concurrently
   int do_List(UgrFileInfo *fi);
   // Waits max a number of seconds for a list process to be complete
   int do_waitList(UgrFileInfo *fi, int tmout=5);

   // Invoked by a thread, gives life to the object
   virtual void tick(int parm);


   unsigned int ticktime;
public:

    UgrConnector(): ticker( boost::bind( &UgrConnector::tick, this, 0 )) {
        // Get the tick pace from the config
        ticktime = CFG->GetLong("glb.tick", 1);

    };
    
   virtual ~UgrConnector();

   int init(char *cfgfile = 0);

   // Returns a pointer to the item, after having populated
   // the list of the available replicas for the given lfn.
   // Sync API that eventually launches a search
   // The nfo instance is returned in locked state, only with the purpose
   // of copying the values out. The caller
   // must release it as soon as possible
   virtual int locate(std::string &lfn, UgrFileInfo **nfo);

   // Returns a pointer to the item with the list of the content of the given lfn (ls).
   // Waits for some time that at least nitemswait items have arrived, ev returns TIMEOUT
   // If the search process terminates and there are no (more) items, returns OK
   // The nfo instance is returned in locked state, only with the purpose
   // of copying the values out. The caller
   // must release it as soon as possible
   virtual int list(std::string &lfn, UgrFileInfo **nfo, int nitemswait = 0);

   // Returns a pointer to the item, after having made sure that
   // its stat information is populated. Eventually populate it before returning.
   // The nfo instance is returned in locked state, only with the purpose
   // of copying the values out. The caller
   // must release it as soon as possible
   virtual int stat(std::string &lfn, UgrFileInfo **nfo);


};


#endif

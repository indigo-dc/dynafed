#ifndef LOCATIONINFOHANDLER_HH
#define LOCATIONINFOHANDLER_HH

/* LocationInfoHandler
 * Handling of the info that is kept per each file
 *
 *
 * by Fabrizio Furano, CERN, Oct 2011
 */

#include "Config.hh"
#include "LocationInfo.hh"

#include <string>
#include <map>
#include <boost/thread.hpp>
#include <boost/bimap.hpp>


// This class acts like a repository of file locations/information
// It may act as a first level cache, thus implementing a insert/purge mechanism
// It may also interface with a secondary (big) cache, thus adding a retrieve mechanism
// These behaviors are implemented in the Tick() method, that has to be called at a
// regular pace
class LocationInfoHandler: public boost::mutex {
private:
    unsigned long long lrutick;

    void deleteLRUitem();
public:

   LocationInfoHandler(): lrutick(0) {
   };

   // Basically a map key->UgrFileInfo
   // where the key is generally a LFN

   typedef boost::bimap< time_t, std::string > lrudatarepo;
   typedef lrudatarepo::value_type lrudataitem;

   lrudatarepo lrudata;
   std::map< std::string, UgrFileInfo * > data;

   //
   // Helper primitives to operate on the list of file locations
   //

   // Get a pointer to a FileInfo, or create a new, pending one
   UgrFileInfo *getFileInfoOrCreateNewOne(std::string &lfn);

   void tick();
};



#endif


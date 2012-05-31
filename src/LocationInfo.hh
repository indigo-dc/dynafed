#ifndef LOCATIONINFO_HH
#define LOCATIONINFO_HH

/* LocationInfo
 * Handling of the info that is kept per each file
 *
 *
 * by Fabrizio Furano, CERN, Jul 2011
 */

#include "Config.hh"

#include <string>
#include <boost/thread.hpp>
#include "SimpleDebug.hh"
#include <boost/date_time/posix_time/posix_time.hpp>

// The information that we have about an (sub)item. Maybe a file or directory
// If it's a directory then this is a (relative) item of its listing
// If it's a file, this is the info about one of its replicas (full path)
// This can be used to build the full name of a file and fetch its info
//

class UgrFileItem {
public:
   // The item's name
   std::string name;
   
   // Some info about the location, e.g. galactic coordinates
   std::string location;
};
   
// This defines the information that is treated about a file
//
// Note that this object is lockable. It's supposed to be locked (if necessary)
// from outside its own code
class UgrFileInfo: public boost::mutex {
protected:
    boost::condition_variable condvar;
public:
   UgrFileInfo(std::string &lfn) {
      status_statinfo = NoInfo;
      status_locations = NoInfo;
      status_items = NoInfo;
      pending_statinfo = 0;
      pending_locations = 0;
      pending_items = 0;
      name = lfn;

      lastupdtime = time(0);
      lastupdreqtime = time(0);
   }

   // The filename this record refers to (the lfn)
   // associated to its stat information
   std::string name;

   enum InfoStatus {
      NoInfo = -1,
      Ok,
      NotFound,
      InProgress
   };
      
   // If a request is in progress, the responsibility of
   // tracking it is of the plugin(s) that are treating it
   //
   InfoStatus status_statinfo;
   InfoStatus status_locations;
   InfoStatus status_items;

   // 
   // Any number > 0 is the count of the plugins that are treating
   // this fileinfo
   int pending_statinfo;
   int pending_locations;
   int pending_items;

 
   InfoStatus getStatStatus() {
       // To stat successfully a file we just need one plugin to answer positively
       // Hence, if we have the stat info here, we just return Ok, regardless
       // of how many plugins are still active 
       if (!status_statinfo) return Ok;

       // If we have no stat info, then the file was not found or it's early to tell
       // Hence the info is inprogress if there are plugins that are still active
       if (pending_statinfo > 0) return InProgress;
      
       return status_statinfo;
   }

   InfoStatus getLocationStatus() {
       // In the case of a pending op that tries to find all the locations, we need to
       // get the response from all the plugins
       // Hence, this info is inprogress if there are still plugins that are active on it
       if (pending_locations > 0) return InProgress;

       if (status_locations == Ok) return Ok;
       return status_locations;
   }

   InfoStatus getItemsStatus() {
       if (pending_items > 0) return InProgress;

       if (status_items == Ok) return Ok;
       return status_items;
   }

   // Builds a summary
   // If some info part is pending then the whole thing is pending
   InfoStatus getInfoStatus() {
      if ((pending_statinfo > 0) ||
          (pending_locations > 0) ||
          (pending_items > 0))
         return InProgress;

      if ((status_statinfo == Ok) ||
          (status_locations == Ok) ||
          (status_items == Ok))
         return Ok;

      return NoInfo;
   };

   // Called by plugins when they start a search, to add 1 to the pending counter
   void notifyStatPending() {
      if (pending_statinfo >= 0) {
         // Set the file status to pending with respect to the stat op
         pending_statinfo++;
      }
   }

   // Called by plugins when they end a search, to subtract 1 from the pending counter
   void notifyStatNotPending() {
      const char *fname = "UgrFileInfo::notifyStatNotPending";
      if (pending_statinfo > 0) {
         // Decrease the pending count with respect to the stat op
         pending_statinfo--;
      }
      else
         Error(fname, "The fileinfo seemed not to be pending?!?");

      signalSomeUpdate();
   }


      // Called by plugins when they start a search, to add 1 to the pending counter
   void notifyLocationPending() {
      if (pending_locations >= 0) {
         // Set the file status to pending with respect to the stat op
         pending_locations++;
      }
   }

   // Called by plugins when they end a search, to subtract 1 from the pending counter
   void notifyLocationNotPending() {
      const char *fname = "UgrFileInfo::notifyLocationNotPending";
      if (pending_locations > 0) {
         // Decrease the pending count with respect to the stat op
         pending_locations--;
      }
      else
         Error(fname, "The fileinfo seemed not to be pending?!?");

      signalSomeUpdate();
   }

      // Called by plugins when they start a search, to add 1 to the pending counter
   void notifyItemsPending() {
      if (pending_items >= 0) {
         // Set the file status to pending with respect to the stat op
         pending_items++;
      }
   }

   // Called by plugins when they end a search, to subtract 1 from the pending counter
   void notifyItemsNotPending() {
      const char *fname = "UgrFileInfo::notifyItemsNotPending";
      if (pending_items > 0) {
         // Decrease the pending count with respect to the stat op
         pending_items--;
      }
      else
         Error(fname, "The fileinfo seemed not to be pending?!?");

      signalSomeUpdate();
   }


   int unixflags;
   long long size;

   std::string owner;
   std::string group;

   // The list of the replicas (if this is a file), eventually partial if in pending state
   // Just the keys, in this small structure
   std::vector<UgrFileItem *> subitems;

   // The last time there was a request to gather info about this entry
   time_t lastupdreqtime;

   // The last time there was an update to this entry
   time_t lastupdtime;

   // We will like to be able to encode this info to a string, e.g. for external caching purposes
   int encodeToString(std::string &str) { str = ""; return 0; };
   int decodeFromString(std::string &str) { str = ""; return 0; };

   // Selects the replica that looks best for the given client. Here we don't make assumptions
   // on the method that we apply to choose one, since it could be implemented as a plugin
   int getBestReplicaIdx(std::string &clientlocation);


   // Wait until any notification update comes
   // Useful to recheck if what came is what we were waiting for
   // 0 if notif received, nonzero if tmout
   int waitForSomeUpdate(boost::unique_lock<boost::mutex> &l, int sectmout);
   int waitStat(boost::unique_lock<boost::mutex> &l, int sectmout);
   int waitLocations(boost::unique_lock<boost::mutex> &l, int sectmout);
   int waitItems(boost::unique_lock<boost::mutex> &l, int sectmout);
   
   // Signal that something changed
   int signalSomeUpdate();


   // Useful for debugging
   void print(std::ostream &out);
      
};



#endif


/** 
 * @file   UgrLocPlugin_dav.cc
 * @brief  Plugin that talks to any Webdav compatible endpoint
 * @author Devresse Adrien
 * @date   Feb 2012
 */

#include <davix_cpp.hpp>
 
#include "UgrLocPlugin_dav.hh"
#include "../../PluginLoader.hh"
#include <time.h>




using namespace boost;
using namespace std;


// ------------------------------------------------------------------------------------
// Plugin-related stuff
// ------------------------------------------------------------------------------------


// The hook function. GetLocationPluginClass must be given the name of this function
// for the plugin to be loaded
extern "C" LocationPlugin *GetLocationPlugin(GetLocationPluginArgs) {
    return (LocationPlugin *)new UgrLocPlugin_dav(dbginstance, cfginstance, parms);
}


void UgrLocPlugin_dav::runsearch(struct worktoken *op, int myidx){
	struct stat st;
	static const char * fname = "UgrLocPlugin_dav::runsearch";
	std::string cannonical_name = base_url;
	bool bad_answer=true;
	Davix::DAVIX_DIR* d=NULL;


    // We act using the identity of this service, hence we don't need to invoke
    // getIdMap/setUserblahblah
	if(op == NULL || op->fi == NULL){
	   LocPluginLogInfoThr(SimpleDebug::kHIGH, fname, " Bad request Handle : FATAL");	
	   return;
	}
	cannonical_name += "/";
	cannonical_name += op->fi->name;

	try {
		switch (op->wop) {

			case LocationPlugin::wop_Stat:
				LocPluginLogInfoThr(SimpleDebug::kHIGH, fname, "invoking Stat(" << cannonical_name << ")");
				dav_core->stat(cannonical_name, &st);			
				break;

			case LocationPlugin::wop_Locate:
				LocPluginLogInfoThr(SimpleDebug::kHIGH, fname, "invoking getReplicas(" << cannonical_name << ")");
				// do nothing for now
				return;
				break;

			case LocationPlugin::wop_List:
				LocPluginLogInfoThr(SimpleDebug::kHIGH, fname, "invoking openDir(" << cannonical_name << ")");
				d = dav_core->opendir(cannonical_name);
				break;

			default:
				break;
		}
		bad_answer = false; // reach here -> request complete 
	} catch (Glib::Error & e) {
		LocPluginLogInfoThr(SimpleDebug::kHIGH, fname, " UgrDav plugin request Error : " << " name: " <<  g_quark_to_string(e.domain()) << " Catched exception: " << e.code() << " what: " << e.what());
		if(d)
			dav_core->closedir(d);
	} catch(std::exception & e){
		LocPluginLogErr(fname, " UgrDav plugin request Error : Unexcepted Error, FATAL  what: " << e.what());		
	} catch(...){
		LocPluginLogErr(fname, " UgrDav plugin request Error: Unknow Error Fatal  ");			
	}
	
	LocPluginLogInfoThr(SimpleDebug::kMEDIUM, fname, "Worker: inserting data for " << op->fi->name);


	UgrFileItem it;

	// Lock the file instance
	unique_lock<mutex> l(*(op->fi));

	op->fi->lastupdtime = time(0);

	if(bad_answer == false){
		switch (op->wop) {


			case LocationPlugin::wop_Stat:
					LocPluginLogInfoThr(SimpleDebug::kHIGHEST, fname, "Worker: stat info:" << st.st_size << " " << st.st_mode);
					op->fi->size = st.st_size;
					//op->fi->lastupdreqtime = st.st_mtime;
					op->fi->status_statinfo = UgrFileInfo::Ok;
					op->fi->unixflags = st.st_mode;
				break;

			case LocationPlugin::wop_Locate:
				// disabled
				return;
				break;

			case LocationPlugin::wop_List:
				dirent * dent;
				while((dent = dav_core->readdir(d)) != NULL){
					UgrFileItem it;					
					LocPluginLogInfoThr(SimpleDebug::kHIGHEST, fname, "Worker: Inserting list " << dent->d_name);
					it.name = std::string(dent->d_name);
					it.location.clear();					
					op->fi->subitems.insert(it);	
				}
				dav_core->closedir(d);
				op->fi->status_statinfo = UgrFileInfo::Ok;                
				op->fi->status_items = UgrFileInfo::Ok;                   
				break;
				
			default:
				break;
		}


		// We have modified the data, hence set the dirty flag
		op->fi->dirty = true;
	}
	// Anyway the notification has to be correct, not redundant
	switch (op->wop) {

		case LocationPlugin::wop_Stat:
			LocPluginLogInfoThr(SimpleDebug::kHIGHEST, fname, "Notify End Stat");  		
			op->fi->notifyStatNotPending();
			break;

		case LocationPlugin::wop_Locate:
			LocPluginLogInfoThr(SimpleDebug::kHIGHEST, fname, "Notify End Locate");     		
			op->fi->notifyLocationNotPending();
			break;

		case LocationPlugin::wop_List:
			LocPluginLogInfoThr(SimpleDebug::kHIGHEST, fname, "Notify End Listdir");                     
			op->fi->notifyItemsNotPending();
			break;

		default:
			break;
	}

}

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

/** 
 * @file   UgrLocPlugin_dav.cc
 * @brief  Plugin that talks to any Webdav compatible endpoint
 * @author Devresse Adrien
 * @date   Feb 2012
 */


#include "UgrLocPlugin_dav.hh"
#include "../../PluginLoader.hh"
#include "../../ExtCacheHandler.hh"
#include "../utils/HttpPluginUtils.hh"
#include <time.h>
#include "libs/time_utils.h"

using namespace boost;
using namespace std;


// ------------------------------------------------------------------------------------
// Plugin-related stuff
// ------------------------------------------------------------------------------------



UgrLocPlugin_dav::UgrLocPlugin_dav(UgrConnector & c, std::vector<std::string> & parms) :
    UgrLocPlugin_http(c, parms) {
    Info(UgrLogger::Lvl1, "UgrLocPlugin_[http/dav]", "UgrLocPlugin_[http/dav]: WebDav ENABLED");
    params.setProtocol(Davix::RequestProtocol::Webdav);
}


void UgrLocPlugin_dav::runsearch(struct worktoken *op, int myidx) {
    struct stat st;
    Davix::DavixError * tmp_err = NULL;
    static const char * fname = "UgrLocPlugin_dav::runsearch";
    std::string canonical_name(base_url_endpoint.getString());
    std::vector<Davix::File> replica_vec;
    std::string xname;
    bool bad_answer = true;
    DAVIX_DIR* d = NULL;
    bool listerror = false;

    // We act using the identity of this service, hence we don't need to invoke
    // getIdMap/setUserblahblah
    if (op == NULL || op->fi == NULL) {
        Error(fname, " Bad request Handle : search aborted");
        return;
    }

    if( doParentQueryCheck(op->fi->name, op, myidx)){
        return;
    }

    // Doublecheck if we are disabled. If so, quickly close the pending requests
    bool imdisabled = !availInfo.isOK();
    
    if (op->wop == wop_CheckReplica) {

        // Do the default name translation for this plugin (prefix xlation)
        if (imdisabled || doNameXlation(op->repl, xname, op->wop, op->altpfx)) {
            unique_lock<mutex> l(*(op->fi));
            op->fi->notifyLocationNotPending();
            return;
        }// Then prepend the URL prefix

    } else {

        // Do the default name translation for this plugin (prefix xlation)
        if (imdisabled || doNameXlation(op->fi->name, xname, op->wop, op->altpfx)) {
            unique_lock<mutex> l(*(op->fi));
            switch (op->wop) {
                case LocationPlugin::wop_Stat:
		    LocPluginLogInfoThr(UgrLogger::Lvl3, fname, "Short-circuit on Stat() " << canonical_name << ")");
                    op->fi->notifyStatNotPending();
                    break;

                case LocationPlugin::wop_Locate:
		    LocPluginLogInfoThr(UgrLogger::Lvl3, fname, "Short-circuit on Locate() " << canonical_name << ")");
                    op->fi->notifyLocationNotPending();
                    break;

                case LocationPlugin::wop_List:
		    LocPluginLogInfoThr(UgrLogger::Lvl3, fname, "Short-circuit on List() " << canonical_name << ")");
                    op->fi->notifyItemsNotPending();
                    break;

                default:
                    break;
            }
            return;
        }
    }

    canonical_name.append(xname);

    memset(&st, 0, sizeof (st));

    switch (op->wop) {

        case LocationPlugin::wop_Stat:
            LocPluginLogInfoThr(UgrLogger::Lvl3, fname, "invoking davix_Stat(" << canonical_name << ")");
            pos.stat(&params, canonical_name, &st, &tmp_err);
            break;

        case LocationPlugin::wop_Locate:
            LocPluginLogInfoThr(UgrLogger::Lvl3, fname, "invoking Locate(" << canonical_name << ")");
            if(flags & UGR_HTTP_FLAG_METALINK){
                LocPluginLogInfoThr(UgrLogger::Lvl3, fname, "invoking Locate with metalink support");
                Davix::File f(dav_core, canonical_name);
                replica_vec = f.getReplicas(&params, &tmp_err);
                if(tmp_err){
                    LocPluginLogInfoThr(UgrLogger::Lvl3, fname, "Impossible to use Metalink, code " << ((int)tmp_err->getStatus()) << " error "<< tmp_err->getErrMsg());
                }
            }

            if( (flags & UGR_HTTP_FLAG_METALINK) == false || tmp_err != NULL){
                Davix::DavixError::clearError(&tmp_err);
                if(pos.stat(&params, canonical_name, &st, &tmp_err) >=0)
                    replica_vec.push_back(Davix::File(dav_core, canonical_name));
            }
            break;
        case LocationPlugin::wop_CheckReplica:
            LocPluginLogInfoThr(UgrLogger::Lvl3, fname, "invoking CheckReplica(" << canonical_name << ")");
            pos.stat(&params, canonical_name, &st, &tmp_err);
            break;
        case LocationPlugin::wop_List:
            LocPluginLogInfoThr(UgrLogger::Lvl3, fname, " invoking davix_openDir(" << canonical_name << ")");
            d = pos.opendirpp(&params, canonical_name, &tmp_err);
            // if reach here -> valid opendir -> specify file as well
            if(d)
                op->fi->unixflags |= S_IFDIR;
            break;

        default:
            break;
    }

    if (!tmp_err) {
        bad_answer = false; // reach here -> request complete

        // Everything OK... keep the online status

        PluginEndpointStatus st;
        availInfo.getStatus(st);
        st.lastcheck = time(0);
        if (st.state == PLUGIN_ENDPOINT_ONLINE)
            availInfo.setStatus(st, true, (char *) name.c_str());



    } else {

        // Connection problem... it's offline for sure!
        if ((tmp_err->getStatus() == Davix::StatusCode::ConnectionTimeout) ||
                (tmp_err->getStatus() == Davix::StatusCode::OperationTimeout)) {
            PluginEndpointStatus st;
            availInfo.getStatus(st);
            st.lastcheck = time(0);
	    
	    if (tmp_err->getStatus() == Davix::StatusCode::ConnectionTimeout)  
	      st.explanation = "Connection timeout: " + tmp_err->getErrMsg();
	    else
	      st.explanation = "Operation timeout: " + tmp_err->getErrMsg();
	    
            st.state = PLUGIN_ENDPOINT_OFFLINE;
            availInfo.setStatus(st, true, (char *) name.c_str());
            // Propagate this fresh result to the extcache
            if (extCache)
                extCache->putEndpointStatus(&st, name);
        }


        LocPluginLogInfoThr(UgrLogger::Lvl3, fname, " UgrDav plugin request Error : " << ((int) tmp_err->getStatus()) << " errMsg: " << tmp_err->getErrMsg());
    }


    op->fi->lastupdtime = time(0);

    if (bad_answer == false) {
        LocPluginLogInfoThr(UgrLogger::Lvl2, fname, "Worker: inserting data for " << op->fi->name);
        op->fi->setPluginID(getID());

        switch (op->wop) {

            case LocationPlugin::wop_Stat:
                LocPluginLogInfoThr(UgrLogger::Lvl4, fname, "Worker: stat info:" << st.st_size << " " << st.st_mode);
                op->fi->takeStat(st);
                break;

            case LocationPlugin::wop_Locate:
            {
                for(std::vector<Davix::File>::iterator it = replica_vec.begin(); it != replica_vec.end(); ++it){
                    UgrFileItem_replica itr;
                    itr.name = HttpUtils::protocolHttpNormalize(it->getUri().getString());
                    HttpUtils::pathHttpNomalize(itr.name);
                    itr.pluginID = getID();
                    LocPluginLogInfoThr(UgrLogger::Lvl4, fname, "Worker: Inserting replicas " << itr.name);

                    // We have modified the data, hence set the dirty flag
                    op->fi->dirtyitems = true;

                    if (!isReplicaXlator()) {
                        op->fi->addReplica(itr);
                    } else {
                        req_checkreplica(op->fi, itr.name);
                    }
                }
                break;
            }
            case LocationPlugin::wop_CheckReplica:
            {
                UgrFileItem_replica itr;
                itr.name = canonical_name;

                itr.pluginID = getID();
                LocPluginLogInfoThr(UgrLogger::Lvl4, fname, "Worker: Inserting replicas " << itr.name);

                // We have modified the data, hence set the dirty flag
                op->fi->dirtyitems = true;

                op->fi->addReplica(itr);

                break;
            }
            case LocationPlugin::wop_List:
            {
                dirent * dent;
                long cnt = 0;
                struct stat st2;
                while ((dent = pos.readdirpp(d, &st2, &tmp_err)) != NULL) {
                    UgrFileItem it;
                    {
                        unique_lock<mutex> l(*(op->fi));

                        // We have modified the data, hence set the dirty flag
                        op->fi->dirtyitems = true;

                        if (cnt++ > CFG->GetLong("glb.maxlistitems", 2000)) {
                            LocPluginLogInfoThr(UgrLogger::Lvl2, fname, "Setting as non listable. cnt=" << cnt);
                            listerror = true;
                            op->fi->subdirs.clear();
                            break;
                        }

                        // create new items
                        LocPluginLogInfoThr(UgrLogger::Lvl4, fname, "Worker: Inserting list " << dent->d_name);
                        it.name = std::string(dent->d_name);
                        it.location.clear();

                        // populate answer
                        op->fi->subdirs.insert(it);
                    }

                    // add childrens
                    string child = op->fi->name;
                    if (child[child.length() - 1] != '/')
                        child = child + "/";
                    child = child + it.name;

                    LocPluginLogInfoThr(UgrLogger::Lvl4, fname,
                            "Worker: Inserting readdirpp stat info for  " << child <<
                            ", flags " << st.st_mode << " size : " << st.st_size);
                    UgrFileInfo *fi = op->handler->getFileInfoOrCreateNewOne(getConn(), child, false);

                    // If the entry was already in cache, don't overwrite
                    // This avoids a massive, potentially useless burst of writes to the 2nd level cache 
                    if (fi && (fi->status_statinfo != UgrFileInfo::Ok)) {
                        fi->takeStat(st2);
                    }
                }
                pos.closedirpp(d, NULL);


            }
                break;

            default:
                break;
        }


        if (tmp_err) {
            LocPluginLogInfoThr(UgrLogger::Lvl3, fname, " UgrDav plugin request Error : " << ((int) tmp_err->getStatus()) << " errMsg: " << tmp_err->getErrMsg());
        }



    }



    // Anyway the notification has to be correct, not redundant
    {
        // Lock the file instance
        unique_lock<mutex> l(*(op->fi));
        switch (op->wop) {

            case LocationPlugin::wop_Stat:
                LocPluginLogInfoThr(UgrLogger::Lvl4, fname, "Notify End Stat");
                op->fi->notifyStatNotPending();
                break;

            case LocationPlugin::wop_Locate:
            case LocationPlugin::wop_CheckReplica:
                LocPluginLogInfoThr(UgrLogger::Lvl4, fname, "Notify End Locate");
                op->fi->status_locations = UgrFileInfo::Ok;
                op->fi->notifyLocationNotPending();
                break;

            case LocationPlugin::wop_List:
                if (listerror) {
                    op->fi->status_items = UgrFileInfo::Error;

                } else
                    op->fi->status_items = UgrFileInfo::Ok;

                LocPluginLogInfoThr(UgrLogger::Lvl4, fname, "Notify End Listdir");
                op->fi->notifyItemsNotPending();
                break;

            default:
                break;
        }

    }

}

int UgrLocPlugin_dav::do_List(UgrFileInfo *fi, LocationInfoHandler *handler){
    return LocationPlugin::do_List(fi, handler);
}

void UgrLocPlugin_dav::do_Check(int myidx) {
    do_CheckInternal(myidx, "UgrLocPlugin_dav::do_Check");

}






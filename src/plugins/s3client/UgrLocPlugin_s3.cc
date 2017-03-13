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
 * @file   UgrLocPlugin_s3.cc
 * @brief  Plugin that talks to any Webdav compatible endpoint
 * @author Devresse Adrien
 * @date   Feb 2012
 */


#include "UgrLocPlugin_s3.hh"
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

UgrLocPlugin_s3::UgrLocPlugin_s3(UgrConnector & c, std::vector<std::string> & parms) :
    UgrLocPlugin_http(c, parms) {
    Info(UgrLogger::Lvl1, "UgrLocPlugin_[http/s3]", "UgrLocPlugin_[http/s3]: s3 ENABLED");
    configure_S3_parameter(getConfigPrefix() + name);
    params.setProtocol(Davix::RequestProtocol::AwsS3);
    checker_params.setProtocol(Davix::RequestProtocol::AwsS3);
}


void UgrLocPlugin_s3::runsearch(struct worktoken *op, int myidx) {
    struct stat st;
    Davix::DavixError * tmp_err = NULL;
    static const char * fname = "UgrLocPlugin_s3::runsearch";
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


    // s3 does not support //
    std::string::iterator it = xname.begin();
    while(*it == '/' && it < xname.end())
        it++;

    canonical_name.append("/");
    canonical_name.append(it, xname.end());

    memset(&st, 0, sizeof (st));

    switch (op->wop) {

        case LocationPlugin::wop_Stat:
            LocPluginLogInfoThr(UgrLogger::Lvl3, fname, "invoking davix_Stat(" << canonical_name << ")");
            pos.stat(&params, canonical_name, &st, &tmp_err);
            break;

        case LocationPlugin::wop_Locate:
            LocPluginLogInfoThr(UgrLogger::Lvl3, fname, "invoking Locate(" << canonical_name << ")");
            Davix::DavixError::clearError(&tmp_err);
            if(pos.stat(&params, canonical_name, &st, &tmp_err) >=0){
                
                Davix::HeaderVec vec;
                Davix::Uri replica = signURI(params, "GET", canonical_name, vec, signature_validity);
                LocPluginLogInfoThr(UgrLogger::Lvl3, fname, "Obtain signed replica " << replica);

                replica_vec.push_back(Davix::File(dav_core, replica.getString()));
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
                (tmp_err->getStatus() == Davix::StatusCode::ConnectionTimeout)) {
            PluginEndpointStatus st;
            availInfo.getStatus(st);
            st.lastcheck = time(0);
            st.state = PLUGIN_ENDPOINT_OFFLINE;
            availInfo.setStatus(st, true, (char *) name.c_str());
            // Propagate this fresh result to the extcache
            if (extCache)
                extCache->putEndpointStatus(&st, name);
        }


        LocPluginLogInfoThr(UgrLogger::Lvl3, fname, " Ugrs3 plugin request Error : " << ((int) tmp_err->getStatus()) << " errMsg: " << tmp_err->getErrMsg());
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

                        if (cnt++ > UgrCFG->GetLong("glb.maxlistitems", 2000)) {
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
            LocPluginLogInfoThr(UgrLogger::Lvl3, fname, " Ugrs3 plugin request Error : " << ((int) tmp_err->getStatus()) << " errMsg: " << tmp_err->getErrMsg());
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

int UgrLocPlugin_s3::do_List(UgrFileInfo *fi, LocationInfoHandler *handler){
    return LocationPlugin::do_List(fi, handler);
}

int UgrLocPlugin_s3::run_findNewLocation(const std::string & lfn, std::shared_ptr<NewLocationHandler> handler){
    std::string new_lfn(lfn);
    static const char * fname = "UgrLocPlugin_s3::run_findNewLocation";
    std::string canonical_name(base_url_endpoint.getString());
    std::string xname;
    std::string alt_prefix;

    // do name translation
    if(doNameXlation(new_lfn, xname, wop_Nop, alt_prefix) != 0){
          LocPluginLogInfoThr(UgrLogger::Lvl4, fname, "can not be translated " << new_lfn);
          return 1;
    }


    if(concat_url_path(canonical_name, xname, canonical_name) == false){
        return 1;
    }

    try{

        Davix::HeaderVec vec;
        std::string new_Location;

        Davix::Uri signed_location = signURI(params, "PUT", canonical_name, vec, signature_validity);
        LocPluginLogInfoThr(UgrLogger::Lvl3, fname, "Obtain signed newLocation " << signed_location);

        new_Location = HttpUtils::protocolHttpNormalize(signed_location.getString());
        HttpUtils::pathHttpNomalize(new_Location);

        handler->addReplica(new_Location, getID());
        

        LocPluginLogInfoThr(UgrLogger::Lvl3, fname, "newLocation found with success " << signed_location);
        return 0;

    }catch(Davix::DavixException & e){
        LocPluginLogInfoThr(UgrLogger::Lvl3, fname, "Error on newLocation: " << e.what());
    }catch(...){
        LocPluginLogInfoThr(UgrLogger::Lvl3, fname, "Unknown Error on newLocation");
    }

    return -1;
}


int UgrLocPlugin_s3::run_deleteReplica(const string & lfn, const std::shared_ptr<DeleteReplicaHandler> handler){
    std::string new_lfn(lfn);
    static const char * fname = "UgrLocPlugin_s3::run_deleteReplica";
    std::string canonical_name(base_url_endpoint.getString());
    std::string xname;
    std::string alt_prefix;

    // do name translation
    if(doNameXlation(new_lfn, xname, wop_Nop, alt_prefix) != 0){
          LocPluginLogInfoThr(UgrLogger::Lvl4, fname, "can not be translated " << new_lfn);
          return 1;
    }



    if(concat_url_path(canonical_name, xname, canonical_name) == false){
        return 1;
    }


    try{

        // TODO: ACL check here

        LocPluginLogInfoThr(UgrLogger::Lvl3, fname, "Try Deletion for  " << canonical_name);
        Davix::File f(dav_core, canonical_name);
        f.deletion(&params);
        LocPluginLogInfoThr(UgrLogger::Lvl3, fname, "Deletion done with success for  " << canonical_name);

        UgrFileItem_replica rep;
        rep.name = canonical_name;
        rep.status = UgrFileItem_replica::Deleted;
        handler->addReplica(rep, getID());
        return 0;

    }catch(Davix::DavixException & e){
        LocPluginLogInfoThr(UgrLogger::Lvl3, fname, "Error on Deletion: " << e.what());
    }catch(...){
        LocPluginLogInfoThr(UgrLogger::Lvl3, fname, "Unknown Error on Deletion");
    }
    return 0;

}



void UgrLocPlugin_s3::configure_S3_parameter(const std::string & prefix){

    const std::string s3_priv_key = pluginGetParam<std::string>(prefix, "s3.priv_key");
    const std::string s3_pub_key = pluginGetParam<std::string>(prefix, "s3.pub_key");
    const std::string s3_region = pluginGetParam<std::string>(prefix, "s3.region");
    
    signature_validity = (time_t)pluginGetParam<long>(prefix, "s3.signaturevalidity", 3600);
    Info(UgrLogger::Lvl1, name, " S3 signature validity is " << signature_validity);
    
    // Now abort everything if the signature validity clashes with the settings of the cache
    // This thing is very important, hence no cache parameter means bad  
    if (signature_validity < (time_t)pluginGetParam<long>(prefix, "extcache.memcached.ttl", 1000000)-60) {
      Error(name, " The given signature validity of " << signature_validity <<
        " is not compatible with the expiration time of the external cache extcache.memcached.ttl");
      throw 1;
    }
    if (signature_validity < (time_t)pluginGetParam<long>(prefix, "infohandler.itemmaxttl", 1000000)-60) {
      Error(name, " The given signature validity of " << signature_validity <<
      " is not compatible with the expiration time of the internal cache infohandler.itemmaxttl");
      throw 1;
    }

    const bool s3_alternate = pluginGetParam<bool>(prefix, "s3.alternate", false);
    if (s3_priv_key.size() > 0 && s3_pub_key.size()){
        Info(UgrLogger::Lvl1, name, " S3 authentication defined");
    }
    params.setAwsAuthorizationKeys(s3_priv_key, s3_pub_key);
    checker_params.setAwsAuthorizationKeys(s3_priv_key, s3_pub_key);

    if(s3_region.size() > 0){
        Info(UgrLogger::Lvl1, name, " S3 region defined - using v4 authentication");
        params.setAwsRegion(s3_region);
        checker_params.setAwsRegion(s3_region);
    }

    if(s3_alternate) {
        Info(UgrLogger::Lvl1, name, " S3 - using v2 alternate");
    }

    params.setAwsAlternate(s3_alternate);
    checker_params.setAwsAlternate(s3_alternate);
}

// concat URI + path, if it correspond to a bucket name, return false -> error
bool UgrLocPlugin_s3::concat_url_path(const std::string & base_uri, const std::string & path, std::string & canonical){
    static const char * fname = "UgrLocPlugin_s3::concat_s3_url_path";
    // s3 does not support //
    auto it = path.begin();
    while(*it == '/' && it < path.end())
        it++;

    if(it == path.end()){
        LocPluginLogInfoThr(UgrLogger::Lvl3, fname, "bucket name, ignore " << path);
        return false;
    }

    canonical = base_uri;
    canonical.append("/");
    canonical.append(it, path.end());
    return true;
}


void UgrLocPlugin_s3::do_Check(int myidx) {
    do_CheckInternal(myidx, "UgrLocPlugin_s3::do_Check");

}


Davix::Uri UgrLocPlugin_s3::signURI(const Davix::RequestParams & params, const std::string & method, const Davix::Uri & url, Davix::HeaderVec headers, const time_t expirationTime) {

    return Davix::S3::signURI(params, method, url, headers, expirationTime);
}




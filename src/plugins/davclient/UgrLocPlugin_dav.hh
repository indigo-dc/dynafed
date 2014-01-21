#pragma once
#ifndef LOCATIONPLUGIN_WEBDAV_HH
#define LOCATIONPLUGIN_WEBDAV_HH


/** 
 * @file   UgrLocPlugin_dav.hh
 * @brief  Plugin that talks to any Webdav compatible endpoint
 * @author Devresse Adrien
 * @date   Feb 2012
 */

#include <davix.hpp>
#include <string>
#include <glibmm.h>
#include "../../LocationPlugin.hh"
#include "../httpclient/UgrLocPlugin_http.hh"

/**
 *  Dav plugin config parameters
 *
 */

/** 
 * Location Plugin for Ugr, inherit from the LocationPlugin
 *  allow to do basic query to a webdav endpoint
 **/
class UgrLocPlugin_dav : public UgrLocPlugin_http {
protected:

    virtual void do_Check(int myidx);
public:

    ///
    /// Follow the standard LocationPlugin construction
    ///
    ///
    UgrLocPlugin_dav(SimpleDebug *dbginstance, Config *cfginstance, std::vector<std::string> &parms);
    virtual ~UgrLocPlugin_dav(){}

    ///
    /// main executor for the plugin
    ///
    virtual void runsearch(struct worktoken *op, int myidx);


    virtual int do_List(UgrFileInfo *fi, LocationInfoHandler *handler);

protected:

};





#endif


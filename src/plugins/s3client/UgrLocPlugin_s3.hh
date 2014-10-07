#pragma once
#ifndef LOCATIONPLUGIN_S3_HH
#define LOCATIONPLUGIN_S3_HH


/** 
 * @file   UgrLocPlugin_s3.hh
 * @brief  Plugin that talks to any S3 compatible endpoint
 * @author Devresse Adrien
 * @date   Feb 2012
 */

#include <davix.hpp>
#include <string>
#include "../../LocationPlugin.hh"
#include "../httpclient/UgrLocPlugin_http.hh"

/**
 *  s3 plugin config parameters
 *
 */

/** 
 * Location Plugin for Ugr, inherit from the LocationPlugin
 *  allow to do basic query to a S3 endpoint
 **/
class UgrLocPlugin_s3 : public UgrLocPlugin_http {
protected:

    virtual void do_Check(int myidx);
public:

    ///
    /// Follow the standard LocationPlugin construction
    ///
    ///
    UgrLocPlugin_s3(UgrConnector & c, std::vector<std::string> & parms);
    virtual ~UgrLocPlugin_s3(){}

    ///
    /// main executor for the plugin
    ///
    virtual void runsearch(struct worktoken *op, int myidx);


    virtual int do_List(UgrFileInfo *fi, LocationInfoHandler *handler);

protected:
    void configure_S3_parameter(const std::string & str);
};





#endif


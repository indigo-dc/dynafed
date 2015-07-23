#pragma once
#ifndef LOCATIONPLUGIN_WEBDAVRUCIO_HH
#define LOCATIONPLUGIN_WEBDAVRUCIO_HH
/**
 *  Copyright (c) CERN 2014
 *
 *  Licensed under the Apache License, Version 2.0
 */

/** 
 * @file   UgrLocPlugin_davrucio.hh
 * @brief  Plugin that talks to any Webdav compatible endpoint, applying the Rucio replica name hash-based xlation
 * @author Fabrizio Furano
 * @date   Oct 2014
 */

#include <davix.hpp>
#include <string>
#include "../davclient/UgrLocPlugin_dav.hh"

/**
 *  Dav plugin config parameters
 *
 */

/** 
 * Location Plugin for DAV with Rucio conventions, inherit from the DAV plugin
 *  and change how the name xlation acts
 **/
class UgrLocPlugin_davrucio : public UgrLocPlugin_dav {
protected:
    // The pars for the Rucio-friendly name xlation
    // that, when triggered, ALSO puts the Rucio hashes in the right place
    std::vector<std::string> xlatepfxruciohash_from;
    std::string xlatepfxruciohash_to;
    
    
    /// Applies the plugin-specific name translation
    virtual int doNameXlation(std::string &from, std::string &to, LocationPlugin::workOp op, std::string &altpfx);

public:

    ///
    /// Follow the standard LocationPlugin construction
    ///
    ///
    UgrLocPlugin_davrucio(UgrConnector & c, std::vector<std::string> & parms);
    virtual ~UgrLocPlugin_davrucio(){}


protected:

};





#endif


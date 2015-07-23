#ifndef UGRMOCKFN_HH
#define UGRMOCKFN_HH
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

#include <UgrConnector.hh>

/// Mocking fuction for UGR, testing
///
extern std::function<bool (UgrConnector*, const UgrFileItem_replica&)> replicasStatusObj;

#endif // UGRMOCKFN_HH

#ifndef UGRMOCKFN_HH
#define UGRMOCKFN_HH

#include <UgrConnector.hh>

/// Mocking fuction for UGR, testing
///
extern std::function<bool (UgrConnector*, const UgrFileItem_replica&)> replicasStatusObj;

#endif // UGRMOCKFN_HH

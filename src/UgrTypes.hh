/**
  * @file Ugr forwards declarations
  * */
#ifndef UGRTYPES_HH
#define UGRTYPES_HH


#include <deque>

class UgrConnector;
class LocationPlugin;
class ExtCacheHandler;
class UgrFileItem_replica;
class UgrClientInfo;



/// Vector of Replicas
typedef std::deque<UgrFileItem_replica> UgrReplicaVec;


#endif // UGRTYPES_HH

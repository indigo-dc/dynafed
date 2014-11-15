
#include <string>
#include <sstream>
#include <gtest/gtest.h>
#include <UgrConnector.hh>
#include <UgrMockFn.hh>
#include <cmath>


UgrFileItem_replica create_replica(){
    UgrFileItem_replica item;
    std::ostringstream ss;
    ss << "random_name" << rand();
    item.name = ss.str();
    item.pluginID = rand();
    return item;
}


bool pass_all(UgrConnector*, const UgrFileItem_replica&){
    return false;
}

bool pass_minus_10(UgrConnector*, const UgrFileItem_replica&){
    static int i=0;
    if(i <10){
        i +=1;
        return false;
    }
    return true;
}


TEST(filterTests, filterOnline){
    const int init_size = 20;
    std::deque<UgrFileItem_replica> vec_rep;

    for(int i= 0; i < init_size; i++){
        vec_rep.push_back(create_replica());
    }

    // create connector, not load anything
    UgrConnector c;

    replicasStatusObj = pass_all;
    c.filterAndSortReplicaList(vec_rep, UgrClientInfo());
    ASSERT_EQ(vec_rep.size(), init_size);

    replicasStatusObj = pass_minus_10;
    c.filterAndSortReplicaList(vec_rep, UgrClientInfo());
    ASSERT_EQ(vec_rep.size(), init_size-10);

}


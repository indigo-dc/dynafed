

/* testnewlocation
 * test new location for directory
 *
 */



#include <iostream>
#include "../UgrConnector.hh"
#include <stdio.h>
#include <dirent.h>

using namespace std;


int main(int argc, char **argv) {

    if (argc != 5) {
        cout << "Usage: " << argv[0] << " <repeat count> <cfgfile> <file to write > <client ip>" << endl;
        exit(1);
    }

    UgrConnector ugr;

    cout << "Initializing" << endl;
    if (ugr.init(argv[2]))
        return 1;

    cout << "Invoking findNewLocation " << endl;
    string fn = argv[3];

    UgrReplicaVec res;
    UgrClientInfo client(argv[4]);
    ugr.findNewLocation(fn, client, res);


    if(res.size() ==0){
        std::cout << " No writing location found : error !" << std::endl;
        return 1;
    }else{
        std::cout << "Start new location listing" << std::endl;
        for(auto it = res.begin(); it < res.end(); ++it){
            std::cout << "NewLocation: " << it->name << std::endl;
        }
    }

    cout << "test findNewLocation : End " << endl;
    return 0;
}
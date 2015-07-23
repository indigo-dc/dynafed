
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



/* testremove
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

    cout << "Invoking remove " << endl;
    string fn = argv[3];

    UgrReplicaVec res;
    UgrClientInfo client(argv[4]);
    UgrCode r = ugr.remove(fn, client, res);


    if(r.isOK() == false){
        std::cout << " Error during remove" << r.getString() << std::endl;
        return 1;
    }else{
        std::cout << "Remove executed with success" << std::endl;
        for(auto it = res.begin(); it < res.end(); ++it){
            std::cout << "To remove: " << it->name << std::endl;
        }
    }

    cout << "test remove : End " << endl;
    return 0;
}

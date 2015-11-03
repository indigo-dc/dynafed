
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


/* testlistdir
 * list a directory content by Ugr
 * 
 */



#include <iostream>
#include "../UgrConnector.hh"
#include <stdio.h>
#include <dirent.h>

using namespace std;


int main(int argc, char **argv) {

    if (argc != 4) {
        cout << "Usage: " << argv[0] << " <repeat count> <cfgfile> <direcoty to read >" << endl;
        exit(1);
    }

    UgrConnector ugr;

    cout << "Initializing" << endl;
    if (ugr.init(argv[2]))
        return 1;

    cout << "Invoking list " << endl;
    string fn = argv[3];

    UgrFileInfo *file_infos=NULL, *file_infos2=NULL;
    UgrClientInfo cli;
    ugr.list(fn, cli, &file_infos);

    std::cout << "-------------------------- testlistdir: Start listing: " << std::endl;
    if (file_infos->getItemsStatus() == UgrFileInfo::Ok) {
           for(std::set<UgrFileItem, UgrFileItemComp>::iterator it = file_infos->subdirs.begin();
				it != file_infos->subdirs.end();
				++it)
				std::cout << "Subitem :  " << it->name << " " << it->location << std::endl;
           
           std::cout << "------------------------ testlistdir: Now stat-ing all the items: " << std::endl;
           for(std::set<UgrFileItem, UgrFileItemComp>::iterator it = file_infos->subdirs.begin();
                                it != file_infos->subdirs.end();
                                ++it) {
               string name=fn+"/"+it->name;
           
               ugr.stat(name, cli, &file_infos2);
               switch (file_infos->getItemsStatus()) {
                   case UgrFileInfo::Ok:
                       std::cout << "Subitem :  " << file_infos2->name << " Flags: " << file_infos2->unixflags << std::endl;
                       break;
                   case UgrFileInfo::NotFound:
                       std::cout << "file : " << fn << " " << strerror(ENOENT) << std::endl;
                       return -2;
                   case UgrFileInfo::Error:
                       std::cout << "testlistdir: Error" << std::endl;
                       return -2;
                   case UgrFileInfo::InProgress:
                       std::cout << "testlistdir: Bug, still in progress" << std::endl;
                       return -2;
                   case UgrFileInfo::NoInfo:
                       std::cout << "testlistdir: Bug, No Information" << std::endl;
                       return -2;
                   default:
                       std::cout << " Unknow FATAL Error ! " << std::endl;
                       return -2;
               
               }
           }
           
    } else if(file_infos->getItemsStatus() == UgrFileInfo::NotFound ){
		std::cout << "file : " << fn << " " << strerror(ENOENT) << std::endl;
		return -1;
	} else {
        if(file_infos->getItemsStatus() == UgrFileInfo::Error){
            std::cout << "testlistdir: Error" << std::endl;
        }
        if(file_infos->getItemsStatus() == UgrFileInfo::InProgress){
           std::cout << "testlistdir: Bug, still in progress" << std::endl;
        }
        if(file_infos->getItemsStatus() == UgrFileInfo::NoInfo){
           std::cout << "testlistdir: Bug, No Information" << std::endl;
        }
		std::cout << " Unknow FATAL Error ! " << std::endl;
		return -2;
	}
	
    cout << "testlistdir : End listdir " << endl;
    return 0;
}

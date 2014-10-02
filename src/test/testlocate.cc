
/* testlocate
 * list replicas  by Ugr
 * 
 */



#include <iostream>
#include "../UgrConnector.hh"
#include <stdio.h>
#include <dirent.h>

using namespace std;

int main(int argc, char **argv) {

    if (argc != 5) {
        cout << "Usage: " << argv[0] << " <repeat count> <cfgfile> <path/file> <client hostname>" << endl;
        exit(1);
    }

    UgrConnector ugr;

    cout << "Initializing" << endl;
    if (ugr.init(argv[2]))
        return 1;

    cout << "Invoking Locate " << endl;
    string fn = argv[3];
    UgrClientInfo cli_info(argv[4]);

    cout << "Client info "<< cli_info.ip << endl;

    UgrFileInfo* file_infos = NULL;
    ugr.locate(fn, &file_infos);

    if (file_infos->getLocationStatus() == UgrFileInfo::Ok) {
        for (std::set<UgrFileItem_replica, UgrFileItemComp>::iterator it = file_infos->replicas.begin();
                it != file_infos->replicas.end();
                ++it)
            std::cout << "Raw Replicas :  " << it->name << std::endl;

        std::deque<UgrFileItem_replica> repls;
        file_infos->getReplicaList(repls);
        ugr.filter(repls, cli_info);

        for (std::deque<UgrFileItem_replica>::iterator it = repls.begin(); it < repls.end(); it++)
            std::cout << "Sorted Replicas :  " << it->name << " " << it->location << std::endl;
        
    } else if (file_infos->getLocationStatus() == UgrFileInfo::NotFound) {
        std::cerr << "file : " << fn << " " << strerror(ENOENT) << std::endl;
        return -1;
    } else {
        std::cerr << " Unknow FATAL Error ! " << std::endl;
        return -2;
    }

    cout << "End Locate " << endl;
    return 0;
}

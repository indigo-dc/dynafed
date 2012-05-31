
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
    UgrFileInfo *fi = 0;

    long long cnt = atoll(argv[1]);

    cout << "Initializing" << endl;
    if (ugr.init(argv[2]))
        return 1;

    cout << "Invoking list " << endl;
    string fn = argv[3];

	UgrFileInfo* file_infos;
    ugr.list(fn, &file_infos);

    if (file_infos->getStatStatus() == UgrFileInfo::Ok) {
           for(std::set<UgrFileItem, UgrFileItemComp>::iterator it = file_infos->subitems.begin();
				it != file_infos->subitems.end();
				++it)
				std::cout << " " << it->name << " " << it->location << std::endl;
    }else if(file_infos->getStatStatus() == UgrFileInfo::NotFound ){
		std::cout << "file : " << fn << " " << strerror(ENOENT) << std::endl;
	}
	
    cout << "End listdir " << endl;	
}

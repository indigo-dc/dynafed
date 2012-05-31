
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

    if (argc != 4) {
        cout << "Usage: " << argv[0] << " <repeat count> <cfgfile> <direcoty to read >" << endl;
        exit(1);
    }

    UgrConnector ugr;

    cout << "Initializing" << endl;
    if (ugr.init(argv[2]))
        return 1;

    cout << "Invoking Locate " << endl;
    string fn = argv[3];

	UgrFileInfo* file_infos=NULL;
    ugr.locate(fn, &file_infos);

    if (file_infos->getLocationStatus() == UgrFileInfo::Ok) {
           for(std::set<UgrFileItem, UgrFileItemComp>::iterator it = file_infos->subitems.begin();
				it != file_infos->subitems.end();
				++it)
				std::cout << "Replicas :  " << it->name << " " << it->location << std::endl;
    }else if(file_infos->getLocationStatus() == UgrFileInfo::NotFound ){
		std::cerr << "file : " << fn << " " << strerror(ENOENT) << std::endl;
		return -1;
	}else{
		std::cerr << " Unknow FATAL Error ! " << std::endl;
		return -2;
	}
	
    cout << "End Locate " << endl;	
    return 0;
}
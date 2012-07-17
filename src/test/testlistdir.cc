
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

	UgrFileInfo* file_infos=NULL;
    ugr.list(fn, &file_infos);

    if (file_infos->getItemsStatus() == UgrFileInfo::Ok) {
           for(std::set<UgrFileItem, UgrFileItemComp>::iterator it = file_infos->subdirs.begin();
				it != file_infos->subdirs.end();
				++it)
				std::cout << "File :  " << it->name << " " << it->location << std::endl;
    }else if(file_infos->getStatStatus() == UgrFileInfo::NotFound ){
		std::cerr << "file : " << fn << " " << strerror(ENOENT) << std::endl;
		return -1;
	}else{
		std::cerr << " Unknow FATAL Error ! " << std::endl;
		return -2;
	}
	
    cout << "End listdir " << endl;	
    return 0;
}

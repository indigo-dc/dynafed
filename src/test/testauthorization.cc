
/*
 *  Copyright (c) CERN 2016
 *
 *
 *  Licensed under the Apache License, Version 2.0
 *  See the LICENSE file for further information
 * 
 */


/* testperm
 * Exercise the ugr authorization layer
 *
 *
 * by Fabrizio Furano, CERN, Oct 2016
 */



#include <iostream>
#include "../UgrConnector.hh"
#include <stdio.h>
#include <sys/stat.h>

#include <string>
#include <vector>

using namespace std;


int main(int argc, char **argv) {
  
  if (argc != 7) {
    cout << "Usage: " << argv[0] << " <repeat count> <cfgfile> <user DN> <group or role> <resource> <single requested privilege>" << endl;
    exit(1);
  }
  
  UgrConnector ugr;
  UgrClientInfo cli_info("localhost");
  UgrFileInfo *fi = 0;
  
  long long cnt = atoll(argv[1]);
  
  cout << "Initializing" << endl;
  if (ugr.init(argv[2]))
    return 1;
  
  cout << "Checking perms" << endl;
  UgrClientInfo cli;
  string user = argv[3];
  
  std::vector<std::string> groups;
  groups.push_back( argv[4] );
  
  string resource = argv[5];
  string privs = argv[6];
  int res;

  
  if ( res = ugr.checkperm("testperm.cc", user, "localhost", groups, groups, (char *)resource.c_str(), privs[0]) ) {
    
    // Not allowed. Explain why
    
    std::ostringstream ss;
    ss << "Unauthorized operation " << privs[0] << " on " << resource;
    ss << " ClientName: " << user << " Addr:" << "localhost" << " fqans: ";
    for (unsigned int i = 0; i < groups.size(); i++ ) {
      ss << groups[i];
      if (i < groups.size() - 1) ss << ",";
    }

  }
  
  
  
  cout << "Result:" << res << endl;
  return res;
  
  
}



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

/** @file   UgrConfig.cc
 * @brief  A helper class that implements a configuration manager. A place where to get configuration parameters from
 * @author Fabrizio Furano
 * @date   Jan 2011
 */


#include "UgrConfig.hh"
#include "SimpleDebug.hh"

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <dirent.h>

using namespace std;


// The instance of our singleton
UgrConfig *UgrConfig::inst = NULL;

UgrConfig *UgrConfig::GetInstance() {
  if (UgrConfig::inst == NULL)
    UgrConfig::inst = new UgrConfig();

  return UgrConfig::inst;
}


// Trim the leading/trailing spaces from a string
void TrimSpaces(string &s) {
  // Remove the leading spaces
  int i;
  for (i = 0; i < (int)s.length(); i++) {
    if (s[i] != ' ') break;
  }
  s.erase(0, i);

  // Remove the trailing spaces
  for (i = s.length()-1; i >= 0; i--) {
    if (s[i] != ' ') break;
  }
  s.erase(i+1, s.length()-i);
}


// Substitute the tokens like ${...} with the value
// of the corresponding envvar
void DoSubst(string &s) {

  bool cont;

  do {
    size_t p1 = s.find("${", 0 );
    size_t p11;
    size_t p2 = 0;

    cont = false;
    if (p1 != s.npos) {
      p11 = p1+2;
      p2 = s.find("}", p11 );
      
      if ( (p2 != s.npos) && (p2 > p11) ) {
	// We have a token to check in the env
	string tok = s.substr(p11, p2-p11);
	
	char *val = getenv(tok.c_str());
	if (val) {
	  s.replace ( p1, p2-p1+1, val );
	  
	  // Look for another occurrence
	  cont = true;
	}
	else
	  Error("DoSubst", "Envvar not found: " << tok);
      }
    }

  } while (cont);

}

int UgrConfig::ProcessFile(char *filename) {
  char fn[10240];

  // Do the parsing
  if (!filename || (strlen(filename) == 0)) {
    strcpy(fn, "/etc/ugr.conf");
    Info(UgrLogger::Lvl1, "UgrConfig::ProcessFile", "Using default config file " << fn);
  }
  else {
    strcpy(fn, filename);
    Info(UgrLogger::Lvl1, "UgrConfig::ProcessFile", "Reading config file " << fn);
  }

  string line, token, val;
  ifstream myfile (fn);
  if (myfile.is_open()){
    vector<string> configfiles;

    myfile.exceptions(std::ios::badbit);
    try{
        while ( myfile.good() ) {
          getline (myfile,line);

          // Avoid comments
          if (line[0] == '#') continue;
          Info(UgrLogger::Lvl3, "UgrConfig::ProcessFile", "fn: " << fn << " line: '" << line << "'");

          if( strncasecmp(line.c_str(), "include", 7) == 0) {
              // only interested in the path
              line.erase(0, 7);
              TrimSpaces(line);
              // check if path is absolute
              if(line[0] != '/') {
                  Error("UgrConfig::ProcessFile", "Directory path must be absolute. fn: " << fn << " line: '" << line << "'");
                  continue;
              }
              configfiles = ReadDirectory(line);
          }
          else {
              token = "";
              val = "";

              char *p = strchr((char *)line.c_str(), (int)':');
              if (!p) continue;

              int pos = p-line.c_str();
              if (pos > 0) {
            char buf[10240];

            strncpy(buf, line.c_str(), pos);
            buf[pos] = 0;
            token = buf;

            strncpy(buf, line.c_str()+pos+1, 1024);
            buf[1023] = 0;
            val = buf;
              }

              TrimSpaces(val);
              DoSubst(val);

              if (token.length() > 0) {
                  char *p2 = strstr((char *)token.c_str(), "[]");
                  char buf2[10240];
                  int pos2 = p2-token.c_str();
                  if (p2 && (pos2 > 0)) {
                // it's a string to be added to an array
                    strncpy(buf2, token.c_str(), pos2);
                    buf2[pos2] = 0;
                    token = buf2;
                    // check if key already exist
                    if(arrdata.count(token) == 1) {
                        Info(UgrLogger::Lvl1, "UgrConfig::ProcessFile", "Duplicate key, overwritting original value. fn: " << fn << " line: '" << line << "'");
                    }
                    Info(UgrLogger::Lvl4, "UgrConfig::ProcessFile", token << "[" << arrdata[token].size() << "] <-" << val);
                arrdata[token].push_back(val);
              }
                  else {
                    if(data.count(token) == 1) {
                        Info(UgrLogger::Lvl1, "UgrConfig::ProcessFile", "Duplicate key, overwritting original value. fn: " << fn << " line: '" << line << "'");
                    }
                    Info(UgrLogger::Lvl4, "UgrConfig::ProcessFile", token << "<-" << val);
                    data[token] = val;
                  }
              }
          }
        }
    }catch(std::exception & e){
        Error("UgrConfig::ProcessFile", "Error during configuration file processing " << fn << " "<< e.what());
        return -1;
    }

    if(!configfiles.empty() ) {
        for(unsigned int i = 0; i < configfiles.size(); ++i) {
            ProcessFile((char *)configfiles[i].c_str() );
        }
    }

  }else {
    Error("UgrConfig::ProcessFile", "Unable to open file " << fn); 
    return -1;
  }

  return 0;
}








//////////////////////////////////////////////
//
// Getters and setters
//





void UgrConfig::SetLong(const char *name, long val) {
  char buf[1024];

  sprintf(buf, "%ld", val);
  data[name] = buf;
}

void UgrConfig::SetString(const char *name, char *val) {
  data[name] = val;
}


long UgrConfig::GetLong(const char *name, long deflt) {
    return GetLong(std::string(name), deflt);
}

long UgrConfig::GetLong(const string &name, long deflt){
    if (data.find(name) == data.end()) {
        std::string newname;
        if(!UgrConfig::FindWithWildcard(name, &newname, deflt) )
            return deflt;
        return atol( data[newname].c_str() );
    }        
    return atol( data[name].c_str() );
}

bool UgrConfig::GetBool(const char *name, bool deflt) 
{
  return GetBool(std::string(name), deflt);
  
}

bool UgrConfig::GetBool(const string & name, bool deflt) 
{
  if (data.find(name) == data.end()){
      std::string newname;  
      if(!UgrConfig::FindWithWildcard(name, &newname, deflt) )
          return deflt;    
      else {
          if (!strcasecmp(data[newname].c_str(), "yes")) return true;
          if (!strcasecmp(data[newname].c_str(), "true")) return true;
          
          return false;
      } 
  } 

  if (!strcasecmp(data[name].c_str(), "yes")) return true;
  if (!strcasecmp(data[name].c_str(), "true")) return true;
  
  return false;
}

string UgrConfig::GetString(const char *name, char *deflt) {

  return GetString(std::string(name), std::string(deflt));
}

string UgrConfig::GetString(const string & name, const string & deflt) {

  if (data.find(name) == data.end()) {
      std::string newname;  
      if(!UgrConfig::FindWithWildcard(name, &newname, deflt) )
          return deflt;
      return data[newname];
  }
  return data[name];
}

void UgrConfig::GetString(const char *name, char *val, char *deflt) {

  if (!val) return;
  if (data.find(name) == data.end()) {
    if (deflt) strcpy(val, deflt);
    else val[0] = 0;
  }
  else
    strcpy(val, data[name].c_str());

}

void UgrConfig::ArrayGetString(const char *name, char *val, int pos) {

  if (!val) return;
  if (arrdata.find(name) == arrdata.end()) {
    val[0] = 0;
    return;
  }
  else {
    if (pos >= (int)arrdata[name].size()) {
      val[0] = 0;
      return;
    }
    strcpy(val, arrdata[name][pos].c_str());

  }

}

vector<string> ReadDirectory(const string& path) {
    vector<string> filename_vec;
    DIR* dp;
    dirent* entry;

    dp = opendir(path.c_str() );

    if(!dp) {
        Error("UgrConfig::ReadDirectory", "Failed to open directory: " << path);
        return filename_vec;
    }
    
    // find all files inside the directory and push full path to vector
    while((entry = readdir(dp)) != NULL) {
            const std::string conf_file_ext(".conf");
            std::string filename = entry->d_name;
            if( filename[0] != '.'
                    /* take only file finishing by .conf */
                && std::distance(std::search(filename.begin(), filename.end(), conf_file_ext.begin(), conf_file_ext.end()),
                                 filename.end())
                    == std::distance(conf_file_ext.begin(), conf_file_ext.end())){
                filename_vec.push_back(path + "/" + filename );
            }

    }
    closedir(dp);
    std::sort(filename_vec.begin(), filename_vec.end() );
    
    return filename_vec;
}

vector<string> tokenize(const string& str, const string& delimiters) {
    vector<string> tokens;

    // skip delimiters at beginning.
    string::size_type lastPos = str.find_first_not_of(delimiters, 0);

    // find first "non-delimiter".
    string::size_type pos = str.find_first_of(delimiters, lastPos);

    while (string::npos != pos || string::npos != lastPos) {
        // found a token, add it to the vector.
        tokens.push_back(str.substr(lastPos, pos - lastPos));

        // skip delimiters.  Note the "not_of"
        lastPos = str.find_first_not_of(delimiters, pos);

        // find next "non-delimiter"
        pos = str.find_first_of(delimiters, lastPos);
    }

    return tokens;
}




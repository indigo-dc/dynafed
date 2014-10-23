/** @file   Config.hh
 * @brief  A helper class that implements a configuration manager. A place where to get configuration parameters from
 * @author Fabrizio Furano
 * @date   Jan 2011
 */


#include "Config.hh"
#include "SimpleDebug.hh"

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <dirent.h>

using namespace std;


// The instance of our singleton
Config *Config::inst = NULL;

Config *Config::GetInstance() {
  if (Config::inst == NULL)
    Config::inst = new Config();

  return Config::inst;
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

int Config::ProcessFile(char *filename) {
  char fn[1024];

  // Do the parsing
  if (!filename || (strlen(filename) == 0)) {
    strcpy(fn, "/etc/ugr.conf");
    Info(Logger::Lvl2, "Config::ProcessFile", "Using default config file " << fn);
  }
  else {
    strcpy(fn, filename);
    Info(Logger::Lvl2, "Config::ProcessFile", "Reading config file " << fn);
  }

  string line, token, val;
  ifstream myfile (fn);
  if (myfile.is_open()) {
    vector<string> configfiles;
    while ( myfile.good() ) {
      getline (myfile,line);

      // Avoid comments
      if (line[0] == '#') continue;
      Info(Logger::Lvl3, "Config::ProcessFile", line);

      // Check for INCLUDE 
      if(line.compare(0, 7, "INCLUDE") == 0) {    
          // only interested in the path
          line.erase(0, 7);
          TrimSpaces(line);
          // check if path is absolute
          if(line[0] != '/') {
              Error("Config::ProcessFile", "Directory path must be absolute");
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
        char buf[1024];

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
              char buf2[1024];
              int pos2 = p2-token.c_str();
              if (p2 && (pos2 > 0)) {
            // it's a string to be added to an array
                strncpy(buf2, token.c_str(), pos2);
                buf2[pos2] = 0;
                token = buf2;
                // check if key already exist
                if(arrdata.count(token) == 1) {
                    Info(Logger::Lvl1, "Config::ProcessFile", "Duplicate key, overwritting original value");
                }
                Info(Logger::Logger::Lvl4, "Config::ProcessFile", token << "[" << arrdata[token].size() << "] <-" << val);
            arrdata[token].push_back(val);
          }
              else {
                if(data.count(token) == 1) {
                    Info(Logger::Lvl1, "Config::ProcessFile", "Duplicate key, overwritting original value");
                }
                Info(Logger::Logger::Lvl4, "Config::ProcessFile", token << "<-" << val);
                data[token] = val;
              }
          }
      }  
    }
    if(!configfiles.empty() ) {
        for(unsigned int i; i < configfiles.size(); ++i) {
        ProcessFile((char *)configfiles[i].c_str() );
    }  
  }

    myfile.close();
  }
  else {
    cout << "Unable to open file"; 
    return -1;
  }

  return 0;
}








//////////////////////////////////////////////
//
// Getters and setters
//





void Config::SetLong(const char *name, long val) {
  char buf[1024];

  sprintf(buf, "%ld", val);
  data[name] = buf;
}

void Config::SetString(const char *name, char *val) {
  data[name] = val;
}


long Config::GetLong(const char *name, long deflt) {
    return GetLong(std::string(name), deflt);
}

long Config::GetLong(const string &name, long deflt){
    if (data.find(name) == data.end()) {
        std::string newname;
        if(!Config::FindWithWildcard(name, &newname, deflt) )
            return deflt;
        return atol( data[newname].c_str() );
    }        
    return atol( data[name].c_str() );
}

bool Config::GetBool(const char *name, bool deflt) 
{
  return GetBool(std::string(name), deflt);
  
}

bool Config::GetBool(const string & name, bool deflt) 
{
  if (data.find(name) == data.end()){
      std::string newname;  
      if(!Config::FindWithWildcard(name, &newname, deflt) )
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

string Config::GetString(const char *name, char *deflt) {

  return GetString(std::string(name), std::string(deflt));
}

string Config::GetString(const string & name, const string & deflt) {

  if (data.find(name) == data.end()) {
      std::string newname;  
      if(!Config::FindWithWildcard(name, &newname, deflt) )
          return deflt;
      return data[newname];
  }
  return data[name];
}

void Config::GetString(const char *name, char *val, char *deflt) {

  if (!val) return;
  if (data.find(name) == data.end()) {
    if (deflt) strcpy(val, deflt);
    else val[0] = 0;
  }
  else
    strcpy(val, data[name].c_str());

}

void Config::ArrayGetString(const char *name, char *val, int pos) {

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
        Error("Config::ReadDirectory", "Failed to open directory: " << path);
        return filename_vec;
    }
    
    // find all files inside the directory and push full path to vector
    while(true) {
        entry = readdir(dp);
        if(entry == NULL) break;
        if((strcmp(entry->d_name, ".")==0) || (strcmp(entry->d_name, "..")==0))
            continue;
        // construct full absolute path to file
        filename_vec.push_back(path + "/" + string(entry->d_name) );
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




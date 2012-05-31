/* Config
 * A place where to get configuration parameters from
 *
 *
 * by Fabrizio Furano, CERN, Jan 2011
 */




#include "Config.hh"
#include "SimpleDebug.hh"

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <string.h>

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
    strcpy(fn, "/etc/sysconfig/SEMsg.cf");
    Info(SimpleDebug::kMEDIUM, "Config::ProcessFile", "Using default config file " << fn);
  }
  else {
    strcpy(fn, filename);
    Info(SimpleDebug::kMEDIUM, "Config::ProcessFile", "Reading config file " << fn);
  }

  string line, token, val;
  ifstream myfile (fn);
  if (myfile.is_open()) {

    while ( myfile.good() ) {
      getline (myfile,line);

      // Avoid comments
      if (line[0] == '#') continue;
      Info(SimpleDebug::kHIGH, "Config::ProcessFile", line);
      
      token = "";
      val = "";

      char *p = strchr((char *)line.c_str(), (int)':');
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
          if (pos2 > 0) {
	    // it's a string to be added to an array
            strncpy(buf2, token.c_str(), pos2);
            buf2[pos] = 0;
            token = buf2;
            Info(SimpleDebug::kHIGHEST, "Config::ProcessFile", token << "[" << arrdata[token].size() << "] <-" << val);
	    arrdata[token].push_back(val);
	  }
          else {
            Info(SimpleDebug::kHIGHEST, "Config::ProcessFile", token << "<-" << val);
            data[token] = val;
          }
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

  if (data.find(name) == data.end()) return deflt;
  return atol( data[name].c_str() );

}


bool Config::GetBool(const char *name, bool deflt) {
  if (data.find(name) == data.end()) return deflt;

  if (!strcasecmp(data[name].c_str(), "yes")) return true;
  if (!strcasecmp(data[name].c_str(), "true")) return true;
  
  return false;
  
}

string Config::GetString(const char *name, char *deflt) {

  if (data.find(string(name)) == data.end()) return deflt;
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

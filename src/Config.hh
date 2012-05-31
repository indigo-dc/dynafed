#ifndef CONFIG_HH
#define CONFIG_HH

/* Config
 * A place where to get configuration parameters from
 *
 *
 * by Fabrizio Furano, CERN, Jan 2011
 */


#include <map>
#include <string>
#include <vector>


// The macro to be used to access the cfg options
#define CFG (Config::GetInstance())


class Config {

protected:
  static Config *inst;
  Config() {};

  std::map<std::string, std::string> data;
  std::map<std::string, std::vector<std::string> > arrdata;
public:

  static Config *GetInstance();
  static void Set(Config *i) {
    inst = i;
  }

  // Gets the values from a config file, or eventually from the default one
  int ProcessFile(char *filename );

  void SetLong(const char *name, long val);
  void SetString(const char *name, char *val);

  long GetLong(const char *name, long deflt = 0);
  std::string GetString(const char *name, char *deflt);
  void GetString(const char *name, char *val, char *deflt);

  // This supports also true|false, yes|no
  bool GetBool(const char *name, bool deflt = false);

  // Get the string in the specified pos of this array
  void ArrayGetString(const char *name, char *val, int pos);
};

#endif


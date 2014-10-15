/** @file   Config.hh
 * @brief  A helper class that implements a configuration manager. A place where to get configuration parameters from
 * @author Fabrizio Furano
 * @date   Jan 2011
 */
#ifndef CONFIG_HH
#define CONFIG_HH

#include <map>
#include <string>
#include <vector>


/// The macro to be used to access the cfg options
#define CFG (Config::GetInstance())

// Utility to tokenize compound lines
std::vector<std::string> tokenize(const std::string& str, const std::string& delimiters);

/// Singleton class that implements a simple config manager
/// Once initialized with ProcessFile, it will contain all
/// the config parameters, organized as:
/// - (string)--->value
/// - (string, idx)--->value
/// This gives a simple way of treating scalar/string parameters like:
/// DebugLevel: 2
/// MyParameter: myvalue
/// ... and arrays of scalars/strings as well like:
/// LoadPlugin[] myplugin myparametersforthisplugin
/// LoadPlugin[] myplugin myparametersforthisplugin
class Config {
protected:
    static Config *inst;

    Config() {
    };

    /// Stores the simple parameters
    std::map<std::string, std::string> data;

    /// Stores the array parameters
    std::map<std::string, std::vector<std::string> > arrdata;
public:

    /// We are a singleton
    static Config *GetInstance();

    /// Quirk to sync singletons with plugins in shared libs

    static void Set(Config *i) {
        inst = i;
    }

    /// Gets the values from a config file
    /// @param filename Configuration file to parse
    int ProcessFile(char *filename);

    /// Set a value of type long
    /// @param name The name of the parameter
    /// @param val  The value for the parameter
    void SetLong(const char *name, long val);
    /// Set a value of type string
    /// @param name The name of the parameter
    /// @param val  The value for the parameter
    void SetString(const char *name, char *val);

    /// Get a value of type long
    /// @param name The name of the parameter
    /// @param deflt  The default value for the parameter
    long GetLong(const char *name, long deflt = 0);
    long GetLong(const std::string & name, long deflt = 0);

    /// Get a value of type string
    /// @param name The name of the parameter
    /// @param deflt  The default value for the parameter
    std::string GetString(const char *name, char *deflt);

    /// Get a value of type long
    /// @param name The name of the parameter
    /// @param val  Char array where to get the result
    /// @param deflt  The default value for the parameter
    void GetString(const char *name, char *val, char *deflt);
	std::string GetString(const std::string & name, const std::string & deflt);    

    /// Get a value of type boolean
    /// This supports also true|false, yes|no
    /// @param name The name of the parameter
    /// @param deflt  The default value for the parameter
    bool GetBool(const char *name, bool deflt = false);
    bool GetBool(const std::string & name, bool deflt) ;

    /// Get a value of type string from an array of values that have the same key
    /// followed by double square brackets[] in the cfg file
    /// @param name The name of the parameter
    /// @param val  The place where the string is written
    /// @param pos The position of the arrray to fetch
    void ArrayGetString(const char *name, char *val, int pos);

    /// Convert locplugin ID to *, then re-search
    /// @param name The name of the parameter
    /// @param newname The wildcard version of name
    /// @param deflt The default value for the parameter
    template <typename T>
    bool FindWithWildcard(const std::string & name, std::string *newname, const T deflt) {
        // return default unless entry starts with locplugin
        if(name.compare(0, 10, "locplugin") == 0)
            return false; 

        // entry is a locplugin config item, split name
        std::vector<std::string> tkns = tokenize(name, ".");
          
        // replace ID with wildcard and reconstruct name string
        tkns[1] = "*";
        for(unsigned int i = 0; i < tkns.size(); ++i) {
            *newname += tkns[i];
            *newname += ".";
        }
        // remove trailing '.'
        newname->erase((newname->length()-1), 1);      

        // new search with wildcard
        if (data.find(*newname) == data.end()) 
            return false;

        return true;
    }
};






#endif


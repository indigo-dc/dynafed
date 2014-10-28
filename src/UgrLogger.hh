#ifndef UgrLogger_HH
#define UgrLogger_HH

#include <syslog.h>

#include <sstream>
#include <string>

#include <map>
#include <vector>


/**
 * A UgrLogger class
 */
class UgrLogger
{

public:
	/// typedef for a bitmask (long long)
	typedef unsigned long long bitmask;
	/// typedef for a component name (std:string)
	typedef std::string component;

	static bitmask unregistered;

    /**
     * Use the same values for log levels as syslog
     */
    enum Level
    {
        Lvl0,       // The default?
        Lvl1,
        Lvl2,
        Lvl3,
        Lvl4
    };

    /// Destructor
    ~UgrLogger();

    static UgrLogger *instance;
    
    /// @return the singleton instance
    static UgrLogger *get()
    {
    	if (instance == 0)
	  instance = new UgrLogger();
    	return instance;
    }

    static void set(UgrLogger *inst) {
      UgrLogger *old = instance;
      instance = inst;
      if (old && (old != inst)) delete old;
    }
    /// @return the current debug level
    short getLevel() const
    {
        return level;
    }

    /// @param lvl : the logging level that will be set
    void setLevel(Level lvl)
    {
        level = lvl;
    }

    /// @param debug_stderr : enable/disable the stderr printing
    void SetStderrPrint(bool debug_stderr) {
      // log the process ID, connect to syslog without delay, log also to 'cerr'
      int options = LOG_PID | LOG_NDELAY;
    
      if (debug_stderr) options |= LOG_PERROR;
    
      // setting 'ident' to NULL means that the program name will be used
      openlog(0, options, LOG_USER);
    }
    
    /// @return true if the given component is being logged, false otherwise
    bool isLogged(bitmask m) const
    {
        if (mask == 0) return mask & unregistered;
    	return mask & m;
    }
    
    /// @param comp : the component that will be registered for logging
    void registerComponent(component const &  comp);
    
    /// @param components : list of components that will be registered for logging
    void registerComponents(std::vector<component> const & components);

    /// Sets if a component has to be logged or not
    /// @param comp : the component name
    /// @param tobelogged : true if we want to log this component
    void setLogged(component const &comp, bool tobelogged);
    
    /**
     * Logs the message
     *
     * @param lvl : log level of the message
     * @param component : bitmask assignet to the given component
     * @param msg : the message to be logged
     */
    void log(Level lvl, std::string const & msg) const;

    /**
     * @param if true all unregistered components will be logged,
     * 			if false only registered components will be logged
     */
    void logAll()
    {
    	mask = ~0;
    }
    
        
    /**
     * @param comp : component name
     * @return respectiv bitmask assigned to given component
     */
    bitmask getMask(component const & comp);

    /**
     * Build a printable stacktrace. Useful e.g. inside exceptions, to understand
     * where they come from.
     * Note: I don't think that the backtrace() function is thread safe, nor this function
     * Returns the number of backtraces
     * @param s : the string that will contain the printable stacktrace
     * @return the number of stacktraces
     */
    static int getStackTrace(std::string &s);


private:

    ///Private constructor
    UgrLogger();
    // Copy constructor (not implemented)
    UgrLogger(UgrLogger const &);
    // Assignment operator (not implemented)
    UgrLogger & operator=(UgrLogger const &);

    /// current log level
    short level;
    /// number of components that were assigned with a bitmask
    int size;
    /// global bitmask with all registered components
    bitmask mask;
    /// component name to bitmask mapping
    std::map<component, bitmask> mapping;



};


#endif

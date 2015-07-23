#ifndef UgrLogger_HH
#define UgrLogger_HH

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
        Lvl0=0,       // The default?
        Lvl1=1,
        Lvl2=2,
        Lvl3=3,
        Lvl4=4
    };

    /// Destructor
    ~UgrLogger();

    
    /// @return the singleton instance
    static UgrLogger *get();

    static void set(UgrLogger *inst);
    
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
        stderr_log = debug_stderr;
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

    /// log to stderr
    bool stderr_log;
    /// current log level
    short level;
    /// number of components that were assigned with a bitmask
    int size;
    /// global bitmask with all registered components
    bitmask mask;
    /// component name to bitmask mapping
    std::map<component, bitmask> mapping;
    
    static UgrLogger *instance;


};


#endif

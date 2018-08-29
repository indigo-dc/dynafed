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






#include "UgrLogger.hh"
#include <stdlib.h>
#include <iostream>
#include <boost/iterator/iterator_concepts.hpp>
#include <cxxabi.h>
#include <execinfo.h>
#include <string.h>

UgrLogger *UgrLogger::instance = 0;
UgrLogger::bitmask UgrLogger::unregistered = ~0;









UgrLogger* UgrLogger::get()
    {
    	if (instance == 0)
	  instance = new UgrLogger();
    	return instance;
    }

void UgrLogger::set(UgrLogger *inst) {
      UgrLogger *old = instance;
      instance = inst;
      if (old && (old != inst)) delete old;
    }

// Build a printable stacktrace. Useful e.g. inside exceptions, to understand
// where they come from.
// Note: I don't think that the backtrace() function is thread safe, nor this function
// Returns the number of backtraces
int UgrLogger::getStackTrace(std::string &s)
{
  std::ostringstream o;
  
  void * array[8];
  int size = backtrace(array, 8);
  
  int linecnt = 0;
    
  char ** messages = backtrace_symbols(array, size);     
  
  // skip first stack frame (points here)
  // skip previous one (usually an exception)
  for (int i = 2; i < size && messages != NULL; ++i)
  {
    // Let's not print more than 4 lines
    if (linecnt > 3) break;
    
    char *mangled_name = 0, *offset_begin = 0, *offset_end = 0;
    
    // find parantheses and +address offset surrounding mangled name
    for (char *p = messages[i]; *p; ++p)
    {
      if (!p) break;
      
      if (*p == '(') {
	mangled_name = p; 
      }
      else if (*p == '+') {
	offset_begin = p;
      }
      else if (*p == ')') {
	offset_end = p;
	break;
      }
    }  
      
    // if the line could be processed, attempt to demangle the symbol
    if (mangled_name && offset_begin && offset_end && 
      mangled_name < offset_begin)
    {
      *mangled_name++ = '\0';
      *offset_begin++ = '\0';
      *offset_end++ = '\0';
      
      int status;
      char * real_name = abi::__cxa_demangle(mangled_name, 0, 0, &status);
    
      
      // if demangling is successful, output the demangled function name
      // Skip those who contain "dmlite::DmException::"
      // as they are never meaningful and clutter the log
      if (status == 0) {
	
	if (!strstr(real_name, "dmlite::DmException::") ) {
	  o << "[bt]: (" << i << ") " << messages[i] << " : " 
	  << real_name << "+" << offset_begin << offset_end 
	  << std::endl;
	  
	  linecnt++;
	}
	
      }
      // otherwise, output the mangled function name
      else
      {
	o << "[bt]: (" << i << ") " << messages[i] << " : " 
	<< mangled_name << "+" << offset_begin << offset_end 
	<< std::endl;
	
	linecnt++;
      }
      free(real_name);
    }
    // otherwise, print the whole line
    else
    {
      o << "[bt]: (" << i << ") " << messages[i] << std::endl;
      linecnt++;
    }
  }
  
  free(messages);
  
  s += o.str();
  
  return size;
}















UgrLogger::UgrLogger() : stderr_log(true), level(Lvl0), size(0)
{
    mask = 0;
    registerComponent("unregistered");
    mask = unregistered = getMask("unregistered");
    
    // log the process ID, connect to syslog without delay, log also to 'cerr'
    int options = LOG_PID | LOG_NDELAY;
    // setting 'ident' to NULL means that the program name will be used
    openlog(0, options, LOG_USER);
}



UgrLogger::~UgrLogger()
{
    closelog();
}

void UgrLogger::log(Level lvl, const std::string & msg) const
{
    syslog(LOG_INFO, "%s", msg.c_str());

    if(stderr_log){
        std::cerr << msg << std::endl;
    }
}

void UgrLogger::registerComponent(component const &  comp)
{
    // check if a mapping already exists
    std::map<component, bitmask>::const_iterator cit = mapping.find(comp);
    if (cit != mapping.end()) return;
    // otherwise generate a new bitmask for the component
    bitmask m = 0;
    m    |= 1 << size; // mask for the component
    mask |= 1 << size; // global mask with all components
    ++size;
    // store the mapping
    mapping.insert(std::make_pair(comp, m));
    
    std::ostringstream outs;
    outs << "UgrLogger::registerComponent" << " : " << "Registered logger component" << comp << "," << size;                      			\
    log(Lvl0, outs.str());
}

void UgrLogger::registerComponents(std::vector<component> const & components)
{
    std::vector<component>::const_iterator it;
    for (it = components.begin(); it != components.end(); ++it)
    {
        registerComponent(*it);
    }
}

UgrLogger::bitmask UgrLogger::getMask(component const & comp)
{
    // if the component has been registered return the respective bit mask
    std::map<component, bitmask>::const_iterator it = mapping.find(comp);
    if (it != mapping.end()) return it->second;
    // otherwise return the mask reserved for unregistered componentsS
    return unregistered;
}

void UgrLogger::setLogged(component const &comp, bool tobelogged) {
    registerComponent(comp);
    bitmask b = getMask(comp);
    
    if (tobelogged) {
      // Switch on the corresponsing bit
      mask |= b;
      // Setting ON some logging disables the unregistered category where everything else falls
      if (comp != "unregistered")
	setLogged("unregistered", false);
    }
    else
      // Switch off the corresponding bit
      mask &= ~b;
    
    
}


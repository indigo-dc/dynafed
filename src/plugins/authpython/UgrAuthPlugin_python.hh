/*
 *  Copyright (c) CERN 2015
 *
 *
 *  Licensed under the Apache License, Version 2.0
 *  See the LICENSE file for further information
 * 
 */


/** @file   UgrAuthPlugin_python.hh
 * @brief  Authorization implementation that invokes a python function, by loading the python interpreter
 * @author Fabrizio Furano
 * @date   Nov 2015
 */

#include "UgrAuthorization.hh"
class UgrConnector;
class myPyFuncInfo;
#include <Python.h>



// Helper mini-class that holds values useful to talk to Python, related to a function to be invoked
class myPyFuncInfo {
public:
  std::string module, func;
  PyObject *pModule, *pFunc;
  myPyFuncInfo(): pModule(NULL), pFunc(NULL) {}
};


class UgrAuthorizationPlugin_py : public UgrAuthorizationPlugin {
public:
  UgrAuthorizationPlugin_py( UgrConnector & c, std::vector<std::string> & parms);
  
  virtual ~UgrAuthorizationPlugin_py();
  
  
  virtual bool isallowed(const char *fname,
                         const std::string &clientName,
                         const std::string &remoteAddress,
                         const std::vector<std::string> &fqans,
                         const std::vector< std::pair<std::string, std::string> > &keys,
                         const char *reqresource, const char reqmode);
  
private:
  
  
  boost::recursive_mutex mtx;
  
  myPyFuncInfo info_pyfunc;
  static boost::recursive_mutex pymtx;
  static bool python_initdone;
  int pypreinit(myPyFuncInfo &funcnfo);
  int pyinit(myPyFuncInfo &funcnfo);
  int pyterm(myPyFuncInfo &funcnfo);
  int pyxeqfunc2(int &retval, PyObject *pFunc,
                  const std::string &clientName,
                  const std::string &remoteAddress,
                  const char *resource,
                  const char reqmode,
                  const std::vector<std::string> &fqans,
                 const std::vector< std::pair<std::string, std::string> > &keys );
};

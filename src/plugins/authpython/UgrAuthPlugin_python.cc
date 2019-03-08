/*
 *  Copyright (c) CERN 2015
 *
 *
 *  Licensed under the Apache License, Version 2.0
 *  See the LICENSE file for further information
 * 
 */


#include "UgrAuthPlugin_python.hh"
#include <time.h>
#include "libs/time_utils.h"



boost::recursive_mutex UgrAuthorizationPlugin_py::pymtx;
bool UgrAuthorizationPlugin_py::python_initdone = false;
static PyThreadState *global_tstate;




PyObject* log_CaptureStdout(PyObject* self, PyObject* pArgs)
{
  
  char* LogStr = NULL;
  
  
  if (!PyArg_ParseTuple(pArgs, "s", &LogStr)) return NULL;
  
  
  Info(UgrLogger::Lvl2, "PythonStdout", LogStr);
  //printf("%s", LogStr); 
  // Simply using printf to do the real work. 
  // You could also write it to a .log file or whatever...
  // MessageBox(NULL, LogStr...
  // WriteFile(hFile, LogStr...
  
  Py_INCREF(Py_None);
  
  
  return Py_None;
}

// Notice we have STDERR too.
PyObject* log_CaptureStderr(PyObject* self, PyObject* pArgs)
{
  char* LogStr = NULL;
  
  
  if (!PyArg_ParseTuple(pArgs, "s", &LogStr)) return NULL;
  
  Info(UgrLogger::Lvl2, "PythonStderr", LogStr);
  //printf("%s", LogStr);
  
  Py_INCREF(Py_None);
  
  return Py_None;
}

static PyMethodDef logMethods[] = {
  {"CaptureStdout", log_CaptureStdout, METH_VARARGS, "Logs stdout"},
  {"CaptureStderr", log_CaptureStderr, METH_VARARGS, "Logs stderr"},
  {NULL, NULL, 0, NULL}
};



void logpythonerror(const char *fname) {
  PyObject* ptype;
  PyObject* pvalue;
  PyObject* ptraceback;
  PyObject* exc = NULL;
  PyObject* pstr;
  char *str = NULL;

  PyErr_Fetch(&ptype, &pvalue, &ptraceback);
  PyErr_NormalizeException(&ptype, &pvalue, &ptraceback);
  pstr = PyObject_Str(ptype);

  int lineno = -1;
  if (ptraceback) lineno = ((PyTracebackObject*)ptraceback)->tb_lineno;
  std::string errmsg = "(null)";
  if (pvalue) {
    str = PyString_AsString(pvalue);
    if (str) errmsg = str;
  }

  Error(fname, "Error '" << errmsg << "' occurred on line: " << lineno << " - " << PyString_AsString(pstr));

  Py_XDECREF(ptype);
  Py_XDECREF(pvalue);
  Py_XDECREF(ptraceback);
  Py_XDECREF(exc);

  PyErr_Clear();
}




// Initialize all the python vars that are needed to invoke the given function efficiently
int UgrAuthorizationPlugin_py::pyinit(myPyFuncInfo &funcnfo)
{
  const char *fname = "SEMsgConsumer_pyintf::pyinit";
  PyObject *pName;

  if (funcnfo.module == "") return 1;
  if (funcnfo.func == "") return 1;

  Info(UgrLogger::Lvl4, fname, "PYTHONPATH: " << getenv("PYTHONPATH"));

  // Dirty trick against sloppy Python scripts
  char tmpbuf[1024];
  char *p = tmpbuf;
  strcpy(tmpbuf, funcnfo.module.c_str());
  strcat(tmpbuf, ".py");
  PySys_SetArgv(1, &p);


  pName = PyString_FromString(funcnfo.module.c_str());
  /* Error checking of pName left out */

  funcnfo.pModule = PyImport_Import(pName);
  //Py_DECREF(pName);

  if (funcnfo.pModule != NULL) {

    funcnfo.pFunc = PyObject_GetAttrString(funcnfo.pModule, (char *)funcnfo.func.c_str());
    /* pFunc is a new reference */
    if (funcnfo.pFunc && PyCallable_Check(funcnfo.pFunc)) {
      return 0;
    }
    else {
      if (PyErr_Occurred())
        logpythonerror(fname);

      PyErr_Clear();
      Error(fname, "Cannot find function '" << funcnfo.func << "' in module '" << funcnfo.module << "'.");
      return 1;
    }

  }
  else {
    if (PyErr_Occurred())
      logpythonerror(fname);

    PyErr_Clear();
    Error(fname, "Failed to load Python module '" << funcnfo.module << "'. Have you checked the current PYTHONPATH? " << getenv("PYTHONPATH"));
    exit(255);
  }


}




// Initialize all the python vars that are needed to invoke the given function efficiently
int UgrAuthorizationPlugin_py::pyterm(myPyFuncInfo &funcnfo)
{
  {
    boost::lock_guard<boost::recursive_mutex> l(mtx);
    python_initdone = false;
  }
  
  
  PyGILState_STATE gstate;
  gstate = PyGILState_Ensure();
  
  if (funcnfo.pFunc) {
    Py_XDECREF(funcnfo.pFunc);
  }
  
  funcnfo.pFunc = NULL;

  if (funcnfo.pModule) {
    Py_DECREF(funcnfo.pModule);
  }
  
  funcnfo.pModule = NULL;

  
  /* Release the thread. No Python API allowed beyond this point. */
  PyGILState_Release(gstate);
  
return 0;
}




int UgrAuthorizationPlugin_py::pyxeqfunc2(int &retval, PyObject *pFunc,
                                          const std::string &clientName,
                                          const std::string &remoteAddress,
                                          const char *resource,
                                          const char reqmode,
                                          const std::vector<std::string> &fqans,
                                          const std::vector< std::pair<std::string, std::string> > &keys)
{ 
  const char *fname = "UgrAuthorizationPlugin_py::pyxeqfunc2";
  
  PyObject *pFqans, *pKeys, *pCouple, *pValue, *pArgs;
  int pos = 0;
  
  
  if (pFunc && PyCallable_Check(pFunc)) {
    pArgs = PyTuple_New(6); // CHANGEME
    
    
    // arg: clientname
    pValue = PyString_FromString(clientName.c_str());
    PyTuple_SetItem(pArgs, pos++, pValue);
    // arg: remote address
    pValue = PyString_FromString(remoteAddress.c_str());
    PyTuple_SetItem(pArgs, pos++, pValue);
    // arg: resource name
    pValue = PyString_FromString(resource);
    PyTuple_SetItem(pArgs, pos++, pValue);
    // arg: requested mode: r, w, l, d
    pValue = PyString_FromStringAndSize(&reqmode, 1);
    PyTuple_SetItem(pArgs, pos++, pValue);
    
    // all the fqans go into a pytuple
    pFqans = PyTuple_New(fqans.size());
    for (unsigned int j = 0; j < fqans.size(); j++) {
      pValue = PyString_FromString(fqans[j].c_str());

      if (!pValue) {
        if (PyErr_Occurred())
          logpythonerror(fname);
          
        Py_DECREF(pArgs);
        Py_DECREF(pFqans);
        PyErr_Clear();
        Error(fname, "Cannot convert fqan " << j << ": '" << fqans[j] << "'");
        
        return 1;
      }
        
      /* pValue reference stolen here: */
      PyTuple_SetItem(pFqans, j, pValue);
      
    }
    
    // all the key/values go into a pytuple of couples
    pKeys = PyTuple_New(keys.size());
    for (unsigned int j = 0; j < keys.size(); j++) {
      // What we will insert is a couple of strings representing key/value
      pCouple = PyTuple_New(2);
      
      // First item: key
      pValue = PyString_FromString(keys[j].first.c_str());
      if (!pValue) {
        if (PyErr_Occurred())
          logpythonerror(fname);
          
        Py_DECREF(pArgs);
        Py_DECREF(pFqans);
        Py_DECREF(pKeys);
        Py_DECREF(pCouple);
        PyErr_Clear();
        Error(fname, "Cannot convert key " << j << ": '" << keys[j].first << "'");
        
        return 1;
      }
      /* pValue reference stolen here: */
      PyTuple_SetItem(pCouple, 0, pValue);
      
      // Second item: value
      pValue = PyString_FromString(keys[j].second.c_str());
      if (!pValue) {
        if (PyErr_Occurred())
          logpythonerror(fname);
        
        Py_DECREF(pArgs);
        Py_DECREF(pFqans);
        Py_DECREF(pKeys);
        Py_DECREF(pCouple);
        PyErr_Clear();
        Error(fname, "Cannot convert key " << j << ": '" << keys[j].second << "'");
        
        return 1;
      }
      /* pValue reference stolen here: */
      PyTuple_SetItem(pCouple, 1, pValue);
      
      
      // Now we insert the couple into the tuple
      /* pValue reference stolen here: */
      PyTuple_SetItem(pKeys, j, pCouple);
      
    }
    

    /* pFqans pKeys reference stolen here: */
    PyTuple_SetItem(pArgs, pos++, pFqans);
    PyTuple_SetItem(pArgs, pos++, pKeys);
  }


    
  Info(UgrLogger::Lvl4, fname, "Invoking func");

  
  {
    struct timespec t1, t2;
    clock_gettime(CLOCK_MONOTONIC, &t1);
    
    
    pValue = PyObject_CallObject(pFunc, pArgs);
    
    Py_DECREF(pArgs);
    
    
    // Finish measuring the time needed
    clock_gettime(CLOCK_MONOTONIC, &t2);
    struct timespec diff_time;
    timespec_sub(&t2, &t1, &diff_time);
    int latency_ms = (diff_time.tv_sec)*1000 + (diff_time.tv_nsec) / 1000000L;
    
    if (latency_ms > 5)
      Error(fname, "The python authorization function took too long to execute. Please fix it or remove it.");
  }
    
  if (pValue != NULL) {
    retval = PyInt_AsLong(pValue);
    Info(UgrLogger::Lvl3, fname, "Result of call: " << retval);
    
    Py_DECREF(pValue);
  }
  else {
    if (PyErr_Occurred())
      logpythonerror(fname);

    
    Error(fname, "Call failed.");
    return 1;
  }
  

  
  PyErr_Clear();

  
  return 0;
}












UgrAuthorizationPlugin_py::UgrAuthorizationPlugin_py( UgrConnector & c, std::vector<std::string> & parms) : UgrAuthorizationPlugin(c, parms) {
  
  const char *fname = "UgrAuthorizationPlugin_py::UgrAuthorizationPlugin_py";
  // Take the parms
  if (parms.size() != 4) {
    
    // here we should abort everything
    throw "Fatal error, wrong number of arguments in UgrAuthorizationPlugin_py"; 
  }
  
  info_pyfunc.module = parms[2];
  info_pyfunc.func = parms[3];
  
  Info(UgrLogger::Lvl1, fname, "Python authorization invokes function: " << info_pyfunc.func << " from module " << info_pyfunc.module);
  
  python_initdone = false;
  
  
  //
  // Diabolic python version checks
  //
  const char *py_compile_version = PY_VERSION;
  const char *py_dynamic_version = 0;
  
  py_dynamic_version = strtok((char *)Py_GetVersion(), " ");
  
  if (strcmp(py_compile_version, py_dynamic_version) != 0) {
    Error(fname, "python_init: Python version mismatch, expected '" <<
    py_compile_version << "', found '" << py_dynamic_version << "'");
    Error(fname, "python_init: Python executable found '" <<
    Py_GetProgramFullPath() << "'");
    Error(fname, "python_init: Python path being used '" << 
    Py_GetPath() << "'");
    Error(fname, "python_init: ... continuing initialization anyway.");
  }
  else
    Info(UgrLogger::Lvl1, fname, "python_init: found Python version  '" <<
    py_dynamic_version << "'");
  
  
}






int UgrAuthorizationPlugin_py::pypreinit(myPyFuncInfo &funcnfo) {
  const char *fname = "UgrAuthorizationPlugin_py::pypreinit";
  bool doinit = false;
  
  {
    boost::lock_guard<boost::recursive_mutex> l(mtx);
    doinit = (!python_initdone);
    python_initdone = true;
  }
  //
  // Various hacks for initializing python, inspired by
  // mod_python
  // https://github.com/grisha/mod_python/blob/master/src/mod_python.c
  //
  
  
  if (doinit) {
    boost::lock_guard<boost::recursive_mutex> l(pymtx);
    
    /* disable user site directories */
    Py_NoUserSiteDirectory = 1;
    
    /* Initialze the main interpreter. */
    #if PY_MAJOR_VERSION == 2 && \
    (PY_MINOR_VERSION < 7 || (PY_MINOR_VERSION == 7 && PY_MICRO_VERSION < 14))
    /*
     * We do not want site.py to
     * be imported because as of Python 2.7.9 it would cause a
     * circular dependency related to _locale which breaks
     * graceful restart so we set Py_NoSiteFlag to 1 just for this
     * one time. (https://github.com/grisha/mod_python/issues/46)
     */
    Py_NoSiteFlag = 1;
    #endif
    
    if (!Py_IsInitialized())
      Py_Initialize();

    
    #if PY_MAJOR_VERSION == 2 && \
    (PY_MINOR_VERSION < 7 || (PY_MINOR_VERSION == 7 && PY_MICRO_VERSION < 14))
    Py_NoSiteFlag = 0;
    #endif
    
    if (!PyEval_ThreadsInitialized()) {
      Info(UgrLogger::Lvl1, fname, "Initializing Python threads" );
      PyEval_InitThreads();
      PyThreadState* st = PyEval_SaveThread();
      Info(UgrLogger::Lvl1, fname, "Python threads initialized. st: " << st );
    }
    
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();
    
    Py_InitModule("mylog", logMethods);
    PyRun_SimpleString(
      "import mylog\n"
      "import sys\n"
      "class StdoutCatcher:\n"
      "\tdef write(self, str):\n"
      "\t\tmylog.CaptureStdout(str)\n"
      "class StderrCatcher:\n"
      "\tdef write(self, str):\n"
      "\t\tmylog.CaptureStderr(str)\n"
      "sys.stdout = StdoutCatcher()\n"
      "sys.stderr = StderrCatcher()\n"
      "sys.path.append(\"/\")\n"
      "sys.path.append(\"/etc/ugr/conf.d/\")\n"
    );
    
   
    
    
    
    if (pyinit(info_pyfunc)) {
      pyterm(info_pyfunc);
      
      // here we should abort everything
      //throw "Fatal error, cannot initialize python authorization module"; 
      
      /* Release the thread. No Python API allowed beyond this point. */
      PyGILState_Release(gstate);
      return 1;
    }
    
    
    /* Release the thread. No Python API allowed beyond this point. */
    PyGILState_Release(gstate);
    return 0;
  }
  
  return 0;
}


UgrAuthorizationPlugin_py::~UgrAuthorizationPlugin_py() {
    boost::lock_guard<boost::recursive_mutex> l(mtx);
    
    if (python_initdone)
      pyterm(info_pyfunc);
    
    python_initdone = false;
  
}


bool UgrAuthorizationPlugin_py::isallowed(const char *fname,
                                                  const std::string &clientName,
                                                  const std::string &remoteAddress,
                                                  const std::vector<std::string> &fqans,
                                                  const std::vector<std::pair<std::string, std::string>> &keys,
                                                  const char *reqresource, const char reqmode) {

  
  
  if (pypreinit(info_pyfunc)) {
    Error(fname, "Could not initialize python. Denying access.");
    return false;
  }
  
  PyGILState_STATE gstate;
  gstate = PyGILState_Ensure();
  
  int retval = 0;
  int r = pyxeqfunc2(retval, info_pyfunc.pFunc, clientName, remoteAddress, reqresource, reqmode, fqans, keys);

  
  /* Release the thread. No Python API allowed beyond this point. */
  PyGILState_Release(gstate);
  
  
  // A value of 0 got from a successful execution means allowed
  if (!r && !retval) {
    Info(UgrLogger::Lvl3, "isallowed", "Allowed. clientname: '" << clientName << "' remoteaddr: '" << remoteAddress << "' mode: " << reqmode );
    return true;
  }
  
  Info(UgrLogger::Lvl3, "isallowed", "Denied. clientname: '" << clientName << "' remoteaddr: '" << remoteAddress << "' mode: " << reqmode );
  return false;

}


// ------------------------------------------------------------------------------------
// Plugin-related stuff
// ------------------------------------------------------------------------------------




/// The plugin hook function. GetPluginInterfaceClass must be given the name of this function
/// for the plugin to be loaded

extern "C" PluginInterface *GetPluginInterface(GetPluginInterfaceArgs) {
  return (PluginInterface *)new UgrAuthorizationPlugin_py(c, parms);
}

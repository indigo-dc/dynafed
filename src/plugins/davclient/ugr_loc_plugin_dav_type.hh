#pragma once
#ifndef LOCATIONPLUGIN_DAV_TYPE_HH
#define LOCATIONPLUGIN_DAV_TYPE_HH


// type definition for RW lockers
typedef Glib::RWLock DavPluginRWMutex;
typedef Glib::RWLock::ReaderLock DavPluginReadLocker;
typedef Glib::RWLock::WriterLock DavPluginWriterLocker;


#endif


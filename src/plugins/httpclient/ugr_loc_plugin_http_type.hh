#pragma once
#ifndef LOCATIONPLUGIN_HTTP_TYPE_HH
#define LOCATIONPLUGIN_HTTP_TYPE_HH


// type definition for RW lockers
typedef Glib::RWLock HttpPluginRWMutex;
typedef Glib::RWLock::ReaderLock HttpPluginReadLocker;
typedef Glib::RWLock::WriterLock HttpPluginWriterLocker;


#endif


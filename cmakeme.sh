cd ..
cd build
setenv PKG_CONFIG_PATH /usr/local/lib64/pkgconfig/
cmake .. -DCMAKE_INSTALL_PREFIX=/usr -DDMLITEUTILS_LIBRARY=/usr/lib64/libdmliteutils.so -DDMLITE_LIBRARY=/usr/lib64/libdmlite.so -DSYSCONF_INSTALL_DIR=/etc

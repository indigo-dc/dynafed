**************
SEMsg library

A few hints to compile from source:

To make a build from scratch (like a make distclean, which CMake does not
provide)

>cd build
>../scratchbuild.sh

The CMake line that I normally use:

cmake ..
-DAPRUTIL_ALTLOCATION=/home/furano/LocalPark/activemqdev/apr-util-1.3.10_install/
-DAPR_ALTLOCATION=/home/furano/LocalPark/activemqdev/apr-1.4.2_install/
-DACTIVEMQCPP_ALTLOCATION=/home/furano/LocalPark/activemqdev/activemq-cpp_trunk_02112010_install/
-DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=/opt/lcg/
-DLIBDESTINATION=lib64

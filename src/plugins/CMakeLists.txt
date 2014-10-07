cmake_minimum_required (VERSION 2.6)

# include the directory itself as a path to include directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(dmlite)
add_subdirectory(dmliteclient)
add_subdirectory(httpclient)
add_subdirectory(davclient)
add_subdirectory(s3client)
add_subdirectory(lfcclient)
add_subdirectory(geoIP)
add_subdirectory(noloop)








cmake_minimum_required( VERSION 2.8 )

set( PROJ_NAME drone_client )

# common
project( ${PROJ_NAME} )
add_definitions( -fPIC -Wall -g -std=c++11 -fext-numeric-literals ) #TODO: -g debug ?

find_package( PkgConfig REQUIRED )
pkg_check_modules( GLIB REQUIRED IMPORTED_TARGET glib-2.0 )
pkg_check_modules( GSTREAMER REQUIRED IMPORTED_TARGET gstreamer-1.0 )
pkg_check_modules( GSTREAMERAPP REQUIRED IMPORTED_TARGET gstreamer-app-1.0 )
pkg_check_modules( OPENCV REQUIRED IMPORTED_TARGET opencv )

# qt
find_package( Qt5Core )
find_package( Qt5Gui )
find_package( Qt5Network )
find_package( Qt5SerialPort )
set( CMAKE_AUTOMOC ON )

#include( ${QT_USE_FILE} )
add_definitions( ${QT_DEFINITIONS} )
add_definitions( -DQT_NO_VERSION_TAGGING )
add_definitions( -DSWITCH_LOGGER_SIMPLE )
#add_definitions( ${OBJREPR_LIBRARY_EXIST} )
set( MOC_HEADERS image_from_drone.h drone_controller.h )
qt5_wrap_cpp( MOC_SRCS ${MOC_HEADERS} )

#
get_directory_property(MYDEFS COMPILE_DEFINITIONS)
message( STATUS "-----------------++++++++++++++++++++++++++++Compile defs contain: " ${MYDEFS} )
#

get_property( TP TARGET SWITCH_LOGGER_SIMPLE PROPERTY COMPILE_DEFINITIONS )
message( "werrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrr - ${TP}" )

include_directories( ${PROJECT_SOURCE_DIR}/from_ms_common )
include_directories( ${PROJECT_SOURCE_DIR}/include )
include_directories( ${PROJECT_SOURCE_DIR}/include/nppntt )

link_directories( ${PROJECT_SOURCE_DIR}/bin/lib )

# files
file( GLOB_RECURSE DSS_CLIENT_SRC *.cpp *.c )
file( GLOB_RECURSE DSS_CLIENT_HDR *.h )

if( DEFINED -DSWITCH_LOGGER_SIMPLE )
message("blaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa")
endif( DEFINED -DSWITCH_LOGGER_SIMPLE )

#list(REMOVE_ITEM lib_srcs "IlmImf/b44ExpLogTable.cpp")

# build
add_executable( ${PROJ_NAME} ${DSS_CLIENT_HDR} ${DSS_CLIENT_SRC} )
target_link_libraries( ${PROJ_NAME} PkgConfig::GLIB PkgConfig::GSTREAMER PkgConfig::GSTREAMERAPP PkgConfig::OPENCV Qt5::Core Qt5::Gui protobuf_old exiv2 boost_filesystem boost_regex boost_system rabbitmq objrepr unilog owlgroundcontrol )

# install
set( PUBLIC_HEADERS drone_client.h
)

install( TARGETS ${PROJ_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib )
install( FILES ${PUBLIC_HEADERS} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${PROJ_NAME} )

# - lib
#install( TARGETS ${PROJ_NAME} DESTINATION ${PROJECT_BINARY_DIR}/install_files/lib )
# - headers
#install( FILES ${PUBLIC_HEADERS} DESTINATION ${PROJECT_BINARY_DIR}/install_files/include/${PROJ_NAME} )

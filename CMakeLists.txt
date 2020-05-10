cmake_minimum_required( VERSION 2.8 )

set( PROJ_NAME drone_client )
project( ${PROJ_NAME} )

# pkg-config
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

# flags / defines / includes / libs
add_definitions( -fPIC -w -g -std=c++11 -fext-numeric-literals )

add_definitions( ${QT_DEFINITIONS} )
add_definitions( -DQT_NO_VERSION_TAGGING )
add_definitions( -DSWITCH_LOGGER_ASTRA )
add_definitions( -DOBJREPR_LIBRARY_EXIST )
set( MOC_HEADERS image_from_drone.h drone_controller.h )
qt5_wrap_cpp( MOC_SRCS ${MOC_HEADERS} )

include_directories( ${PROJECT_SOURCE_DIR}/from_ms_common )
include_directories( ${PROJECT_SOURCE_DIR}/include )
include_directories( ${PROJECT_SOURCE_DIR}/include/nppntt )
link_directories( ${PROJECT_SOURCE_DIR}/bin/lib )

# files
file( GLOB DC_SRC_TOP ${PROJECT_SOURCE_DIR}/*.cpp )
file( GLOB DC_HDR_TOP ${PROJECT_SOURCE_DIR}/*.h )
file( GLOB_RECURSE DC_SRC_NESTED ${PROJECT_SOURCE_DIR}/from_ms_common/*.cpp )
file( GLOB_RECURSE DC_HDR_NESTED ${PROJECT_SOURCE_DIR}/from_ms_common/*.h )

# filter from unneccesary files
foreach( f ${DC_HDR_NESTED} )	
	if( NOT ${f} MATCHES ".*logger_simple.*" AND NOT ${f} MATCHES ".*logger_normal.*" )		
		list( APPEND DC_FILTERED_NESTED_HDR ${f} )
	endif()
endforeach()

foreach( f ${DC_SRC_NESTED} )	
	if( NOT ${f} MATCHES ".*logger_simple.*" AND NOT ${f} MATCHES ".*logger_normal.*" )		
		list( APPEND DC_FILTERED_NESTED_SRC ${f} )
	endif()
endforeach()

# build
add_executable( ${PROJ_NAME} ${DC_SRC_TOP} ${DC_HDR_TOP} ${DC_FILTERED_NESTED_HDR} ${DC_FILTERED_NESTED_SRC} )
target_link_libraries( ${PROJ_NAME} pthread PkgConfig::GLIB PkgConfig::GSTREAMER PkgConfig::GSTREAMERAPP PkgConfig::OPENCV Qt5::Core Qt5::Gui protobuf_old exiv2 boost_filesystem boost_regex boost_system rabbitmq objrepr unilog owlgroundcontrol )

# install
install( TARGETS ${PROJ_NAME} DESTINATION ${PROJECT_SOURCE_DIR}/bin )

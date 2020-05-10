ROOT_DIR=./

TEMPLATE = app
TARGET = drone_client

include($${ROOT_DIR}pri/common.pri)

CONFIG += link_pkgconfig
PKGCONFIG += glib-2.0 gstreamer-1.0 gstreamer-app-1.0 opencv

QT += core serialport network

QMAKE_CXXFLAGS += -Wno-unused-parameter
QMAKE_CXXFLAGS += -Wno-unused-variable

DEFINES += \
    QT_NO_VERSION_TAGGING \
    SWITCH_LOGGER_SIMPLE \
#    SWITCH_LOGGER_ASTRA \
#    OBJREPR_LIBRARY_EXIST \

LIBS += \
    -lboost_regex \
    -lboost_system \
    -lboost_filesystem \
    -lexiv2 \
    -lprotobuf_old \
    -lowlgroundcontrol \

contains( DEFINES, OBJREPR_LIBRARY_EXIST ){
    message("connect 'unilog' and 'objrepr' libraries")
LIBS += \
    -lunilog \
    -lobjrepr
}

INCLUDEPATH += \
    $${PWD}/from_ms_common/ \
    $${ROOT_DIR}/nppntt \

SOURCES += main.cpp \
    drone_client.cpp \
    video_generator.cpp \
    image_from_drone.cpp \
    image_from_file.cpp \
    drone_controller.cpp \
    control_signal_receiver.cpp \
    from_ms_common/common/ms_common_types.cpp \
    from_ms_common/system/a_config_reader.cpp \    
    from_ms_common/system/logger_simple.cpp \
    from_ms_common/system/objrepr_bus.cpp \
    from_ms_common/communication/network_interface.cpp \
    from_ms_common/communication/unified_command_convertor.cpp \
    objrepr_bus_drone.cpp \
    config_reader.cpp

contains( DEFINES, OBJREPR_LIBRARY_EXIST ){
    message("connect astra-logger for 'unilog'")
SOURCES += \
    from_ms_common/system/logger_astra.cpp \
#    from_ms_common/system/logger_normal.cpp \
}

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

HEADERS += \
    drone_client.h \
    video_generator.h \
    common_stuff.h \
    common_utils.h \
    image_from_drone.h \
    image_from_file.h \
    drone_controller.h \
    control_signal_receiver.h \
    from_ms_common/common/ms_common_types.h \
    from_ms_common/common/ms_common_utils.h \
    from_ms_common/system/a_config_reader.h \
    from_ms_common/system/logger.h \
    from_ms_common/system/logger_common.h \    
    from_ms_common/system/logger_simple.h \
    from_ms_common/system/objrepr_bus.h \
    from_ms_common/communication/network_interface.h \
    from_ms_common/communication/unified_command_convertor.h \
    objrepr_bus_drone.h \
    config_reader.h \
    from_ms_common/common/ms_common_vars.h

contains( DEFINES, OBJREPR_LIBRARY_EXIST ){
    message("connect astra-logger for 'unilog'")
HEADERS += \
    from_ms_common/system/logger_astra.h \
#    from_ms_common/system/logger_normal.h \
}


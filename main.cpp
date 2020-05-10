
#include <QCoreApplication>
#include "from_ms_common/common/ms_common_utils.h"
#include "from_ms_common/system/logger.h"

#include "objrepr_bus_drone.h"
#include "config_reader.h"
#include "drone_client.h"

using namespace std;

int g_argc = 0;
char ** g_argv = nullptr;

static bool initSingletons( int _argc, char ** _argv, char ** _env ){

    // configs
    ConfigReader::SIninSettings settings3;
    settings3.mainConfigPath = "";
    settings3.commandConvertor = nullptr;
    settings3.env = _env;
    settings3.projectName = "drone_client";
    if( ! CONFIG_READER.init(settings3) ){
        return false;
    }

    // logger
    logger_common::SInitSettings settings2;
    settings2.loggerName = "DroneClient";
    settings2.unilogConfigPath = CONFIG_PARAMS.baseParams.SYSTEM_UNILOG_CONFIG_PATH;

    if( CONFIG_PARAMS.baseParams.SYSTEM_LOG_TO_STDOUT ){
        settings2.logEndpoints = (logger_common::ELogEndpoints)( (int)settings2.logEndpoints | (int)logger_common::ELogEndpoints::Stdout );
    }
    if( CONFIG_PARAMS.baseParams.SYSTEM_LOG_TO_FILE ){
        settings2.logEndpoints = (logger_common::ELogEndpoints)( (int)settings2.logEndpoints | (int)logger_common::ELogEndpoints::File );
        settings2.fileName = CONFIG_PARAMS.baseParams.SYSTEM_LOGFILE_NAME;
        settings2.filePath = CONFIG_PARAMS.baseParams.SYSTEM_REGULAR_LOGFILE_PATH;
        settings2.rotationSizeMb = CONFIG_PARAMS.baseParams.SYSTEM_LOGFILE_ROTATION_SIZE_MB;
    }

    Logger::singleton().initGlobal( settings2 );

    // objrepr bus
    ObjreprBus::SInitSettings busSettings;
    busSettings.objreprConfigPath = CONFIG_PARAMS.baseParams.OBJREPR_CONFIG_PATH;
    busSettings.initialContextName = CONFIG_PARAMS.baseParams.OBJREPR_INITIAL_CONTEXT_NAME;
    if( ! OBJREPR_BUS.init(busSettings) ){
        return false;
    }

    return true;
}

int main( int argc, char ** argv, char ** env ){

    QCoreApplication app( argc, argv );
    g_argc = argc;
    g_argv = argv;

    if( ! initSingletons(argc, argv, env) ){
        PRELOG_ERR << "============================ DRONE CLIENT FAILED (singletons area) ============================" << endl;
        return EXIT_FAILURE;
    }

    DroneClient::SInitSettings settings;
    if( DroneClient::singleton()->init(settings) ){
        DroneClient::singleton()->launch();
    }
    else{
        VS_LOG_ERROR << "============================ DRONE CLIENT FAILED ============================" << endl;
        return EXIT_FAILURE;
    }

    return app.exec();
}

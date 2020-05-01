
#include <fstream>

#include "from_ms_common/system/logger.h"

#include "config_reader.h"
#include "image_from_file.h"
#include "image_from_drone.h"
#include "drone_client.h"

using namespace std;

static constexpr const char * PRINT_HEADER = "DroneClient:";

extern int g_argc;
extern char ** g_argv;

DroneClient::DroneClient()
{
    setenv( "GST_DEBUG", "", 0 );
    setenv( "G_MESSAGES_DEBUG", "", 0 );
    gst_init( & g_argc, & g_argv );
}

DroneClient::~DroneClient()
{
    gst_deinit();
}

bool DroneClient::init( const SInitSettings & _settings ){

    // ----------------------------------------------------------------------
    // control sub-system
    // ----------------------------------------------------------------------
    ControlSignalReceiver::SInitSettings sigRecSettings;
    sigRecSettings.cameraObjectId = CONFIG_PARAMS.DRONE_CONTROL_CAMERA_OBJECT_ID;
    sigRecSettings.carrierObjectId = CONFIG_PARAMS.DRONE_CONTROL_CARRIER_OBJECT_ID;
    if( ! m_controlSignalReceiver.init(sigRecSettings) ){
        return false;
    }
    m_controlSignalReceiver.addObserver( & m_droneController );

    //
    DroneController::SInitSettings dcSettings;
    dcSettings.configFilePath = CONFIG_PARAMS.DRONE_CONTROL_LIB_CONFIG_PATH;
    dcSettings.pingTimeoutMillisec = CONFIG_PARAMS.DRONE_CONTROL_PING_TIMEOUT_MILLISEC;
    dcSettings.movingImitationEnable = CONFIG_PARAMS.DRONE_CONTROL_MOVING_IMITATION;
    if( ! m_droneController.init(dcSettings) ){
        return false;
    }
    m_droneController.addObserver( & m_controlSignalReceiver );

    // ----------------------------------------------------------------------
    // video sub-system
    // ----------------------------------------------------------------------
    if( "file" == CONFIG_PARAMS.VIDEO_STREAMING_SRC_TYPE ){
        ImageFromFile * imageProvider = new ImageFromFile();
        ImageFromFile::SInitSettings settings;
        settings.imageDir = CONFIG_PARAMS.VIDEO_STREAMING_IMAGES_DIR_PATH;
        settings.imageCaptureIntervalMilllisec = 50;
        settings.statusOverlay = CONFIG_PARAMS.VIDEO_STREAMING_STATUS_OVERLAY;
        if( ! imageProvider->init(settings) ){
            return false;
        }
        m_imageProvider = imageProvider;
    }
    else if( "drone" == CONFIG_PARAMS.VIDEO_STREAMING_SRC_TYPE ){
        ImageFromDrone * imageProvider = new ImageFromDrone();
        ImageFromDrone::SInitSettings settings;
        settings.configFilePath = CONFIG_PARAMS.DRONE_CONTROL_LIB_CONFIG_PATH;
        settings.statusOverlay = CONFIG_PARAMS.VIDEO_STREAMING_STATUS_OVERLAY;
        if( ! imageProvider->init(settings) ){
            return false;
        }
        m_imageProvider = imageProvider;
    }
    else{
        VS_LOG_ERROR << PRINT_HEADER << " incorrect video source type [" << CONFIG_PARAMS.VIDEO_STREAMING_SRC_TYPE << "]" << endl;
        return false;
    }

    //
    VideoGenerator::SInitSettings settings2;
    settings2.imageFormat = VideoGenerator::EImageFormat::JPEG;
    settings2.enableMulticast = CONFIG_PARAMS.VIDEO_STREAMING_ENABLE_MULTICAST;
    settings2.rtpEmitIp = ( CONFIG_PARAMS.VIDEO_STREAMING_ENABLE_MULTICAST ? CONFIG_PARAMS.VIDEO_STREAMING_MULTICAST_GROUP_IP : CONFIG_PARAMS.VIDEO_STREAMING_UNICAST_DEST_IP );
    settings2.rtpEmitUdpPort = CONFIG_PARAMS.VIDEO_STREAMING_DEST_PORT;
    settings2.imageProvider = m_imageProvider;
    if( ! m_videoGenerator.init(settings2) ){
        return false;
    }

    VS_LOG_INFO << PRINT_HEADER << " ============================ INIT SUCCESS ============================" << endl;
    return true;
}

void DroneClient::launch(){

    // NOTE: don't stuck for Qt event queue process
}



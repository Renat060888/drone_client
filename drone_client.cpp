
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
    m_controlSignalReceiver.addObserver( (IControlSignalsObserver *)(& m_droneController) );
    m_controlSignalReceiver.addObserver( (ISystemObserver *)(& m_droneController) );

    //
    DroneController::SInitSettings dcSettings;
    dcSettings.configFilePath = CONFIG_PARAMS.DRONE_CONTROL_LIB_CONFIG_PATH;
    dcSettings.pingTimeoutMillisec = CONFIG_PARAMS.DRONE_CONTROL_PING_TIMEOUT_MILLISEC;
    dcSettings.imitationEnable = CONFIG_PARAMS.DRONE_CONTROL_IMITATION_ENABLE;
    if( ! m_droneController.init(dcSettings) ){
        return false;
    }
    m_droneController.addObserver( & m_controlSignalReceiver );

    // ----------------------------------------------------------------------
    // video sub-system
    // ----------------------------------------------------------------------
    if( "image-file" == CONFIG_PARAMS.VIDEO_STREAMING_SRC_TYPE ){
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

        imageProvider->m_signalFirstFrameFromDrone.connect( boost::bind( & DroneClient::firstFrameFronDrone, this ) );
        m_controlSignalReceiver.addObserver( imageProvider );
        m_imageProvider = imageProvider;
    }
    else if( "video-file" == CONFIG_PARAMS.VIDEO_STREAMING_SRC_TYPE ){
        // dummy
    }
    else{
        VS_LOG_ERROR << PRINT_HEADER << " incorrect video source type [" << CONFIG_PARAMS.VIDEO_STREAMING_SRC_TYPE << "]" << endl;
        return false;
    }

    //
    if( "image-file" == CONFIG_PARAMS.VIDEO_STREAMING_SRC_TYPE || "drone" == CONFIG_PARAMS.VIDEO_STREAMING_SRC_TYPE ){
        VideoGenerator::SInitSettings settings2;
        settings2.imageFormat = VideoGenerator::EImageFormat::JPEG;
        settings2.enableMulticast = CONFIG_PARAMS.VIDEO_STREAMING_ENABLE_MULTICAST;
        settings2.rtpEmitIp = ( CONFIG_PARAMS.VIDEO_STREAMING_ENABLE_MULTICAST ? CONFIG_PARAMS.VIDEO_STREAMING_MULTICAST_GROUP_IP : CONFIG_PARAMS.VIDEO_STREAMING_UNICAST_DEST_IP );
        settings2.rtpEmitUdpPort = CONFIG_PARAMS.VIDEO_STREAMING_DEST_PORT;
        settings2.imageProvider = m_imageProvider;
        if( ! m_videoGenerator.init(settings2) ){
            return false;
        }
    }
    else if( "video-file" == CONFIG_PARAMS.VIDEO_STREAMING_SRC_TYPE ){
        VideoConvertor::SInitSettings settings2;
        settings2.enableMulticast = CONFIG_PARAMS.VIDEO_STREAMING_ENABLE_MULTICAST;
        settings2.rtpEmitIp = ( CONFIG_PARAMS.VIDEO_STREAMING_ENABLE_MULTICAST ? CONFIG_PARAMS.VIDEO_STREAMING_MULTICAST_GROUP_IP : CONFIG_PARAMS.VIDEO_STREAMING_UNICAST_DEST_IP );
        settings2.rtpEmitUdpPort = CONFIG_PARAMS.VIDEO_STREAMING_DEST_PORT;
        settings2.fileFullPath = CONFIG_PARAMS.VIDEO_STREAMING_VIDEO_FILE_PATH;
        if( ! m_videoConvertor.init(settings2) ){
            return false;
        }
        m_videoConvertor.addObserver( & m_controlSignalReceiver );
    }
    else{
        VS_LOG_ERROR << PRINT_HEADER << " incorrect video source type [" << CONFIG_PARAMS.VIDEO_STREAMING_SRC_TYPE << "]" << endl;
        return false;
    }

    // start stream if source already available
    if( "image-file" == CONFIG_PARAMS.VIDEO_STREAMING_SRC_TYPE ){
        if( ! m_videoGenerator.connectToSource() ){
            return false;
        }
    }
    else if( "video-file" == CONFIG_PARAMS.VIDEO_STREAMING_SRC_TYPE ){
        if( ! m_videoConvertor.connectToSource() ){
            return false;
        }
    }

    VS_LOG_INFO << PRINT_HEADER << " ============================ INIT SUCCESS ============================" << endl;
    return true;
}

void DroneClient::firstFrameFronDrone(){

    VS_LOG_INFO << PRINT_HEADER << " signal from ImageFromDrone about first frame" << endl;
    m_videoGenerator.connectToSource();
}

void DroneClient::launch(){

    // NOTE: don't stuck for Qt event queue process
}



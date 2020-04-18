
#include <fstream>

#include "drone_client.h"
#include "image_from_file.h"
#include "image_from_drone.h"

using namespace std;

DroneClient::DroneClient()
{
}

DroneClient::~DroneClient()
{
    gst_deinit();
}

bool DroneClient::init( const SInitSettings & _settings ){

    setenv( "GST_DEBUG", "", 0 );
    setenv( "G_MESSAGES_DEBUG", "", 0 );
    gst_init( & const_cast<SInitSettings &>(_settings).argc, & const_cast<SInitSettings &>(_settings).argv );


    //
    ControlSignalReceiver::SInitSettings sigRecSettings;
    sigRecSettings.objectId = 18048399;
    if( ! m_controlSignalReceiver.init(sigRecSettings) ){
        return false;
    }

    return true;

//    //
//    ImageFromDrone * imageProvider2 = new ImageFromDrone();
//    ImageFromDrone::SInitSettings settings4;
//    settings4.configFilePath = "../doc/nppntt/rfconfig.json";
//    imageProvider2->init( settings4 );

//    return true;

    //
    ImageFromFile * imageProvider = new ImageFromFile();

    ImageFromFile::SInitSettings settings3;
//    settings3.imageDir = "../resources/17-46";
    settings3.imageDir = "/home/renat/0-develop/drone_client/resources/17-46";
    settings3.imageCaptureIntervalMilllisec = 50;
    if( ! imageProvider->init(settings3) ){
        return false;
    }

    m_imageProvider = imageProvider;

    //
    VideoGenerator::SInitSettings settings2;
    settings2.imageFormat = VideoGenerator::EImageFormat::JPEG;
    settings2.rtpEmitUdpPort = 5000;
    settings2.imageProvider = imageProvider;
    if( ! m_videoGenerator.init(settings2) ){
        return false;
    }

    return true;
}

void DroneClient::launch(){

    // NOTE: don't stuck for Qt event queue process
}



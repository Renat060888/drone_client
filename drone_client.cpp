
#include <fstream>

#include <opencv2/opencv.hpp>

#include "drone_client.h"
#include "image_from_file.h"
#include "image_from_drone.h"

using namespace std;

DroneClient::DroneClient()
{

}

bool DroneClient::init( const SInitSettings & _settings ){

    ImageFromDrone * imageProvider2 = new ImageFromDrone();
    ImageFromDrone::SInitSettings settings4;
    settings4.configFilePath = "../doc/nppntt/rfconfig.json";
    imageProvider2->init( settings4 );

    return true;

    //
    ImageFromFile * imageProvider = new ImageFromFile();

    ImageFromFile::SInitSettings settings3;
    settings3.imageDir = "../resources/17-46";
    settings3.imageCaptureIntervalMilllisec = 50;
    if( ! imageProvider->init(settings3) ){
        return false;
    }

    m_imageProvider = imageProvider;


    ifstream ifs( "00.02419.jpg", ios::binary | ios::ate );
    const ifstream::pos_type bytesCount = ifs.tellg();

    m_imageBytes.resize( bytesCount );

    ifs.seekg( 0, ios::beg );
    ifs.read( & m_imageBytes[ 0 ], bytesCount );

    const cv::Mat imageFromBase64 = cv::imdecode( m_imageBytes, 1 );

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

}



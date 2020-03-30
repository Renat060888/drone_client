
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>

#include "image_from_file.h"
#include "common_utils.h"

using namespace std;

static int32_t g_fps = 0;

ImageFromFile::ImageFromFile()
    : m_currentImageRef(nullptr)
    , m_currentSecondIdx(0)
    , m_currentImageIdx(0)
{

}

bool ImageFromFile::init( const SInitSettings & _settings ){

    m_state.settings = _settings;

    g_fps = 1000LL / _settings.imageCaptureIntervalMilllisec;

    if( ! createImageTimeline(_settings.imageDir) ){
        return false;
    }




    return true;
}

bool ImageFromFile::createImageTimeline( const std::string & _imageDir ){

    //
    const string targetPath = _imageDir;
//    const boost::regex filter1( "video_server_analyzer_.*\." );
    const boost::regex filter1( ".*\.jpg" );

    boost::filesystem::directory_iterator endIter;
    for( boost::filesystem::directory_iterator iter( targetPath ); iter != endIter; ++iter ){

        boost::smatch what;
        if( ! boost::regex_match( iter->path().filename().string(), what, filter1) ){
            continue;
        }

        VS_LOG_INFO << "image to load [" << iter->path().filename().string() << "]" << endl;

        // load
        iter->path();
    }
    //


    SOneSecondImages second;
    second.lastImageIdx = 0;

    SImage * image = loadImage( "" );
    second.images.push_back( image );

    m_imagesBySeconds.push_back( second );



    // TODO: get w/h via const cv::Mat imageFromBase64 = cv::imdecode( m_imageBytes, 1 );



    return true;
}

ImageFromFile::SImage * ImageFromFile::loadImage( const std::string & _imagePath ){

    SImage * image = new SImage();

    // time
    image->capturedAtTimeMillisec = 0;

    // data
    ifstream ifs( _imagePath, ios::binary | ios::ate );
    const ifstream::pos_type bytesCount = ifs.tellg();

    image->imageBytes.resize( bytesCount );

    ifs.seekg( 0, ios::beg );
    ifs.read( & image->imageBytes[ 0 ], bytesCount );

    // meta
    image->imageMetadata.first = image->imageBytes.data();
    image->imageMetadata.second = image->imageBytes.size();

    return image;
}

void ImageFromFile::tick(){

    // switching by real time
    static int64_t lastTimeImageSwitching = 0;
    if( (common_utils::getCurrentTimeMillisec() - lastTimeImageSwitching) < m_state.settings.imageCaptureIntervalMilllisec ){
        return;
    }
    lastTimeImageSwitching = common_utils::getCurrentTimeMillisec();

    // get actual image
    SOneSecondImages & second = m_imagesBySeconds[ m_currentSecondIdx ];
    const SImage * image = second.images[ m_currentImageIdx ];

    if( image ){
        m_currentImageRef = image;
        second.lastImageIdx = m_currentImageIdx;
    }
    else{
        m_currentImageRef = second.images[ second.lastImageIdx ];
    }

    // move indexes
    m_currentImageIdx++;
    if( m_currentImageIdx == (g_fps - 1) ){
        m_currentImageIdx = 0;

        m_currentSecondIdx = ++m_currentSecondIdx % m_imagesBySeconds.size();
    }
}

std::pair<TConstDataPointer, TDataSize> ImageFromFile::getImageData(){

    std::pair<TConstDataPointer, TDataSize> out;
    m_mutexImageRef.lock();
    out = m_currentImageRef->imageMetadata;
    m_mutexImageRef.unlock();
    return out;
}

SImageProperties ImageFromFile::getImageProperties(){

    return m_state.imageProps;
}

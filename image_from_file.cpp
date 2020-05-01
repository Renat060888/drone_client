
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include "from_ms_common/system/logger.h"

#include <QImage>
#include <QBuffer>

#include "image_from_file.h"
#include "common_utils.h"

using namespace std;

static constexpr const char * PRINT_HEADER = "FileImageProvider:";
static int32_t g_fps = 0;

ImageFromFile::ImageFromFile()
    : m_currentImageRef(nullptr)
    , m_currentSecondIdx(0)
    , m_currentFrameIdx(0)
{

}

bool ImageFromFile::init( const SInitSettings & _settings ){

    m_state.settings = _settings;

    g_fps = 1000LL / _settings.imageCaptureIntervalMilllisec;

    if( ! createImageTimeline(_settings.imageDir) ){
        return false;
    }

//    m_trImageRotation = new std::thread( & ImageFromFile::threadImageRotation, this );

    return true;
}

void ImageFromFile::threadImageRotation(){

    while( true ){

        tick();

        std::this_thread::sleep_for( chrono::milliseconds(10) );
    }
}

bool ImageFromFile::createImageTimeline( const std::string & _imageDir ){

    // 1 obtain jpeg file names
    vector<std::string> jpegFileNames;

    const boost::regex regexFilterForFileName( "[0-9][0-9]\..*\.jpg" ); // for instance 58.0045.jpg

    boost::filesystem::directory_iterator endIter;
    for( boost::filesystem::directory_iterator iter( _imageDir ); iter != endIter; ++iter ){

        boost::smatch regexMatch;
        if( ! boost::regex_match( iter->path().filename().string(), regexMatch, regexFilterForFileName) ){
            continue;
        }

        jpegFileNames.push_back( iter->path().filename().string() );
    }

    // 2 sort list of files
    std::sort( jpegFileNames.begin(), jpegFileNames.end() );

    // 3 load them into 'second' structures according their time
    const boost::regex regexFilterForSecondNumber( "([0-9][0-9])\..*" );

    int totalFramesInImages = 0;

    int currentSecondNum = -1;
    SOneSecondImages * currentSecond = nullptr;
    for( const string & fileName : jpegFileNames ){

        boost::smatch regexMatch;
        if( boost::regex_search( fileName.begin(), fileName.end(), regexMatch, regexFilterForSecondNumber) ){
            const int frameSecondNum = std::stoi( regexMatch[ 1 ] );

            // current second
            if( frameSecondNum == currentSecondNum ){
                SImage * image = loadImage( _imageDir + "/" + fileName );
                image->capturedAtTimeMillisec = 0;
                image->fileName = fileName;

                currentSecond->images.push_back( image );

                totalFramesInImages++;
            }
            // next second
            else{
                currentSecondNum = frameSecondNum;

                m_imagesBySeconds.resize( m_imagesBySeconds.size() + 1 );
                currentSecond = & m_imagesBySeconds[ m_imagesBySeconds.size() - 1 ];

                SImage * image = loadImage( _imageDir + "/" + fileName );
                image->capturedAtTimeMillisec = 0;
                image->fileName = fileName;

                currentSecond->images.push_back( image );
                currentSecond->lastImageIdx = 0;

                totalFramesInImages++;
            }
        }
    }

    VS_LOG_INFO << "average FPS in files set = " << (float)totalFramesInImages / (float)m_imagesBySeconds.size() << endl;

    // frame parameters
    const SOneSecondImages & firstSecond = m_imagesBySeconds.front();
    std::vector<char> vecForParams( firstSecond.images[ 0 ]->imageBytes.begin(), firstSecond.images[ 0 ]->imageBytes.end() );

    const cv::Mat decodedImage = cv::imdecode( vecForParams, 1 );

    m_state.imageProps.width = decodedImage.cols;
    m_state.imageProps.height = decodedImage.rows;

    return true;
}

ImageFromFile::SImage * ImageFromFile::loadImage( const std::string & _imagePath ){

    SImage * image = new SImage();

    // data ( std )
#if 0
    ifstream file( _imagePath, ios::binary | ios::ate );

    file.seekg( 0, ios::end );
    const ifstream::pos_type bytesCount = file.tellg();
    image->imageBytes.resize( bytesCount );

    file.seekg( 0, ios::beg );
    file.read( & image->imageBytes[ 0 ], bytesCount );
#else
    // data ( Qt )
    QImage img( QString(_imagePath.c_str()) );
    QBuffer buf( & image->imageBytes );
    buf.open( QIODevice::WriteOnly );
    img.save( & buf, "JPG" );
#endif

    // meta
    image->imageMetadata.first = image->imageBytes.data();
    image->imageMetadata.second = image->imageBytes.size();

    return image;
}

void ImageFromFile::tick(){

    // NOTE: idea is that we are moving within a 'second' by frame index switch

    // switching by real time
    static int64_t lastTimeImageSwitching = 0;
    if( (common_utils::getCurrentTimeMillisec() - lastTimeImageSwitching) < m_state.settings.imageCaptureIntervalMilllisec ){
        return;
    }
    lastTimeImageSwitching = common_utils::getCurrentTimeMillisec();

    // get actual image
    SOneSecondImages & second = m_imagesBySeconds[ m_currentSecondIdx ];

    if( m_currentFrameIdx < second.images.size() ){
        m_currentImageRef = second.images[ m_currentFrameIdx ];
        second.lastImageIdx = m_currentFrameIdx;
    }
    else{
        m_currentImageRef = second.images[ second.lastImageIdx ];
    }

    // move indexes
    m_currentFrameIdx++;
    if( m_currentFrameIdx == g_fps ){
        m_currentFrameIdx = 0;

        m_currentSecondIdx = ++m_currentSecondIdx % m_imagesBySeconds.size();
    }
}

std::pair<TConstDataPointer, TDataSize> ImageFromFile::getImageData(){

    std::pair<TConstDataPointer, TDataSize> out;
//    m_mutexImageRef.lock();

    // 1st version ( independent cycle )
//    out = m_currentImageRef->imageMetadata;
//    VS_LOG_INFO << "requested image file: " << m_currentImageRef->fileName << endl;

    // 2nd version ( on demand )
    SOneSecondImages & second = m_imagesBySeconds[ m_currentSecondIdx ];
    out = second.images[ m_currentFrameIdx ]->imageMetadata;

    // text overlay
    if( m_state.settings.statusOverlay ){
//        cv::imdecode( second.images[ m_currentFrameIdx ]->imageBytes, 1, & m_currentImage );
//        constexpr int fontScale = 1;
//        cv::putText( m_currentImage, "ONLINE [FPS 20.0]", cv::Point2f(30, m_state.imageProps.height - 30), cv::FONT_HERSHEY_TRIPLEX, fontScale, cv::Scalar(0, 255, 0, 0) );
//        m_currentImageBytes.clear();
//        cv::imencode( ".jpg", m_currentImage, m_currentImageBytes );
//        out.first = m_currentImageBytes.data();
//        out.second = m_currentImageBytes.size();
    }

    // move indexes
    m_currentFrameIdx++;
    if( m_currentFrameIdx == second.images.size() ){
        m_currentFrameIdx = 0;

        m_currentSecondIdx = ++m_currentSecondIdx % m_imagesBySeconds.size();
    }

//    m_mutexImageRef.unlock();




    return out;
}

SImageProperties ImageFromFile::getImageProperties(){

    return m_state.imageProps;
}





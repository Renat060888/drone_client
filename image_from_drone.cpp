
#include <QFile>
#include <QBuffer>
#include <QJsonParseError>
#include "from_ms_common/system/logger.h"

#include "image_from_drone.h"

using namespace std;

static constexpr const char * PRINT_HEADER = "ImgFromDrone:";

ImageFromDrone::ImageFromDrone()
    : m_frameDescr(nullptr)
{
    QObject::connect( & rfv, SIGNAL(lastFrameChanged(QByteArray &)),
                     this, SLOT(slotLastFrameChanged(QByteArray &))
                    );
}

ImageFromDrone::~ImageFromDrone(){


}

bool ImageFromDrone::init( const SInitSettings & _settings ){

    if( ! rfv.init() ){
        VS_LOG_ERROR << PRINT_HEADER << " rfv init failed" << endl;
        return false;
    }

    QFile rfconfig( _settings.configFilePath.c_str() );
    QJsonParseError jerror;
    QJsonDocument jdoc;
    QJsonObject jobj;
    if(rfconfig.open(QIODevice::ReadOnly))
        jdoc = QJsonDocument::fromJson( rfconfig.readAll(), &jerror );
	else{
		assert( false );
	}

    if(jerror.error == QJsonParseError::NoError) {
        jobj = jdoc.object();

        rfv.fromJson( jobj );
        VS_LOG_INFO << PRINT_HEADER << rfv.dump().toStdString() << endl;
    }
    else{
        VS_LOG_ERROR << PRINT_HEADER
                     << " rfv json file [" << _settings.configFilePath << "] parsing error, reason: " << jerror.errorString().toStdString()
                     << endl;
        return false;
    }

    if( ! rfv.startAsync() ){
        VS_LOG_ERROR << PRINT_HEADER << " rfv start async failed" << endl;
        return false;
    }
    
    rfv.setCalcFps( true );

	VS_LOG_INFO << PRINT_HEADER << " init success, config " << _settings.configFilePath << endl;
    return true;
}

void ImageFromDrone::slotLastFrameChanged( QByteArray & _frame ){

	VS_LOG_INFO << PRINT_HEADER << " frame, current fps: " << rfv.fps() << endl;
	
	

    m_mutexImageRef.lock();
    m_droneCurrentFrame = _frame;
    m_mutexImageRef.unlock();

	if( m_droneCurrentFrame.isEmpty() ){
		return;
	}
    
    if( ! m_frameDescr ){
		m_mutexImageDescrRef.lock();
        m_frameDescr = new OwlDeviceInputData::OwlDeviceFrameDescriptor();
        ( * m_frameDescr ) = rfv.parseFrameDescriptor( _frame );
        m_mutexImageDescrRef.unlock();
        
        
        VS_LOG_INFO << PRINT_HEADER << "try decode" << endl;
        const std::vector<char> vecForParams( m_droneCurrentFrame.begin(), m_droneCurrentFrame.end() );
        VS_LOG_INFO << PRINT_HEADER << "vector size: " << vecForParams.size() << endl;
		const cv::Mat decodedImage = cv::imdecode( vecForParams, 1 );
		VS_LOG_INFO << PRINT_HEADER << "decode success" << endl;

		m_widthViaOpenCV = decodedImage.cols;
		m_heightViaOpenCV = decodedImage.rows;
        
        VS_LOG_INFO << PRINT_HEADER << " first frame w" << m_frameDescr->frameWidth() << endl;
        VS_LOG_INFO << PRINT_HEADER << " first frame h" << m_frameDescr->frameHeight() << endl;
        
        m_signalFirstFrameFromDrone();
    }
    
}

std::pair<TConstDataPointer, TDataSize> ImageFromDrone::getImageData(){

    std::pair<TConstDataPointer, TDataSize> out;

    m_mutexImageRef.lock();
    out.first = m_droneCurrentFrame.data();
    out.second = m_droneCurrentFrame.size();

    // text overlay
    if( m_settings.statusOverlay ){
//        cv::imdecode( m_droneCurrentFrame, 1, & m_currentImage );
//        constexpr int fontScale = 1;
//        cv::putText( m_currentImage, "ONLINE [FPS 20.0]", cv::Point2f(30, m_frameDescr->frameHeight() - 30), cv::FONT_HERSHEY_TRIPLEX, fontScale, cv::Scalar(0, 255, 0, 0) );
//        m_currentImageBytes.clear();
//        cv::imencode( ".jpg", m_currentImage, m_currentImageBytes );
//        out.first = m_currentImageBytes.data();
//        out.second = m_currentImageBytes.size();
    }

    m_mutexImageRef.unlock();

    return out;
}

SImageProperties ImageFromDrone::getImageProperties(){

    SImageProperties out;

    m_mutexImageDescrRef.lock();
    out.width = m_widthViaOpenCV;
    out.height = m_heightViaOpenCV;
    m_mutexImageDescrRef.unlock();

    return out;
}








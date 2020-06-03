
#include <QFile>
#include <QBuffer>
#include <QJsonParseError>
#include "from_ms_common/system/logger.h"

#include "common_utils.h"
#include "image_from_drone.h"

using namespace std;

static constexpr const char * PRINT_HEADER = "ImgFromDrone:";

ImageFromDrone::ImageFromDrone()
    : m_frameDescr(nullptr)
    , m_trFrameAndTelemetryDump(nullptr)
    , m_shutdownCalled(false)
{
    QObject::connect( & rfv, SIGNAL(lastFrameChanged(QByteArray &)),
                     this, SLOT(slotLastFrameChanged(QByteArray &))
                    );
}

ImageFromDrone::~ImageFromDrone(){

    common_utils::threadShutdown( m_trFrameAndTelemetryDump );
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

    if( m_settings.dumpIncomingData ){
        m_trFrameAndTelemetryDump = new std::thread( & ImageFromDrone::threadFrameAndTelemetryDump, this );
    }

	VS_LOG_INFO << PRINT_HEADER << " init success, config " << _settings.configFilePath << endl;
    return true;
}

void ImageFromDrone::callbackSwitchOn( bool _on ){

    // TODO: do
}

void ImageFromDrone::threadFrameAndTelemetryDump(){

    while( ! m_shutdownCalled ){
        m_muDumpQueue.lock();

        while( ! m_dumpQueue.empty() ){
            SDumpData & toDump = m_dumpQueue.front();

            const int64_t timeMillisec = common_utils::getCurrentTimeMillisec();

            const string pathToSave1 = common_utils::timeMillisecToStr( timeMillisec ) + ".bin";
            QFile file( pathToSave1.c_str() );
            file.open( QIODevice::WriteOnly );
            file.write( toDump.frameBytes );
            file.close();

            const string pathToSave2 = common_utils::timeMillisecToStr( timeMillisec ) + ".json";
            QFile file2( pathToSave2.c_str() );
            file2.open( QIODevice::WriteOnly );
            file2.write( toDump.telemetryStr.toUtf8() );
            file2.close();

            m_dumpQueue.pop();
        }

        m_muDumpQueue.unlock();
        std::this_thread::sleep_for( chrono::milliseconds(100) );
    }
}

void ImageFromDrone::slotLastFrameChanged( QByteArray & _frame ){
	
    m_muCurrentFrame.lock();
    m_droneCurrentFrame = _frame;
    m_muCurrentFrame.unlock();

    if( m_settings.dumpIncomingData ){
        SDumpData toDump;
        toDump.frameBytes = _frame;
        toDump.telemetryStr = "";
        m_muDumpQueue.lock();
        m_dumpQueue.push( std::move(toDump) );
        m_muDumpQueue.unlock();
    }

	if( m_droneCurrentFrame.isEmpty() ){
		return;
	}
    
    if( ! m_frameDescr ){
		m_mutexImageDescrRef.lock();
        m_frameDescr = new OwlDeviceInputData::OwlDeviceFrameDescriptor();
        ( * m_frameDescr ) = rfv.parseFrameDescriptor( _frame );
        m_mutexImageDescrRef.unlock();
        
        const std::vector<char> vecForParams( m_droneCurrentFrame.begin(), m_droneCurrentFrame.end() );
		const cv::Mat decodedImage = cv::imdecode( vecForParams, 1 );

		m_widthViaOpenCV = decodedImage.cols;
		m_heightViaOpenCV = decodedImage.rows;
        
        m_signalFirstFrameFromDrone();
    }    
}

std::pair<TConstDataPointer, TDataSize> ImageFromDrone::getImageData(){

    std::pair<TConstDataPointer, TDataSize> out;

    m_muCurrentFrame.lock();
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

    m_muCurrentFrame.unlock();

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








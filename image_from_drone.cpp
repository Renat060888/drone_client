
#include <QFile>
#include <QBuffer>
#include <QJsonParseError>
#include "from_ms_common/system/logger.h"

#include "image_from_drone.h"

using namespace std;

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

    QFile rfconfig( _settings.configFilePath.c_str() );
    QJsonParseError jerror;
    QJsonDocument jdoc;
    QJsonObject jobj;

    rfv.init();

    if(rfconfig.open(QIODevice::ReadOnly))
        jdoc = QJsonDocument::fromJson( rfconfig.readAll(), &jerror );

    if(jerror.error == QJsonParseError::NoError) {
        jobj = jdoc.object();

        rfv.fromJson( jobj["videoClient"].toObject() );
    }

    rfv.startAsync();

    return true;
}

void ImageFromDrone::slotLastFrameChanged( QByteArray & _frame ){

    m_mutexImageRef.lock();
    m_droneCurrentFrame = _frame;
    m_mutexImageRef.unlock();

    m_mutexImageDescrRef.lock();
    if( ! m_frameDescr ){
        m_frameDescr = new OwlDeviceInputData::OwlDeviceFrameDescriptor();
        ( * m_frameDescr ) = rfv.parseFrameDescriptor( _frame );
    }
    m_mutexImageDescrRef.unlock();
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
    out.width = m_frameDescr->frameWidth();
    out.height = m_frameDescr->frameHeight();
    m_mutexImageDescrRef.unlock();

    return out;
}








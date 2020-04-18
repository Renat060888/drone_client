
#include <QFile>
#include <QBuffer>
#include <QJsonParseError>

#include "image_from_drone.h"

using namespace std;

ImageFromDrone::ImageFromDrone()
    : m_frameDescr(nullptr)
{

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

void ImageFromDrone::slotFrameChanged( QImage _frame ){

    m_mutexImageRef.lock();
    m_droneCurrentFrame = _frame;
    m_mutexImageRef.unlock();

    m_mutexImageDescrRef.lock();
    if( ! m_frameDescr ){
        m_frameDescr = new OwlDeviceInputData::OwlDeviceFrameDescriptor();

        QByteArray array;
        QBuffer buf( & array );
        buf.open( QIODevice::WriteOnly );
        _frame.save( & buf, "yourformat" );

        ( * m_frameDescr ) = rfv.parseFrameDescriptor( array );
    }
    m_mutexImageDescrRef.unlock();
}

std::pair<TConstDataPointer, TDataSize> ImageFromDrone::getImageData(){

    std::pair<TConstDataPointer, TDataSize> out;

    m_mutexImageRef.lock();
    out.first = m_droneCurrentFrame.data_ptr();
    out.second = m_droneCurrentFrame.byteCount();
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








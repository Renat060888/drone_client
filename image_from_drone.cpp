
#include <QFile>
#include <QJsonParseError>

#include "image_from_drone.h"

using namespace std;

ImageFromDrone::ImageFromDrone()
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

std::pair<TConstDataPointer, TDataSize> ImageFromDrone::getImageData(){

}

SImageProperties ImageFromDrone::getImageProperties(){

}

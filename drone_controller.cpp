
#include <QFile>
#include <QJsonParseError>

#include "drone_controller.h"

using namespace std;

DroneController::DroneController()
{

}

bool DroneController::init( const SInitSettings & _settings ){

    QFile rfconfig( _settings.configFilePath.c_str() );
    QJsonParseError jerror;
    QJsonDocument jdoc;
    QJsonObject jobj;

    rfc.init();

    if(rfconfig.open(QIODevice::ReadOnly))
        jdoc = QJsonDocument::fromJson( rfconfig.readAll(), &jerror );

    if(jerror.error == QJsonParseError::NoError) {
        jobj = jdoc.object();

        rfc.fromJson( jobj["controlClient"].toObject() );
    }

    rfc.startAsync();

    return true;
}

void DroneController::callbackSetTargetCoord( float lat, float lon, int alt ){

    rfc.setTargetCoord( lat, lon, alt );
}















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




    OwlDeviceInputData::State * state = rfc.currentState();

    OwlDeviceInputData::BoardPosition * boardPos = rfc.boardPos();

    OwlDeviceInputData::RollPitchZoomLens * lensParams = rfc.rpzLens();
    OwlDeviceInputData::RollPitchYawLineOfSight * lineOfSightParams = rfc.rpyLineOfSight();
    OwlDeviceInputData::LineOfSightCrossSurface * lineOfSightCrossSurface = rfc.lineOfSightCrossSurface();





}

void DroneController::callbackSetMode( EDroneMode _mode ){

    switch( _mode ){
    case EDroneMode::OBSERVATION : {
        rfc.setObservationMode();
        break;
    }
    case EDroneMode::HOLD : {
        rfc.setHoldCoordinateMode();
        break;
    }
    case EDroneMode::PILOT_WINDOW : {
        rfc.setPilotWindowMode();
        break;
    }
    case EDroneMode::POSITION : {
        rfc.setPositionMode();
        break;
    }
    case EDroneMode::STOW : {
        rfc.setStowMode();
        break;
    }
    default : {
        // TODO: catch an error
    }
    }
}

void DroneController::callbackSetAzimut( double _azimutDeg ){

    // TODO: 'get' ?
}

void DroneController::callbackSetWantAzimut( double _azimutAbsDeg ){

    rfc.setPositionAngle( _azimutAbsDeg, 0.0f );
}

void DroneController::callbackSetAzimutChangeRate( double _azimutIncDeg ){

    rfc.cameraRotation( _azimutIncDeg, 0.0f );
}

void DroneController::callbackSetElevation( double _elevationDeg ){

    // TODO: 'get' ?
}

void DroneController::callbackSetWantElevation( double _elevationAbsDeg ){

    rfc.setPositionAngle( 0.0f, _elevationAbsDeg );
}

void DroneController::callbackSetElevationChangeRate( double _elevationIncDeg ){

    rfc.cameraRotation( 0.0f, _elevationIncDeg );
}

void DroneController::callbackSetFocalLength( double _focalLengthMillimeter ){

//    rfc.se
}

void DroneController::callbackSetFocusChangeRate( double _rate ){

}

void DroneController::callbackSetZoomChangeRate( double _rate ){

}

void DroneController::callbackSetDiaphragmMode( ECameraDiaphragmMode _mode ){

}

void DroneController::callbackSetTxRxScale( double _scale ){

}

void DroneController::callbackSetShowAim( bool _show ){

}

void DroneController::callbackSetShowTelemetry( bool _show ){

}







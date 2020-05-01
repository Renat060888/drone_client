
#include <cmath>

#include <QFile>
#include <QJsonParseError>
#include "from_ms_common/system/logger.h"

#include "drone_controller.h"

using namespace std;

DroneController::DroneController()
    : m_shutdownCalled(false)
{
    QObject::connect( & rfc, SIGNAL(readResponseComplete(OwlDeviceInputData::OwlDeviceDataBase::DataType)),
                     this, SLOT(slotReadResponseComplete(OwlDeviceInputData::OwlDeviceDataBase::DataType))
                    );
    QObject::connect( & rfc, SIGNAL(currentStateChanged(OwlDeviceInputData::State *)),
                     this, SLOT(slotCurrentStateChanged(OwlDeviceInputData::State *))
                    );
    QObject::connect( & rfc, SIGNAL(boardPosChanged(OwlDeviceInputData::BoardPosition *)),
                     this, SLOT(slotBoardPosChanged(OwlDeviceInputData::BoardPosition *))
                    );
    QObject::connect( & rfc, SIGNAL(rpzLensChanged(OwlDeviceInputData::RollPitchZoomLens *)),
                     this, SLOT(slotRpzLensChanged(OwlDeviceInputData::RollPitchZoomLens *))
                    );
}

DroneController::~DroneController()
{

}

bool DroneController::init( const SInitSettings & _settings ){

    m_trCarrierImitation = new std::thread( & DroneController::threadCarrierImitation, this );

    QFile rfconfig( _settings.configFilePath.c_str() );
    QJsonParseError jerror;
    QJsonDocument jdoc;
    QJsonObject jobj;

    //
    if( ! rfc.init() ){
        return false;
    }

    //
    if(rfconfig.open(QIODevice::ReadOnly))
        jdoc = QJsonDocument::fromJson( rfconfig.readAll(), &jerror );

    if(jerror.error == QJsonParseError::NoError) {
        jobj = jdoc.object();

        rfc.fromJson( jobj["controlClient"].toObject() );
    }

    //
    if( ! rfc.startAsync() ){
        return false;
    }

    return true;
}

void DroneController::threadCarrierImitation(){

    while( ! m_shutdownCalled ){

        if( m_settings.movingImitationEnable ){
            movingImitation();
        }

        checkPings();

        std::this_thread::sleep_for( chrono::milliseconds(100) );
    }
}

void DroneController::movingImitation(){

    static double t = 0;
    t += 1;

    const double lat = 33.51 + ::cos( t / 100 );
    const double lon = 53.15 + ::sin( t / 100 );
    constexpr double alt = 100;

    for( IDroneStateObserver * observer : m_observers ){
        observer->callbackBoardPositionChanged( lat, lon, alt );
    }

//    double angleDeg = 0.0f;
//    const double centerLatDeg = 60.0f;
//    const double centerLonDeg = 29.0f;
//    const double latDeg = centerLatDeg + ::sin( angleDeg / 360.0f );
//    const double lonDeg = centerLonDeg + ::cos( angleDeg / 360.0f );
}

void DroneController::checkPings(){

    static int64_t lastPingMillisec = 0;

}

// ----------------------------------------------------------------------
// Drone events
// ----------------------------------------------------------------------
void DroneController::slotReadResponseComplete( OwlDeviceInputData::OwlDeviceDataBase::DataType _cmd ){

    // NOTE: not necessarily
}

void DroneController::slotCurrentStateChanged( OwlDeviceInputData::State * _currentState ){


}

void DroneController::slotBoardPosChanged( OwlDeviceInputData::BoardPosition * _boardPos ){

    if( ! m_settings.movingImitationEnable ){
        for( IDroneStateObserver * observer : m_observers ){
            observer->callbackBoardPositionChanged( _boardPos->latitude(), _boardPos->longitude(), _boardPos->altitude() );
        }
    }
}

void DroneController::slotRpzLensChanged( OwlDeviceInputData::RollPitchZoomLens * _rpzLens ){

    if( ! m_settings.movingImitationEnable ){
        for( IDroneStateObserver * observer : m_observers ){
            observer->callbackCameraPositionChanged( _rpzLens->pitch(), _rpzLens->roll(), _rpzLens->zoom() );
        }
    }
}

// ----------------------------------------------------------------------
// GUI signals
// ----------------------------------------------------------------------
void DroneController::callbackSetTargetCoord( float lat, float lon, int alt ){

    VS_LOG_INFO << "callbackSetTargetCoord:" << lat << " " << lon << " " << alt << endl;
    rfc.setTargetCoord( lat, lon, alt );
}

void DroneController::callbackSetMode( EDroneMode _mode ){

    VS_LOG_INFO << "callbackSetMode:" << ((int)_mode) << endl;

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
        VS_LOG_ERROR << "unknown drone mode (int): " << ((int)_mode) << endl;
    }
    }
}

void DroneController::callbackSetAzimut( double _azimutDeg ){

    // TODO: 'get' ?
    VS_LOG_INFO << "callbackSetAzimut:" << _azimutDeg << endl;
}

void DroneController::callbackSetWantAzimut( double _azimutAbsDeg ){

    VS_LOG_INFO << "callbackSetWantAzimut:" << _azimutAbsDeg << endl;
    rfc.setPositionAngle( _azimutAbsDeg, 0.0f );
}

void DroneController::callbackSetAzimutChangeRate( double _azimutIncDeg ){

    VS_LOG_INFO << "callbackSetAzimutChangeRate:" << _azimutIncDeg << endl;
    rfc.cameraRotation( _azimutIncDeg, 0.0f );
}

void DroneController::callbackSetElevation( double _elevationDeg ){

    // TODO: 'get' ?
    VS_LOG_INFO << "callbackSetElevation:" << _elevationDeg << endl;
}

void DroneController::callbackSetWantElevation( double _elevationAbsDeg ){

    VS_LOG_INFO << "callbackSetWantElevation:" << _elevationAbsDeg << endl;
    rfc.setPositionAngle( 0.0f, _elevationAbsDeg );
}

void DroneController::callbackSetElevationChangeRate( double _elevationIncDeg ){

    VS_LOG_INFO << "callbackSetElevationChangeRate:" << _elevationIncDeg << endl;
    rfc.cameraRotation( 0.0f, _elevationIncDeg );
}

void DroneController::callbackSetFocalLength( double _focalLengthMillimeter ){

    // TODO: ?
    VS_LOG_INFO << "callbackSetFocalLength:" << _focalLengthMillimeter << endl;
}

void DroneController::callbackSetFocusChangeRate( double _rate ){

    VS_LOG_INFO << "callbackSetFocusChangeRate:" << _rate << endl;

    static double lastRate = 0.0f;

    if( _rate > lastRate ){
        rfc.cameraFunctionFocusIn();
        rfc.cameraFunctionStopFocus();
    }
    else{
        rfc.cameraFunctionFocusOut();
        rfc.cameraFunctionStopFocus();
    }

    lastRate = _rate;
}

void DroneController::callbackSetZoomChangeRate( double _rate ){

    VS_LOG_INFO << "callbackSetZoomChangeRate:" << ((int)_rate) << endl;

    static double lastRate = 0.0f;

    if( _rate > lastRate ){
        rfc.cameraFunctionZoomIn();
        rfc.cameraFunctionStopZoom();
    }
    else{
        rfc.cameraFunctionZoomOut();
        rfc.cameraFunctionStopZoom();
    }

    lastRate = _rate;
}

void DroneController::callbackSetDiaphragmMode( ECameraDiaphragmMode _mode ){

    VS_LOG_INFO << "callbackSetDiaphragmMode:" << ((int)_mode) << endl;

    switch( _mode ){
    case ECameraDiaphragmMode::AUTO : {
        rfc.cameraFunctionDiaphragmOpen( false);
        break;
    }
    case ECameraDiaphragmMode::OPEN : {
        rfc.cameraFunctionDiaphragmOpen( true  );
        break;
    }
    default : {
        VS_LOG_ERROR << "unknown diaphragm mode (int): " << ((int)_mode) << endl;
    }
    }
}

void DroneController::callbackSetTxRxScale( double _scale ){

    VS_LOG_INFO << "callbackSetTxRxScale:" << _scale << endl;
    rfc.setTxrxScale( _scale );
}

void DroneController::callbackSetShowAim( bool _show ){

    VS_LOG_INFO << "callbackSetShowAim:" << _show << endl;
    rfc.cameraFunctionShowAim( _show );
}

void DroneController::callbackSetShowTelemetry( bool _show ){

    VS_LOG_INFO << "callbackSetShowTelemetry:" << _show << endl;
    rfc.cameraFunctionShowTelemetry( _show );
}

void DroneController::addObserver( IDroneStateObserver * _observer ){
    m_observers.push_back( _observer );
}





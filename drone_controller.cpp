
#include <cmath>

#include <QFile>
#include <QJsonParseError>
#include "from_ms_common/system/logger.h"
#include "from_ms_common/common/ms_common_utils.h"

#include "drone_controller.h"

using namespace std;

static constexpr const char * PRINT_HEADER = "DroneController:";

DroneController::DroneController()
    : m_shutdownCalled(false)
    , m_azimutAbsDeg(0.0)
    , m_azimutIncDeg(0.0)
    , m_elevationAbsDeg(0.0)
    , m_elevationIncDeg(0.0)
    , m_lastPingValFromDrone(false)
    , m_boardOnline(false)
    , m_lastPingAtMillisec(0)
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

    m_settings = _settings;

    // init
    if( ! rfc.init() ){
        VS_LOG_ERROR << PRINT_HEADER << " rfc init failed" << endl;
        return false;
    }

    // read config
    QFile rfconfig( _settings.configFilePath.c_str() );
    QJsonParseError jerror;
    QJsonDocument jdoc;
    QJsonObject jobj;

    if(rfconfig.open(QIODevice::ReadOnly))
        jdoc = QJsonDocument::fromJson( rfconfig.readAll(), &jerror );
    else{
		assert( false );
	}

    if( jerror.error == QJsonParseError::NoError ){
        jobj = jdoc.object();
        rfc.fromJson( jobj );
    }
    else{
        VS_LOG_ERROR << PRINT_HEADER
                     << " rfc json file [" << _settings.configFilePath << "] parsing error, reason: " << jerror.errorString().toStdString()
                     << endl;
        return false;
    }

    // start
    if( ! rfc.startAsync() ){
        VS_LOG_ERROR << PRINT_HEADER << " rfc start async failed" << endl;
        return false;
    }

    m_trMaintenance = new std::thread( & DroneController::threadMaintenance, this );
        
	VS_LOG_INFO << PRINT_HEADER << " init success, config " << _settings.configFilePath << endl;
    return true;
}

void DroneController::threadMaintenance(){

    while( ! m_shutdownCalled ){

        if( m_settings.movingImitationEnable ){
            movingImitation();
        }

        checkPings();

        std::this_thread::sleep_for( chrono::milliseconds(10) );
    }
}

void DroneController::movingImitation(){

    // board
    static double t = 0;
    t += 1;

    const double lat = 33.51 + ::cos( t / 100 );
    const double lon = 53.15 + ::sin( t / 100 );
    constexpr double alt = 100;

    for( IDroneStateObserver * observer : m_observers ){
        observer->callbackBoardPositionChanged( lat, lon, alt );
    }

    // camera
    static int64_t lastLensChangeAtMillisec = 0;

    if( (common_utils::getCurrentTimeMillisec() - lastLensChangeAtMillisec) > 1000LL ){
        lastLensChangeAtMillisec = common_utils::getCurrentTimeMillisec();

        for( IDroneStateObserver * observer : m_observers ){
            observer->callbackCameraPositionChanged( ::rand() % 90, ::rand() % 45, ::rand() % 255 );
        }
    }

//    double angleDeg = 0.0f;
//    const double centerLatDeg = 60.0f;
//    const double centerLonDeg = 29.0f;
//    const double latDeg = centerLatDeg + ::sin( angleDeg / 360.0f );
//    const double lonDeg = centerLonDeg + ::cos( angleDeg / 360.0f );
}

void DroneController::checkPings(){

    // detect ping
    if( m_lastPingValFromDrone ^ rfc.pingIndicator() ){
        m_lastPingValFromDrone = rfc.pingIndicator();

        m_lastPingAtMillisec = common_utils::getCurrentTimeMillisec();
    }

    if( m_boardOnline ){
        // check for offline
        if( (common_utils::getCurrentTimeMillisec() - m_lastPingAtMillisec) > 10000LL ){
            VS_LOG_WARN << PRINT_HEADER << " drone goes offline" << endl;

            m_boardOnline = false;
            for( IDroneStateObserver * observer : m_observers ){
                observer->callbackBoardOnline( false );
            }
        }
    }
    else{
        // check for online
        if( (common_utils::getCurrentTimeMillisec() - m_lastPingAtMillisec) < 10000LL ){
            VS_LOG_INFO << PRINT_HEADER << " drone back to online" << endl;

            m_boardOnline = true;
            for( IDroneStateObserver * observer : m_observers ){
                observer->callbackBoardOnline( true );
            }
        }
    }
}

// ----------------------------------------------------------------------
// Drone events
// ----------------------------------------------------------------------
void DroneController::slotReadResponseComplete( OwlDeviceInputData::OwlDeviceDataBase::DataType _cmd ){

    // NOTE: not necessarily
}

void DroneController::slotCurrentStateChanged( OwlDeviceInputData::State * _currentState ){

    // TODO: do ?
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

// mode
void DroneController::callbackSetMode( EDroneMode _mode ){

    VS_LOG_INFO << PRINT_HEADER << " callbackSetMode:" << ((int)_mode) << endl;

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
        VS_LOG_ERROR << PRINT_HEADER << " unknown drone mode (int): " << ((int)_mode) << endl;
    }
    }
}

// position control
void DroneController::callbackSetWantAzimut( double _azimutAbsDeg ){

    VS_LOG_INFO << PRINT_HEADER << " callbackSetWantAzimut: " << _azimutAbsDeg << " abs elev: " << m_elevationAbsDeg << endl;

    // NOTE: values swapped
    rfc.setPositionAngle( m_elevationAbsDeg, _azimutAbsDeg );
    m_azimutAbsDeg = _azimutAbsDeg;
}

void DroneController::callbackSetWantElevation( double _elevationAbsDeg ){

    VS_LOG_INFO << PRINT_HEADER << " callbackSetWantElevation: " << _elevationAbsDeg << " abs azim: " << m_azimutAbsDeg << endl;

    // NOTE: values swapped
    rfc.setPositionAngle( _elevationAbsDeg, m_azimutAbsDeg );
    m_elevationAbsDeg = _elevationAbsDeg;
}

void DroneController::callbackSetAzimutChangeRate( double _azimutIncDeg ){

    VS_LOG_INFO << PRINT_HEADER << " callbackSetAzimutChangeRate: " << _azimutIncDeg << " inc elev: " << m_elevationIncDeg << endl;

    rfc.cameraRotation( _azimutIncDeg, m_elevationIncDeg );
    m_azimutIncDeg = _azimutIncDeg;
}

void DroneController::callbackSetElevationChangeRate( double _elevationIncDeg ){

    VS_LOG_INFO << PRINT_HEADER << " callbackSetElevationChangeRate: " << _elevationIncDeg << " inc azim: " << m_azimutIncDeg << endl;

    rfc.cameraRotation( m_azimutIncDeg, _elevationIncDeg );
    m_elevationIncDeg = _elevationIncDeg;
}

// optic control
void DroneController::callbackSetFocusChangeRate( double _rate ){

    if( _rate > 0.0f ){
        rfc.cameraFunctionFocusIn();
        VS_LOG_INFO << PRINT_HEADER << " callbackSetFocusChangeRate (In):" << _rate << endl;
    }
    else if( _rate < 0.0f ){
        rfc.cameraFunctionFocusOut();
        VS_LOG_INFO << PRINT_HEADER << " callbackSetFocusChangeRate (Out):" << _rate << endl;
    }
    else if( _rate == 0.0f ){
        rfc.cameraFunctionStopFocus();
        VS_LOG_INFO << PRINT_HEADER << " callbackSetFocusChangeRate (Stop):" << _rate << endl;
    }
    else{
        VS_LOG_WARN << PRINT_HEADER << " wtf with focus?" << endl;
    }
}

void DroneController::callbackSetZoomChangeRate( double _rate ){

    if( _rate > 0.0f ){
        rfc.cameraFunctionZoomIn();
        VS_LOG_INFO << PRINT_HEADER << " callbackSetZoomChangeRate (In):" << _rate << endl;
    }
    else if( _rate < 0.0f ){
        rfc.cameraFunctionZoomOut();
        VS_LOG_INFO << PRINT_HEADER << " callbackSetZoomChangeRate (Out):" << _rate << endl;
    }
    else if( _rate == 0.0f ){
        rfc.cameraFunctionStopZoom();
        VS_LOG_INFO << PRINT_HEADER << " callbackSetZoomChangeRate (Stop):" << _rate << endl;
    }
    else{
        VS_LOG_WARN << PRINT_HEADER << " wtf with zoom?" << endl;
    }
}

void DroneController::callbackSetDiaphragmMode( ECameraDiaphragmMode _mode ){

    VS_LOG_INFO << PRINT_HEADER << " callbackSetDiaphragmMode:" << ((int)_mode) << endl;

    switch( _mode ){
    case ECameraDiaphragmMode::AUTO : {
        rfc.cameraFunctionDiaphragmOpen( false );
        break;
    }
    case ECameraDiaphragmMode::OPEN : {
        rfc.cameraFunctionDiaphragmOpen( true  );
        break;
    }
    default : {
        VS_LOG_ERROR << PRINT_HEADER << " unknown diaphragm mode (int): " << ((int)_mode) << endl;
    }
    }
}

// service
void DroneController::callbackStartVideoStream( bool _start ){

    if( _start ){
        rfc.startStream();
    }
    else{
        rfc.stopStream();
    }
}

void DroneController::callbackSetTxRxScale( double _scale ){

    VS_LOG_INFO << PRINT_HEADER << " callbackSetTxRxScale:" << _scale << endl;
    rfc.setTxrxScale( _scale );
}

void DroneController::callbackSetShowAim( bool _show ){

    VS_LOG_INFO << PRINT_HEADER << " callbackSetShowAim:" << _show << endl;
    rfc.cameraFunctionShowAim( _show );
}

void DroneController::callbackSetShowTelemetry( bool _show ){

    VS_LOG_INFO << PRINT_HEADER << " callbackSetShowTelemetry:" << _show << endl;
    rfc.cameraFunctionShowTelemetry( _show );
}

// ... ?
void DroneController::callbackSetTargetCoord( float lat, float lon, int alt ){

    VS_LOG_INFO << PRINT_HEADER << " callbackSetTargetCoord:" << lat << " " << lon << " " << alt << endl;
    rfc.setTargetCoord( lat, lon, alt );
}

void DroneController::addObserver( IDroneStateObserver * _observer ){
    m_observers.push_back( _observer );
}






#include <cmath>

#include <QFile>
#include <QJsonParseError>
#include "from_ms_common/system/logger.h"
#include "from_ms_common/common/ms_common_utils.h"

#include "drone_controller.h"

using namespace std;

static constexpr const char * PRINT_HEADER = "DroneController:";
static constexpr double ANGLES_IN_ZOOM_UNIT = ( 57.8 - 2.0 ) / 255.0;
static constexpr int ZOOM_STEP = 2;

DroneController::DroneController()
    : m_shutdownCalled(false)
    , m_azimutAbsDeg(0.0)
    , m_azimutIncDeg(0.0)
    , m_elevationAbsDeg(0.0)
    , m_elevationIncDeg(0.0)
    , m_lastPingValFromDrone(false)
    , m_boardOnline(false)
    , m_lastPingAtMillisec(0)
    , m_imitatedCameraPitch(0)
    , m_imitatedCameraRoll(0)
    , m_imitateCameraZoom(0)
    , m_requestedCameraZoom(0)
    , m_requestedCameraPitch(0)
    , m_requestedCameraRoll(0)
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

        if( m_settings.imitationEnable ){
            droneActivityImitation();
        }
        else{
            checkPings();
        }

        std::this_thread::sleep_for( chrono::milliseconds(10) );
    }
}

void DroneController::droneActivityImitation(){

    // board
    static int64_t lastMoveChangeAtMillisec = 0;

    if( (common_utils::getCurrentTimeMillisec() - lastMoveChangeAtMillisec) > 100LL ){
        lastMoveChangeAtMillisec = common_utils::getCurrentTimeMillisec();

        static double t = 0;
        t += 1;

        const double lat = 33.51 + ::cos( t / 100 );
        const double lon = 53.15 + ::sin( t / 100 );
        constexpr double alt = 100;

        for( IDroneStateObserver * observer : m_observers ){
            observer->callbackBoardPositionChanged( lat, lon, alt );
        }
    }

    // camera
#if 1
    static int64_t lastLensChangeAtMillisec = 0;

    if( (common_utils::getCurrentTimeMillisec() - lastLensChangeAtMillisec) > 100LL ){
        lastLensChangeAtMillisec = common_utils::getCurrentTimeMillisec();

        if( (int)m_imitatedCameraPitch != (int)m_requestedCameraPitch  ){
            if( (int)m_imitatedCameraPitch < (int)m_requestedCameraPitch ){
                VS_LOG_INFO << PRINT_HEADER << " camera elevation +1" << endl;
                m_imitatedCameraPitch += 1;
            }
            else if( (int)m_imitatedCameraPitch > (int)m_requestedCameraPitch ){
                VS_LOG_INFO << PRINT_HEADER << " camera elevation -1" << endl;
                m_imitatedCameraPitch -= 1;
            }

            for( IDroneStateObserver * observer : m_observers ){
                observer->callbackCameraPositionChanged( m_imitatedCameraPitch, m_imitatedCameraRoll, 0, m_imitateCameraZoom );
            }
        }

        if( (int)m_imitatedCameraRoll != (int)m_requestedCameraRoll ){
            if( (int)m_imitatedCameraRoll < (int)m_requestedCameraRoll ){
                VS_LOG_INFO << PRINT_HEADER << " camera azimut +1" << endl;
                m_imitatedCameraRoll += 1;
            }
            else if( (int)m_imitatedCameraRoll > (int)m_requestedCameraRoll ){
                VS_LOG_INFO << PRINT_HEADER << " camera azimut -1" << endl;
                m_imitatedCameraRoll -= 1;
            }

            for( IDroneStateObserver * observer : m_observers ){
                observer->callbackCameraPositionChanged( m_imitatedCameraPitch, m_imitatedCameraRoll, 0, m_imitateCameraZoom );
            }
        }

        if( m_imitateCameraZoom != m_requestedCameraZoom ){
            if( m_imitateCameraZoom < m_requestedCameraZoom ){
                VS_LOG_INFO << PRINT_HEADER << " camera zoom +" << ZOOM_STEP << " units" << endl;
                m_imitateCameraZoom += ZOOM_STEP;
            }
            else if( m_imitateCameraZoom > m_requestedCameraZoom ){
                VS_LOG_INFO << PRINT_HEADER << " camera zoom -" << ZOOM_STEP << " units" << endl;
                m_imitateCameraZoom -= ZOOM_STEP;
            }

            const double fov = ( ANGLES_IN_ZOOM_UNIT * m_imitateCameraZoom ) + 2.0f;
            VS_LOG_INFO << PRINT_HEADER << " FOV: " << fov << endl;
            for( IDroneStateObserver * observer : m_observers ){
                observer->callbackCameraFOVChanged( fov );
            }
        }
    }
#endif

    // online
    static const int64_t ONLINE_ACTIVITY_INTERVAL_MILLISEC = 1000 * 30 * 1;
    static const int64_t OFFLINE_INACIVITY_INTERVAL_MILLISEC = 1000 * 10;
    static int64_t onlineAtMillisec = 0;
    static int64_t offlineAtMillisec = common_utils::getCurrentTimeMillisec(); // begin from offline mode

    if( m_boardOnline ){
        if( (common_utils::getCurrentTimeMillisec() - onlineAtMillisec) > ONLINE_ACTIVITY_INTERVAL_MILLISEC ){
            m_boardOnline = false;
            offlineAtMillisec = common_utils::getCurrentTimeMillisec();

            for( IDroneStateObserver * observer : m_observers ){
                observer->callbackBoardOnline( false );
            }

            VS_LOG_INFO << "offline: " << common_utils::getCurrentDateTimeStr() << endl;
        }
    }
    else{
        if( (common_utils::getCurrentTimeMillisec() - offlineAtMillisec) > OFFLINE_INACIVITY_INTERVAL_MILLISEC ){
            m_boardOnline = true;
            onlineAtMillisec = common_utils::getCurrentTimeMillisec();

            for( IDroneStateObserver * observer : m_observers ){
                observer->callbackBoardOnline( true );
            }

            VS_LOG_INFO << "online: " << common_utils::getCurrentDateTimeStr() << endl;
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

    if( ! m_settings.imitationEnable ){
        for( IDroneStateObserver * observer : m_observers ){
            observer->callbackBoardPositionChanged( _boardPos->latitude(), _boardPos->longitude(), _boardPos->altitude() );
        }
    }
}

void DroneController::slotRpzLensChanged( OwlDeviceInputData::RollPitchZoomLens * _rpzLens ){

    if( ! m_settings.imitationEnable ){
        for( IDroneStateObserver * observer : m_observers ){
            observer->callbackCameraPositionChanged( _rpzLens->pitch(), _rpzLens->roll(), 0, _rpzLens->zoom() );
        }
    }
}

// ----------------------------------------------------------------------
// GUI signals
// ----------------------------------------------------------------------

// system
void DroneController::callbackSwitchOn( bool _on ){

    // TODO: do
}

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
    case EDroneMode::HOLD_TO_AIM : {
        rfc.setHoldCurrentCoordinateMode();
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

    if( m_settings.imitationEnable ){
        m_requestedCameraRoll = _azimutAbsDeg;
    }

    // NOTE: values swapped
    rfc.setPositionAngle( m_elevationAbsDeg, _azimutAbsDeg );
    m_azimutAbsDeg = _azimutAbsDeg;
}

void DroneController::callbackSetWantElevation( double _elevationAbsDeg ){

    VS_LOG_INFO << PRINT_HEADER << " callbackSetWantElevation: " << _elevationAbsDeg << " abs azim: " << m_azimutAbsDeg << endl;

    if( m_settings.imitationEnable ){
        m_requestedCameraPitch = _elevationAbsDeg;
    }

    // NOTE: values swapped
    rfc.setPositionAngle( _elevationAbsDeg, m_azimutAbsDeg );
    m_elevationAbsDeg = _elevationAbsDeg;    
}

void DroneController::callbackSetAzimutChangeRate( double _azimutIncDeg ){

    VS_LOG_INFO << PRINT_HEADER << " callbackSetAzimutChangeRate: " << _azimutIncDeg << " inc elev: " << m_elevationIncDeg << endl;

    if( m_settings.imitationEnable ){
        if( _azimutIncDeg > 0 ){
            m_requestedCameraRoll += 0.5f;
        }
        else{
            m_requestedCameraRoll -= 0.5f;
        }
    }

    rfc.cameraRotation( _azimutIncDeg, m_elevationIncDeg );
    m_azimutIncDeg = _azimutIncDeg;
}

void DroneController::callbackSetElevationChangeRate( double _elevationIncDeg ){

    VS_LOG_INFO << PRINT_HEADER << " callbackSetElevationChangeRate: " << _elevationIncDeg << " inc azim: " << m_azimutIncDeg << endl;

    if( m_settings.imitationEnable ){
        if( _elevationIncDeg > 0 ){
            m_requestedCameraPitch += 0.5f;
        }
        else{
            m_requestedCameraPitch -= 0.5f;
        }
    }

    rfc.cameraRotation( m_azimutIncDeg, _elevationIncDeg );
    m_elevationIncDeg = _elevationIncDeg;
}

// optic control
void DroneController::callbackSetFocusChangeRate( double _rate ){

    if( _rate > 0.0f ){
        rfc.cameraFunctionFocusIn();
        VS_LOG_INFO << PRINT_HEADER << " callbackSetFocusChangeRate (In): " << _rate << endl;
    }
    else if( _rate < 0.0f ){
        rfc.cameraFunctionFocusOut();
        VS_LOG_INFO << PRINT_HEADER << " callbackSetFocusChangeRate (Out): " << _rate << endl;
    }
    else if( _rate == 0.0f ){
        rfc.cameraFunctionStopFocus();
        VS_LOG_INFO << PRINT_HEADER << " callbackSetFocusChangeRate (Stop): " << _rate << endl;
    }
    else{
        VS_LOG_WARN << PRINT_HEADER << " wtf with focus?" << endl;
    }
}

void DroneController::callbackSetZoomChangeRate( double _rate ){

    if( m_settings.imitationEnable ){
        if( _rate > 0.0f ){
            if( m_requestedCameraZoom < 255 ){
                m_requestedCameraZoom += ZOOM_STEP;
            }
        }
        else{
            if( m_requestedCameraZoom > 0 ){
                m_requestedCameraZoom -= ZOOM_STEP;
            }
        }
    }

    if( _rate > 0.0f ){
        rfc.cameraFunctionZoomIn();
        VS_LOG_INFO << PRINT_HEADER << " callbackSetZoomChangeRate (In): " << _rate << endl;
    }
    else if( _rate < 0.0f ){
        rfc.cameraFunctionZoomOut();
        VS_LOG_INFO << PRINT_HEADER << " callbackSetZoomChangeRate (Out): " << _rate << endl;
    }
    else if( _rate == 0.0f ){
        rfc.cameraFunctionStopZoom();
        VS_LOG_INFO << PRINT_HEADER << " callbackSetZoomChangeRate (Stop): " << _rate << endl;
    }
    else{
        VS_LOG_WARN << PRINT_HEADER << " wtf with zoom?" << endl;
    }
}

void DroneController::callbackSetDiaphragmMode( ECameraDiaphragmMode _mode ){

    VS_LOG_INFO << PRINT_HEADER << " callbackSetDiaphragmMode: " << ((int)_mode) << endl;

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

    VS_LOG_INFO << PRINT_HEADER << " callbackStartVideoStream: " << _start << endl;

    if( _start ){
        rfc.startStream();
    }
    else{
        rfc.stopStream();
    }
}

void DroneController::callbackSetTxRxScale( double _scale ){

    VS_LOG_INFO << PRINT_HEADER << " callbackSetTxRxScale: " << _scale << endl;
    rfc.setTxrxScale( _scale );
}

void DroneController::callbackSetShowAim( bool _show ){

    VS_LOG_INFO << PRINT_HEADER << " callbackSetShowAim: " << _show << endl;
    rfc.cameraFunctionShowAim( _show );
}

void DroneController::callbackSetShowTelemetry( bool _show ){

    VS_LOG_INFO << PRINT_HEADER << " callbackSetShowTelemetry: " << _show << endl;
    rfc.cameraFunctionShowTelemetry( _show );
}

// ... ?
void DroneController::callbackSetTargetCoord( float lat, float lon, int alt ){

    VS_LOG_INFO << PRINT_HEADER << " callbackSetTargetCoord: " << lat << " " << lon << " " << alt << endl;
    rfc.setTargetCoord( lat, lon, alt );
}

void DroneController::addObserver( IDroneStateObserver * _observer ){
    m_observers.push_back( _observer );
}





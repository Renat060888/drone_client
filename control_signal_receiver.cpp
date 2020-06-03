
#include "from_ms_common/system/logger.h"

#include "common_utils.h"
#include "control_signal_receiver.h"

using namespace std;

static constexpr const char * PRINT_HEADER = "ObjreprSignalReceiver:";

static IControlSignalsObserver::EDroneMode convertDroneModeFromStr( const string & _str ){

    if( "observation" == _str ){ return IControlSignalsObserver::EDroneMode::OBSERVATION; }
    else if( "hold" == _str ){ return IControlSignalsObserver::EDroneMode::HOLD; }
    else if( "hold_to_aim" == _str ){ return IControlSignalsObserver::EDroneMode::HOLD_TO_AIM; }
    else if( "pilot_window" == _str ){ return IControlSignalsObserver::EDroneMode::PILOT_WINDOW; }
    else if( "stow" == _str ){ return IControlSignalsObserver::EDroneMode::STOW; }
    else if( "position" == _str ){ return IControlSignalsObserver::EDroneMode::POSITION; }
    else{ return IControlSignalsObserver::EDroneMode::UNDEFINED; }
}

static IControlSignalsObserver::ECameraDiaphragmMode convertDiaphragmModeFromStr( const string & _str ){

    if( "open" == _str ){ return IControlSignalsObserver::ECameraDiaphragmMode::OPEN; }
    else if( "auto" == _str ){ return IControlSignalsObserver::ECameraDiaphragmMode::AUTO; }
    else{ return IControlSignalsObserver::ECameraDiaphragmMode::UNDEFINED; }
}

ControlSignalReceiver::ControlSignalReceiver()
    : m_holdPointObjectId(0)
{

}

bool ControlSignalReceiver::init( const SInitSettings & _settings ){
#ifdef OBJREPR_LIBRARY_EXIST
    // bind to carrier
    objrepr::SpatialObjectPtr carrierObject = objrepr::RepresentationServer::instance()->objectManager()->getObject( _settings.carrierObjectId );
    if( ! carrierObject ){
        VS_LOG_ERROR << PRINT_HEADER << " carrier object with id " << _settings.carrierObjectId << " is not found" << endl;
        return false;
    }
    m_carrierObject = carrierObject;

    // bind to camera
    objrepr::SpatialObjectPtr cameraObject = objrepr::RepresentationServer::instance()->objectManager()->getObject( _settings.cameraObjectId );
    if( ! cameraObject ){
        VS_LOG_ERROR << PRINT_HEADER << " camera object with id " << _settings.cameraObjectId << " is not found" << endl;
        return false;
    }
    m_cameraObject = cameraObject;

    // listen attrs changing
    objrepr::DynamicAttributeMapPtr attrMap = cameraObject->dynamicAttrMap();

    attrMap->approvePending.connect( boost::bind( & ControlSignalReceiver::callbackApprovePending, this, _1 ) );
    attrMap->attrUpdated.connect( boost::bind( & ControlSignalReceiver::callbackAttrUpdated, this, _1 ) );
    attrMap->reqCompleted.connect( boost::bind( & ControlSignalReceiver::callbackRequestCompleted, this, _1 ) );
    attrMap->reqFailed.connect( boost::bind( & ControlSignalReceiver::callbackRequestFailed, this, _1 ) );

    attrMap->getHold();
    m_attrMap = attrMap;

    // object change listening
    objrepr::RepresentationServer::instance()->objectManager()->objectUpdated.connect( boost::bind( & ControlSignalReceiver::callbackObjectUpdated, this, _1 ) );
#endif

    return true;
}

// drone state callbacks
void ControlSignalReceiver::callbackBoardPositionChanged( double _lat, double _lon, double _alt ){
#ifdef OBJREPR_LIBRARY_EXIST
    objrepr::GeoCoord coords( _lat, _lon, _alt );
    objrepr::CoordTransform::instance()->EPSG4326_EPSG4978( & coords );

    m_carrierObject->setTemporalState( objrepr::SpatialObject::TemporalState::TS_Active );
    if( ! m_carrierObject->changePoint( 0, 0, coords ) ){
        VS_LOG_WARN << PRINT_HEADER << " failed to change point of objrepr-object: " << m_carrierObject->name() << endl;
    }
    m_carrierObject->push();

    cout << _lat << " " << _lon << " " << _alt << endl;
#endif
}

void ControlSignalReceiver::callbackBoardOnline( bool _online ){
#ifdef OBJREPR_LIBRARY_EXIST
    objrepr::DynBooleanAttributePtr attr1 = boost::dynamic_pointer_cast<objrepr::DynBooleanAttribute>( m_attrMap->getAttr("online") );
    bool rt = attr1->setValue( _online );
    attr1->pushAsync();
#endif
}

void ControlSignalReceiver::callbackCameraPositionChanged( double _pitch, double _roll, double _yaw, double _zoom ){
#ifdef OBJREPR_LIBRARY_EXIST
    objrepr::DynRealAttributePtr attr1 = boost::dynamic_pointer_cast<objrepr::DynRealAttribute>( m_attrMap->getAttr("elevation") );
    bool rt = attr1->setValue( _pitch );
    attr1->pushAsync();

    objrepr::DynRealAttributePtr attr2 = boost::dynamic_pointer_cast<objrepr::DynRealAttribute>( m_attrMap->getAttr("azimut") );
    rt = attr2->setValue( _roll );
    attr2->pushAsync();

    objrepr::DynRealAttributePtr attr3 = boost::dynamic_pointer_cast<objrepr::DynRealAttribute>( m_attrMap->getAttr("focal_length") );
    rt = attr3->setValue( _zoom );
    attr3->pushAsync();

    m_carrierObject->setOrientationAngles( _pitch, _yaw, _roll );
    m_carrierObject->push();

    float pitch = -_pitch + 90; //! ВНИМАНИЕ, на 2Д ожидается диапазон pitch[-180..0] !
    float yaw = _yaw;
    float roll = 0;
    if( pitch > 90.0f ){
        // fix pitch to range 0; +90
        pitch = 180 - pitch;

        // change yaw
        yaw += 180;

        // fix yaw to range -180;+180
        if( yaw > 180 ){
            yaw -= 360;
        }

        roll += 180;
    }

    m_cameraObject->setOrientationAngles( pitch, yaw, roll );
    m_cameraObject->push();
#endif
}

void ControlSignalReceiver::callbackCameraFOVChanged( double _angle ){
#ifdef OBJREPR_LIBRARY_EXIST
    objrepr::DynRealAttributePtr attr1 = boost::dynamic_pointer_cast<objrepr::DynRealAttribute>( m_attrMap->getAttr("field_of_view") );
    bool rt = attr1->setValue( _angle );
    attr1->pushAsync();
#endif
}

// objrepr attr callbacks
void ControlSignalReceiver::callbackObjectUpdated( uint64_t _id ){

    VS_LOG_INFO << PRINT_HEADER << "objrepr object updated: " << _id << endl;

    if( m_holdPointObject ){
        for( IControlSignalsObserver * observer : m_observers ){
            const objrepr::GeoCoord coord = m_holdPointObject->point( 0, 0 );
            observer->callbackSetTargetCoord( coord.y, coord.x, coord.h );
        }
    }
    else if( m_holdPointObjectId != 0 ){
        objrepr::SpatialObjectPtr holdPointObject = objrepr::RepresentationServer::instance()->objectManager()->getObject( m_holdPointObjectId );
        if( holdPointObject ){
            VS_LOG_INFO << PRINT_HEADER << " catch hold object by id [" << m_holdPointObjectId << "]" << endl;
            m_holdPointObject = holdPointObject;
        }
        else{
            VS_LOG_ERROR << PRINT_HEADER << " hold object NOT FOUND by id: " << m_holdPointObjectId << endl;
        }

        for( IControlSignalsObserver * observer : m_observers ){
            const objrepr::GeoCoord coord = m_holdPointObject->point( 0, 0 );
            observer->callbackSetTargetCoord( coord.y, coord.x, coord.h );
        }
    }
}

void ControlSignalReceiver::callbackAttrUpdated( const std::string & _attrName ){

    VS_LOG_INFO << PRINT_HEADER << " attr updated: " << _attrName << endl;
}

void ControlSignalReceiver::callbackRequestCompleted( int32_t _id ){

    VS_LOG_INFO << PRINT_HEADER << " callbackRequestCompleted" << endl;
}

void ControlSignalReceiver::callbackRequestFailed( int32_t _id ){

    VS_LOG_INFO << PRINT_HEADER << " callbackRequestFailed" << endl;
}

void ControlSignalReceiver::callbackApprovePending( const std::string _attrName ){

    VS_LOG_INFO << PRINT_HEADER << " callbackApprovePending: " << _attrName << endl;

#ifdef OBJREPR_LIBRARY_EXIST
    // ------------------------------------------------------------
    // mode
    // ------------------------------------------------------------
    if( "mode" == _attrName ){
        objrepr::DynEnumAttributePtr attr = boost::dynamic_pointer_cast<objrepr::DynEnumAttribute>( m_attrMap->getAttr(_attrName.c_str()) );
        attr->approve();
        const string modeStr = attr->value();
        const IControlSignalsObserver::EDroneMode mode = ::convertDroneModeFromStr( modeStr );
        if( IControlSignalsObserver::EDroneMode::UNDEFINED == mode ){
            VS_LOG_WARN << PRINT_HEADER << " undefined mode from objrepr [" << modeStr << "]" << endl;
        }

        if( (m_holdPointObject || m_holdPointObjectId != 0) && mode != IControlSignalsObserver::EDroneMode::HOLD ){
            VS_LOG_INFO << PRINT_HEADER << " mode is not 'HOLD' - reset holdPointObject" << endl;
            m_holdPointObject.reset();
            m_holdPointObjectId = 0;
        }

        for( IControlSignalsObserver * observer : m_observers ){
            observer->callbackSetMode( mode );
        }
    }
    // ------------------------------------------------------------
    // position control
    // ------------------------------------------------------------
    else if( "want_azimut" == _attrName ){
        objrepr::DynRealAttributePtr attr = boost::dynamic_pointer_cast<objrepr::DynRealAttribute>( m_attrMap->getAttr(_attrName.c_str()) );
        attr->approve();
        const int64_t azimutAbs = attr->value();

        for( IControlSignalsObserver * observer : m_observers ){
            observer->callbackSetWantAzimut( azimutAbs );
        }
    }
    else if( "azimut_change_rate" == _attrName ){
        objrepr::DynRealAttributePtr attr = boost::dynamic_pointer_cast<objrepr::DynRealAttribute>( m_attrMap->getAttr(_attrName.c_str()) );
        attr->approve();
        const double azimutInc = attr->value();

        for( IControlSignalsObserver * observer : m_observers ){
            observer->callbackSetAzimutChangeRate( azimutInc );
        }
    }
    else if( "want_elevation" == _attrName ){
        objrepr::DynRealAttributePtr attr = boost::dynamic_pointer_cast<objrepr::DynRealAttribute>( m_attrMap->getAttr(_attrName.c_str()) );
        attr->approve();
        const double elevationAbs = attr->value();

        for( IControlSignalsObserver * observer : m_observers ){
            observer->callbackSetWantElevation( elevationAbs );
        }
    }
    else if( "elevation_change_rate" == _attrName ){
        objrepr::DynRealAttributePtr attr = boost::dynamic_pointer_cast<objrepr::DynRealAttribute>( m_attrMap->getAttr(_attrName.c_str()) );
        attr->approve();
        const double elevationInc = attr->value();

        for( IControlSignalsObserver * observer : m_observers ){
            observer->callbackSetElevationChangeRate( elevationInc );
        }
    }
    // ------------------------------------------------------------
    // optic control
    // ------------------------------------------------------------
    else if( "focus_change_rate" == _attrName ){
        objrepr::DynRealAttributePtr attr = boost::dynamic_pointer_cast<objrepr::DynRealAttribute>( m_attrMap->getAttr(_attrName.c_str()) );
        attr->approve();
        const double rate = attr->value();

        for( IControlSignalsObserver * observer : m_observers ){
            observer->callbackSetFocusChangeRate( rate );
        }
    }
    else if( "zoom_change_rate" == _attrName ){
        objrepr::DynRealAttributePtr attr = boost::dynamic_pointer_cast<objrepr::DynRealAttribute>( m_attrMap->getAttr(_attrName.c_str()) );
        attr->approve();
        const double rate = attr->value();

        for( IControlSignalsObserver * observer : m_observers ){
            observer->callbackSetZoomChangeRate( rate );
        }
    }
    else if( "diaphragm_mode" == _attrName ){
        objrepr::DynEnumAttributePtr attr = boost::dynamic_pointer_cast<objrepr::DynEnumAttribute>( m_attrMap->getAttr(_attrName.c_str()) );
        attr->approve();
        const string modeStr = attr->value();
        const IControlSignalsObserver::ECameraDiaphragmMode mode = convertDiaphragmModeFromStr( modeStr );

        for( IControlSignalsObserver * observer : m_observers ){
            observer->callbackSetDiaphragmMode( mode );
        }
    }
    // ------------------------------------------------------------
    // service
    // ------------------------------------------------------------
    else if( "start_video_stream" == _attrName ){
        objrepr::DynBooleanAttributePtr attr = boost::dynamic_pointer_cast<objrepr::DynBooleanAttribute>( m_attrMap->getAttr(_attrName.c_str()) );
        attr->approve();
        const bool start = attr->value();

        for( IControlSignalsObserver * observer : m_observers ){
            observer->callbackStartVideoStream( start );
        }
    }
    else if( "trx_scale" == _attrName ){
        objrepr::DynRealAttributePtr attr = boost::dynamic_pointer_cast<objrepr::DynRealAttribute>( m_attrMap->getAttr(_attrName.c_str()) );
        attr->approve();
        const double scale = attr->value();

        for( IControlSignalsObserver * observer : m_observers ){
            observer->callbackSetTxRxScale( scale );
        }
    }
    else if( "show_aim" == _attrName ){
        objrepr::DynBooleanAttributePtr attr = boost::dynamic_pointer_cast<objrepr::DynBooleanAttribute>( m_attrMap->getAttr(_attrName.c_str()) );
        attr->approve();
        const bool show = attr->value();

        for( IControlSignalsObserver * observer : m_observers ){
            observer->callbackSetShowAim( show );
        }
    }
    else if( "show_telemetry" == _attrName ){
        objrepr::DynBooleanAttributePtr attr = boost::dynamic_pointer_cast<objrepr::DynBooleanAttribute>( m_attrMap->getAttr(_attrName.c_str()) );
        attr->approve();
        const bool show = attr->value();

        for( IControlSignalsObserver * observer : m_observers ){
            observer->callbackSetShowTelemetry( show );
        }
    }
    else if( "active_hold_point" == _attrName ){
        objrepr::DynIntegerAttributePtr attr = boost::dynamic_pointer_cast<objrepr::DynIntegerAttribute>( m_attrMap->getAttr(_attrName.c_str()) );
        attr->approve();
        m_holdPointObjectId = attr->value();
    }    
    else if( "connection_settings" == _attrName ){
        objrepr::DynStringAttributePtr attr = boost::dynamic_pointer_cast<objrepr::DynStringAttribute>( m_attrMap->getAttr(_attrName.c_str()) );
        attr->approve();
        const string settingsStr = attr->value();

        VS_LOG_INFO << PRINT_HEADER << " settings content [" << settingsStr << "]" << endl;

        for( ISystemObserver * observer : m_observersSystem ){
            observer->callbackSwitchOn( true );
        }
    }
    else{
        VS_LOG_WARN << PRINT_HEADER << " unknown attr name [" << _attrName << "]" << endl;
    }
#endif
}

void ControlSignalReceiver::addObserver( IControlSignalsObserver * _observer ){
    m_observers.push_back( _observer );
}

void ControlSignalReceiver::addObserver( ISystemObserver * _observer ){
    m_observersSystem.push_back( _observer );
}

/*
mode                    observation		чтение и запись		Перечисление	( observation / hold / pilot_window / stow / position )
azimut                  0 	гр.         чтение              Вещественное число
elevation               0 	гр.         чтение              Вещественное число
focal_length            3,6	мм.         чтение              Вещественное число
want_azimut             protected0 	гр.         чтение и запись		Вещественное число
want_elevation          0 	гр.         чтение и запись		Вещественное число
elevation_change_rate	0               чтение и запись		Вещественное число
azimut_change_rate      0               чтение и запись		Вещественное число
zoom_change_rate        0               чтение и запись		Вещественное число
focus_change_rate       0               чтение и запись		Вещественное число
show_telemetry          Ложь            чтение и запись		Истина/ложь
show_aim                Ложь            чтение и запись		Истина/ложь
trx_scale               1               чтение и запись		Вещественное число
diaphragm_mode          auto            чтение и запись		Перечисление
active_hold_point       0               чтение и запись		Целое число
*/













#include "from_ms_common/system/logger.h"

#include "common_utils.h"
#include "control_signal_receiver.h"

using namespace std;

static IControlSignalsObserver::EDroneMode convertDroneModeFromStr( const string & _str ){

    if( "observation" == _str ){ return IControlSignalsObserver::EDroneMode::OBSERVATION; }
    else if( "hold" == _str ){ return IControlSignalsObserver::EDroneMode::HOLD; }
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
{

}

bool ControlSignalReceiver::init( const SInitSettings & _settings ){
#ifdef OBJREPR_LIBRARY_EXIST
    // bind to carrier
    objrepr::SpatialObjectPtr carrierObject = objrepr::RepresentationServer::instance()->objectManager()->getObject( _settings.carrierObjectId );
    if( ! carrierObject ){
        VS_LOG_ERROR << "carrier object with id " << _settings.carrierObjectId << " is not found" << endl;
        return false;
    }
    m_carrierObject = carrierObject;

    // bind to camera
    objrepr::SpatialObjectPtr cameraObject = objrepr::RepresentationServer::instance()->objectManager()->getObject( _settings.cameraObjectId );
    if( ! cameraObject ){
        VS_LOG_ERROR << "camera object with id " << _settings.cameraObjectId << " is not found" << endl;
        return false;
    }

    // listen attrs changing
    objrepr::DynamicAttributeMapPtr attrMap = cameraObject->dynamicAttrMap();

    attrMap->approvePending.connect( boost::bind( & ControlSignalReceiver::callbackApprovePending, this, _1 ) );
    attrMap->attrUpdated.connect( boost::bind( & ControlSignalReceiver::callbackAttrUpdated, this, _1 ) );
    attrMap->reqCompleted.connect( boost::bind( & ControlSignalReceiver::callbackRequestCompleted, this, _1 ) );
    attrMap->reqFailed.connect( boost::bind( & ControlSignalReceiver::callbackRequestFailed, this, _1 ) );

    attrMap->getHold();
    m_attrMap = attrMap;
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
        cout << "WARNING: failed to change point of objrepr-object: " << m_carrierObject->name() << endl;
    }
    m_carrierObject->push();
#endif
//    cout << "carrier moving: " << _lat << endl;
}

void ControlSignalReceiver::callbackCameraPositionChanged( double _pitch, double _roll, double _zoom ){

    // TODO: do
}

// objrepr attr callbacks
void ControlSignalReceiver::callbackAttrUpdated( const std::string & _attrName ){

    VS_LOG_INFO << "attr updated: " << _attrName << endl;
}

void ControlSignalReceiver::callbackRequestCompleted( int32_t _id ){

    VS_LOG_INFO << "callbackRequestCompleted" << endl;
}

void ControlSignalReceiver::callbackRequestFailed( int32_t _id ){

    VS_LOG_INFO << "callbackRequestFailed" << endl;
}

void ControlSignalReceiver::callbackApprovePending( std::string _attrName ){

    VS_LOG_INFO << "callbackApprovePending: " << _attrName << endl;

#ifdef OBJREPR_LIBRARY_EXIST
    // ------------------------------------------------------------
    // mode
    // ------------------------------------------------------------
    if( "mode" == _attrName ){
        objrepr::DynEnumAttributePtr attr = boost::dynamic_pointer_cast<objrepr::DynEnumAttribute>( m_attrMap->getAttr(_attrName.c_str()) );
        const string modeStr = attr->value();
        const IControlSignalsObserver::EDroneMode mode = convertDroneModeFromStr( modeStr );

        attr->approve();
        VS_LOG_INFO << "val: " << attr->desiredValue() << endl;

        for( IControlSignalsObserver * observer : m_observers ){
            observer->callbackSetMode( mode );
        }
    }
    // ------------------------------------------------------------
    // position control
    // ------------------------------------------------------------
    else if( "azimut" == _attrName ){
        objrepr::DynRealAttributePtr attr = boost::dynamic_pointer_cast<objrepr::DynRealAttribute>( m_attrMap->getAttr(_attrName.c_str()) );
        const double azimut = attr->value();

        attr->approve();
        VS_LOG_INFO << "val: " << attr->desiredValue() << endl;

        for( IControlSignalsObserver * observer : m_observers ){
            observer->callbackSetAzimut( azimut );
        }
    }
    else if( "want_azimut" == _attrName ){
        objrepr::DynRealAttributePtr attr = boost::dynamic_pointer_cast<objrepr::DynRealAttribute>( m_attrMap->getAttr(_attrName.c_str()) );
        const int64_t azimutAbs = attr->value();

        attr->approve();
        VS_LOG_INFO << "val: " << attr->desiredValue() << endl;

        for( IControlSignalsObserver * observer : m_observers ){
            observer->callbackSetWantAzimut( azimutAbs );
        }
    }
    else if( "azimut_change_rate" == _attrName ){
        objrepr::DynRealAttributePtr attr = boost::dynamic_pointer_cast<objrepr::DynRealAttribute>( m_attrMap->getAttr(_attrName.c_str()) );
        const double azimutInc = attr->value();

        attr->approve();
        VS_LOG_INFO << "val: " << attr->desiredValue() << endl;

        for( IControlSignalsObserver * observer : m_observers ){
            observer->callbackSetAzimutChangeRate( azimutInc );
        }
    }
    else if( "elevation" == _attrName ){
        objrepr::DynRealAttributePtr attr = boost::dynamic_pointer_cast<objrepr::DynRealAttribute>( m_attrMap->getAttr(_attrName.c_str()) );
        const double elevation = attr->value();

        attr->approve();
        VS_LOG_INFO << "val: " << attr->desiredValue() << endl;

        for( IControlSignalsObserver * observer : m_observers ){
            observer->callbackSetElevation( elevation );
        }
    }
    else if( "want_elevation" == _attrName ){
        objrepr::DynRealAttributePtr attr = boost::dynamic_pointer_cast<objrepr::DynRealAttribute>( m_attrMap->getAttr(_attrName.c_str()) );
        const double elevationAbs = attr->value();

        attr->approve();
        VS_LOG_INFO << "val: " << attr->desiredValue() << endl;

        for( IControlSignalsObserver * observer : m_observers ){
            observer->callbackSetWantElevation( elevationAbs );
        }
    }
    else if( "elevation_change_rate" == _attrName ){
        objrepr::DynRealAttributePtr attr = boost::dynamic_pointer_cast<objrepr::DynRealAttribute>( m_attrMap->getAttr(_attrName.c_str()) );
        const double elevationInc = attr->value();

        attr->approve();
        VS_LOG_INFO << "val: " << attr->desiredValue() << endl;

        for( IControlSignalsObserver * observer : m_observers ){
            observer->callbackSetElevationChangeRate( elevationInc );
        }
    }
    // ------------------------------------------------------------
    // optic control
    // ------------------------------------------------------------
    else if( "focal_length" == _attrName ){
        objrepr::DynRealAttributePtr attr = boost::dynamic_pointer_cast<objrepr::DynRealAttribute>( m_attrMap->getAttr(_attrName.c_str()) );
        const double lengthMillimeter = attr->value();

        attr->approve();
        VS_LOG_INFO << "val: " << attr->desiredValue() << endl;

        for( IControlSignalsObserver * observer : m_observers ){
            observer->callbackSetFocalLength( lengthMillimeter );
        }
    }
    else if( "focus_change_rate" == _attrName ){
        objrepr::DynRealAttributePtr attr = boost::dynamic_pointer_cast<objrepr::DynRealAttribute>( m_attrMap->getAttr(_attrName.c_str()) );
        const double rate = attr->value();

        attr->approve();
        VS_LOG_INFO << "val: " << attr->desiredValue() << endl;

        for( IControlSignalsObserver * observer : m_observers ){
            observer->callbackSetFocusChangeRate( rate );
        }
    }
    else if( "zoom_change_rate" == _attrName ){
        objrepr::DynRealAttributePtr attr = boost::dynamic_pointer_cast<objrepr::DynRealAttribute>( m_attrMap->getAttr(_attrName.c_str()) );
        const double rate = attr->value();

        attr->approve();
        VS_LOG_INFO << "val: " << attr->desiredValue() << endl;

        for( IControlSignalsObserver * observer : m_observers ){
            observer->callbackSetZoomChangeRate( rate );
        }
    }
    else if( "diaphragm_mode" == _attrName ){
        objrepr::DynEnumAttributePtr attr = boost::dynamic_pointer_cast<objrepr::DynEnumAttribute>( m_attrMap->getAttr(_attrName.c_str()) );
        const string modeStr = attr->value();
        const IControlSignalsObserver::ECameraDiaphragmMode mode = convertDiaphragmModeFromStr( modeStr );

        attr->approve();
        VS_LOG_INFO << "val: " << attr->desiredValue() << endl;

        for( IControlSignalsObserver * observer : m_observers ){
            observer->callbackSetDiaphragmMode( mode );
        }
    }
    // ------------------------------------------------------------
    // service
    // ------------------------------------------------------------
    else if( "trx_scale" == _attrName ){
        objrepr::DynRealAttributePtr attr = boost::dynamic_pointer_cast<objrepr::DynRealAttribute>( m_attrMap->getAttr(_attrName.c_str()) );
        const double scale = attr->value();

        attr->approve();
        VS_LOG_INFO << "val: " << attr->desiredValue() << endl;

        for( IControlSignalsObserver * observer : m_observers ){
            observer->callbackSetTxRxScale( scale );
        }
    }
    else if( "show_aim" == _attrName ){
        objrepr::DynBooleanAttributePtr attr = boost::dynamic_pointer_cast<objrepr::DynBooleanAttribute>( m_attrMap->getAttr(_attrName.c_str()) );
        const bool show = attr->value();

        attr->approve();
        VS_LOG_INFO << "val: " << attr->desiredValue() << endl;

        for( IControlSignalsObserver * observer : m_observers ){
            observer->callbackSetShowAim( show );
        }
    }
    else if( "show_telemetry" == _attrName ){
        objrepr::DynBooleanAttributePtr attr = boost::dynamic_pointer_cast<objrepr::DynBooleanAttribute>( m_attrMap->getAttr(_attrName.c_str()) );
        const bool show = attr->value();

        attr->approve();
        VS_LOG_INFO << "val: " << attr->desiredValue() << endl;

        for( IControlSignalsObserver * observer : m_observers ){
            observer->callbackSetShowTelemetry( show );
        }
    }
    else if( "active_hold_point" == _attrName ){
        objrepr::DynBooleanAttributePtr attr = boost::dynamic_pointer_cast<objrepr::DynBooleanAttribute>( m_attrMap->getAttr(_attrName.c_str()) );
        const bool ahp = attr->value();

        attr->approve();
        VS_LOG_INFO << "val: " << attr->desiredValue() << endl;

        for( IControlSignalsObserver * observer : m_observers ){
            observer->callback( ahp );
        }
    }
    else{
        // TODO: catch error
    }
#endif
}

void ControlSignalReceiver::addObserver( IControlSignalsObserver * _observer ){
    m_observers.push_back( _observer );
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












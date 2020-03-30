
#include "control_signal_receiver.h"

using namespace std;

ControlSignalReceiver::ControlSignalReceiver()
{

}

void ControlSignalReceiver::callbackAttrUpdated( const std::string & _attrName ){

    if( "bla" == _attrName ){

        objrepr::DynBooleanAttributePtr attr = boost::dynamic_pointer_cast<objrepr::DynBooleanAttribute>( m_attrMap->getAttr(_attrName.c_str()) );
        const bool val = attr->value();




    }
    else if( "bla" == _attrName ){

        objrepr::DynIntegerAttributePtr attr = boost::dynamic_pointer_cast<objrepr::DynIntegerAttribute>( m_attrMap->getAttr(_attrName.c_str()) );
        const int64_t val = attr->value();


    }
    else if( "blu" == _attrName ){

        objrepr::DynRealAttributePtr attr = boost::dynamic_pointer_cast<objrepr::DynRealAttribute>( m_attrMap->getAttr(_attrName.c_str()) );
        const double val = attr->value();

        for( IControlSignalsObserver * observer : m_observers ){
            observer->callbackSetTargetCoord( 0.0f, val, 0.0f );
        }
    }
    else{





    }


}

void ControlSignalReceiver::callbackApprovePending( std::string _attrName ){






}

bool ControlSignalReceiver::init( const SInitSettings & _settings ){

    objrepr::SpatialObjectPtr droneObject = objrepr::RepresentationServer::instance()->objectManager()->getObject( _settings.objectId );
    if( ! droneObject ){
        return false;
    }

    objrepr::DynamicAttributeMapPtr attrMap = droneObject->dynamicAttrMap();

    attrMap->getHold();

    attrMap->approvePending.connect( boost::bind( & ControlSignalReceiver::callbackApprovePending, this, _1 ) );
    attrMap->attrUpdated.connect( boost::bind( & ControlSignalReceiver::callbackAttrUpdated, this, _1 ) );

    m_attrMap = attrMap;



    return true;
}














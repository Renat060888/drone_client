#ifndef CONTROL_SIGNAL_RECEVIER_H
#define CONTROL_SIGNAL_RECEVIER_H

#include <vector>

#ifdef OBJREPR_LIBRARY_EXIST
#include <objrepr/reprServer.h>
#endif

#include "common_stuff.h"

class ControlSignalReceiver : public IDroneStateObserver
{
public:
    struct SInitSettings {
        uint64_t cameraObjectId;
        uint64_t carrierObjectId;
    };

    struct SState {
        SInitSettings settings;
        std::string lastError;
    };

    ControlSignalReceiver();

    bool init( const SInitSettings & _settings );
    void addObserver( IControlSignalsObserver * _observer );



private:
    // signals from objrepr
    void callbackAttrUpdated( const std::string & _attrName );
    void callbackApprovePending( const std::string _attrName );
    void callbackRequestCompleted( int32_t _id );
    void callbackRequestFailed( int32_t _id );

    // signals from drone controller
    virtual void callbackBoardPositionChanged( double _lat, double _lon, double _alt ) override;
    virtual void callbackCameraPositionChanged( double _pitch, double _roll, double _zoom ) override;
    virtual void callbackBoardOnline( bool _online ) override;



    // data
    std::vector<IControlSignalsObserver *> m_observers;
    SState m_state;
#ifdef OBJREPR_LIBRARY_EXIST
    objrepr::SpatialObjectPtr m_carrierObject;
    objrepr::SpatialObjectPtr m_cameraObject;
    objrepr::DynamicAttributeMapPtr m_attrMap;
#endif

    // service


};

#endif // CONTROL_SIGNAL_RECEVIER_H

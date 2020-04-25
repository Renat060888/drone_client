#ifndef CONTROL_SIGNAL_RECEVIER_H
#define CONTROL_SIGNAL_RECEVIER_H

#include <vector>

#ifdef OBJREPR_LIBRARY_EXIST
#include <objrepr/reprServer.h>
#endif

#include "common_stuff.h"

class ControlSignalReceiver
{
public:
    struct SInitSettings {
        uint64_t objectId;
    };

    struct SState {
        std::string lastError;
    };

    ControlSignalReceiver();

    bool init( const SInitSettings & _settings );
    void addObserver( IControlSignalsObserver * _observer );



private:
    void callbackAttrUpdated( const std::string & _attrName );
    void callbackApprovePending( std::string _attrName );
    void callbackRequestCompleted( int32_t _id );
    void callbackRequestFailed( int32_t _id );



    // data
    SState m_state;
    std::vector<IControlSignalsObserver *> m_observers;
#ifdef OBJREPR_LIBRARY_EXIST
    objrepr::DynamicAttributeMapPtr m_attrMap;
#endif


};

#endif // CONTROL_SIGNAL_RECEVIER_H

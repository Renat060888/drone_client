#ifndef CONTROL_SIGNAL_RECEVIER_H
#define CONTROL_SIGNAL_RECEVIER_H

#include <objrepr/reprServer.h>

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



    // data
    SState m_state;
    objrepr::DynamicAttributeMapPtr m_attrMap;
    std::vector<IControlSignalsObserver *> m_observers;


};

#endif // CONTROL_SIGNAL_RECEVIER_H

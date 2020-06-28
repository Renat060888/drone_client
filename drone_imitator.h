#ifndef DRONE_IMITATOR_H
#define DRONE_IMITATOR_H

#include <thread>

#include "common_stuff.h"

class DroneImitator : public IControlSignalsObserver, public ISystemObserver
{
public:
    DroneImitator();





private:
    void threadMaintenance();

    // signals from objrepr
    virtual void callbackSetTargetCoord(float lat, float lon, int alt) override;

    virtual void callbackSetMode( EDroneMode _mode ) override;

    virtual void callbackSetWantAzimut( double _azimutAbsDeg ) override;
    virtual void callbackSetAzimutChangeRate( double _azimutIncDeg ) override;
    virtual void callbackSetWantElevation( double _elevationAbsDeg ) override;
    virtual void callbackSetElevationChangeRate( double _elevationIncDeg ) override;

    virtual void callbackSetFocusChangeRate( double _rate ) override;
    virtual void callbackSetZoomChangeRate( double _rate ) override;
    virtual void callbackSetDiaphragmMode( ECameraDiaphragmMode _mode ) override;

    virtual void callbackSetTxRxScale( double _scale ) override;
    virtual void callbackSetShowAim( bool _show ) override;
    virtual void callbackSetShowTelemetry( bool _show ) override;
    virtual void callbackStartVideoStream( bool _start ) override;

    virtual void callbackSwitchOn( bool _on, const std::string _runSettings ) override;




    // data
    double m_imitatedCameraPitch;
    double m_imitatedCameraRoll;
    double m_requestedCameraPitch;
    double m_requestedCameraRoll;
    int m_imitateCameraZoom;
    int m_requestedCameraZoom;


    // service
    std::thread * m_trMaintenance;


};

#endif // DRONE_IMITATOR_H

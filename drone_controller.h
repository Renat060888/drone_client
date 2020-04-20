#ifndef DRONE_CONTROLLER_H
#define DRONE_CONTROLLER_H

#include <nppntt/rfgroundcontrolprocessing.h>

#include "common_stuff.h"

class DroneController : public IControlSignalsObserver
{
public:
    struct SInitSettings {
        std::string configFilePath;
    };

    DroneController();

    bool init( const SInitSettings & _settings );


private:
    virtual void callbackSetTargetCoord(float lat, float lon, int alt) override;

    virtual void callbackSetMode( EDroneMode _mode ) override;

    virtual void callbackSetAzimut( double _azimutDeg ) override;
    virtual void callbackSetWantAzimut( double _azimutAbsDeg ) override;
    virtual void callbackSetAzimutChangeRate( double _azimutIncDeg ) override;
    virtual void callbackSetElevation( double _elevationDeg ) override;
    virtual void callbackSetWantElevation( double _elevationAbsDeg ) override;
    virtual void callbackSetElevationChangeRate( double _elevationIncDeg ) override;

    virtual void callbackSetFocalLength( double _focalLengthMillimeter ) override;
    virtual void callbackSetFocusChangeRate( double _rate ) override;
    virtual void callbackSetZoomChangeRate( double _rate ) override;
    virtual void callbackSetDiaphragmMode( ECameraDiaphragmMode _mode ) override;

    virtual void callbackSetTxRxScale( double _scale ) override;
    virtual void callbackSetShowAim( bool _show ) override;
    virtual void callbackSetShowTelemetry( bool _show ) override;

    // service
    OwlGroudControl::RFGroundControlProcessing rfc;

};

#endif // DRONE_CONTROLLER_H

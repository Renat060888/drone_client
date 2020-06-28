#ifndef COMMON_STUFF_H
#define COMMON_STUFF_H

#include <iostream>

using TConstDataPointer = const void *;
using TDataSize = int32_t;

struct SImageProperties {
    int32_t width;
    int32_t height;
    int64_t timestampMillisec;
};

class IImageProvider {
public:
    virtual ~IImageProvider(){}

    virtual std::pair<TConstDataPointer, TDataSize> getImageData() = 0;
    virtual SImageProperties getImageProperties() = 0;
};

class ISystemObserver {
public:
    virtual ~ISystemObserver(){}

    virtual void callbackSwitchOn( bool _on, const std::string _runSettings ) = 0;

};

class IDroneStateObserver {
public:
    virtual ~IDroneStateObserver(){}

    virtual void callbackBoardPositionChanged( double _lat, double _lon, double _alt ) = 0;
    virtual void callbackCameraPositionChanged( double _pitch, double _roll, double _yaw, double _zoom ) = 0;
    virtual void callbackCameraFOVChanged( double _angle ) = 0;
    virtual void callbackBoardOnline( bool _online ) = 0;


};

class IControlSignalsObserver {
public:
    enum class EDroneMode {
        OBSERVATION,
        HOLD,
        HOLD_TO_AIM,
        PILOT_WINDOW,
        STOW,
        POSITION,
        UNDEFINED
    };

    enum class ECameraDiaphragmMode {
        OPEN,
        AUTO,
        UNDEFINED
    };

    virtual ~IControlSignalsObserver(){}

    virtual void callbackSetTargetCoord( float lat, float lon, int alt ) = 0;

    // mode
    virtual void callbackSetMode( EDroneMode _mode ) = 0;

    // position control
    virtual void callbackSetWantAzimut( double _azimutAbsDeg ) = 0;
    virtual void callbackSetAzimutChangeRate( double _azimutIncDeg ) = 0;
    virtual void callbackSetWantElevation( double _elevationAbsDeg ) = 0;
    virtual void callbackSetElevationChangeRate( double _elevationIncDeg ) = 0;

    // optic control
    virtual void callbackSetFocusChangeRate( double _rate ) = 0;
    virtual void callbackSetZoomChangeRate( double _rate ) = 0;
    virtual void callbackSetDiaphragmMode( ECameraDiaphragmMode _mode ) = 0;

    // service
    virtual void callbackSetTxRxScale( double _scale ) = 0;
    virtual void callbackSetShowAim( bool _show ) = 0;
    virtual void callbackSetShowTelemetry( bool _show ) = 0;
    virtual void callbackStartVideoStream( bool _start ) = 0;
};




















#endif // COMMON_STUFF_H

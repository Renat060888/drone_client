#ifndef COMMON_STUFF_H
#define COMMON_STUFF_H

#include <iostream>

#define VS_LOG_WARN std::cerr
#define VS_LOG_ERROR std::cerr
#define VS_LOG_CRITICAL std::cerr
#define VS_LOG_INFO std::cout
#define VS_LOG_DEBUG std::cerr
#define VS_LOG_TRACE std::cerr

using TConstDataPointer = const void *;
using TDataSize = int32_t;

struct SImageProperties {
    int32_t width;
    int32_t height;
};

class IImageProvider {
public:
    virtual ~IImageProvider(){}

    virtual std::pair<TConstDataPointer, TDataSize> getImageData() = 0;
    virtual SImageProperties getImageProperties() = 0;
};

class IControlSignalsObserver {
public:
    enum class EDroneMode {
        OBSERVATION,
        HOLD,
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
    virtual void callbackSetAzimut( double _azimutDeg ) = 0;
    virtual void callbackSetWantAzimut( double _azimutAbsDeg ) = 0;
    virtual void callbackSetAzimutChangeRate( double _azimutIncDeg ) = 0;
    virtual void callbackSetElevation( double _elevationDeg ) = 0;
    virtual void callbackSetWantElevation( double _elevationAbsDeg ) = 0;
    virtual void callbackSetElevationChangeRate( double _elevationIncDeg ) = 0;

    // optic control
    virtual void callbackSetFocalLength( double _focalLengthMillimeter ) = 0;
    virtual void callbackSetFocusChangeRate( double _rate ) = 0;
    virtual void callbackSetZoomChangeRate( double _rate ) = 0;
    virtual void callbackSetDiaphragmMode( ECameraDiaphragmMode _mode ) = 0;

    // service
    virtual void callbackSetTxRxScale( double _scale ) = 0;
    virtual void callbackSetShowAim( bool _show ) = 0;
    virtual void callbackSetShowTelemetry( bool _show ) = 0;
};




















#endif // COMMON_STUFF_H

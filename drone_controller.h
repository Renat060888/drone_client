#ifndef DRONE_CONTROLLER_H
#define DRONE_CONTROLLER_H

#include <thread>

#include <nppntt/rfgroundcontrolprocessing.h>

#include "common_stuff.h"

class DroneController : public QObject, public IControlSignalsObserver
{
Q_OBJECT
public:
    struct SInitSettings {
        std::string configFilePath;
        int64_t pingTimeoutMillisec;
        bool movingImitationEnable;
    };

    DroneController();
    ~DroneController();

    bool init( const SInitSettings & _settings );
    void addObserver( IDroneStateObserver * _observer );


private:
    void threadCarrierImitation();

    // signals from objrepr
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

    void movingImitation();
    void checkPings();

    // data
    std::vector<IDroneStateObserver *> m_observers;
    bool m_shutdownCalled;
    SInitSettings m_settings;

    // service
    OwlGroudControl::RFGroundControlProcessing rfc;
    std::thread * m_trCarrierImitation;


private slots:
    void slotReadResponseComplete( OwlDeviceInputData::OwlDeviceDataBase::DataType _cmd );

    void slotCurrentStateChanged( OwlDeviceInputData::State * _currentState );
    void slotBoardPosChanged( OwlDeviceInputData::BoardPosition * _boardPos );
    void slotRpzLensChanged( OwlDeviceInputData::RollPitchZoomLens * _rpzLens );
};

#endif // DRONE_CONTROLLER_H

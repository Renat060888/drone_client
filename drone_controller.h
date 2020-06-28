#ifndef DRONE_CONTROLLER_H
#define DRONE_CONTROLLER_H

#include <thread>

#include <nppntt/rfgroundcontrolprocessing.h>

#include "common_stuff.h"

class DroneController : public QObject, public IControlSignalsObserver, public ISystemObserver
{
Q_OBJECT
public:
    struct SInitSettings {
        std::string configFilePath;
        int64_t pingTimeoutMillisec;
        bool imitationEnable;
    };

    DroneController();
    ~DroneController();

    bool init( const SInitSettings & _settings );
    void addObserver( IDroneStateObserver * _observer );


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

    void droneActivityImitation();
    void checkPings();

    // data
    std::vector<IDroneStateObserver *> m_observers;
    bool m_shutdownCalled;
    SInitSettings m_settings;
    double m_azimutAbsDeg;
    double m_azimutIncDeg;
    double m_elevationAbsDeg;
    double m_elevationIncDeg;
    bool m_lastPingValFromDrone;
    bool m_boardOnline;
    int64_t m_lastPingAtMillisec;

    // imitation data
    double m_imitatedCameraPitch;
    double m_imitatedCameraRoll;
    double m_requestedCameraPitch;
    double m_requestedCameraRoll;
    int m_imitateCameraZoom;
    int m_requestedCameraZoom;

    // service
    OwlGroudControl::RFGroundControlProcessing rfc;
    std::thread * m_trMaintenance;


private slots:
    void slotReadResponseComplete( OwlDeviceInputData::OwlDeviceDataBase::DataType _cmd );

    void slotCurrentStateChanged( OwlDeviceInputData::State * _currentState );
    void slotBoardPosChanged( OwlDeviceInputData::BoardPosition * _boardPos );
    void slotRpzLensChanged( OwlDeviceInputData::RollPitchZoomLens * _rpzLens );
};

#endif // DRONE_CONTROLLER_H

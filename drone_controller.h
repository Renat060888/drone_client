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



    // service
    OwlGroudControl::RFGroundControlProcessing rfc;

};

#endif // DRONE_CONTROLLER_H

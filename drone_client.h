#ifndef DRONE_CLIENT_H
#define DRONE_CLIENT_H

#include <vector>

#include "video_generator.h"
#include "video_convertor.h"
#include "drone_controller.h"
#include "control_signal_receiver.h"

class DroneClient
{
public:
    struct SInitSettings {
    };

    static DroneClient * singleton(){
        static DroneClient * instance = nullptr;

        if( ! instance ){
            instance = new DroneClient();
        }
        return instance;
    }

    bool init( const SInitSettings & _settings );
    void launch();


private:
    DroneClient( const DroneClient & _rhs ) = delete;
    DroneClient & operator=( const DroneClient & _rhs ) = delete;

    DroneClient();
    ~DroneClient();

    void firstFrameFronDrone();

    // data

    // service
    VideoGenerator m_videoGenerator;
    VideoConvertor m_videoConvertor;
    IImageProvider * m_imageProvider;
    ControlSignalReceiver m_controlSignalReceiver;
    DroneController m_droneController;
};

#endif // DRONE_CLIENT_H

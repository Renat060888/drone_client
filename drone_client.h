#ifndef DRONE_CLIENT_H
#define DRONE_CLIENT_H

#include <vector>

#include "video_generator.h"
#include "drone_controller.h"
#include "control_signal_receiver.h"

class DroneClient
{
public:
    struct SInitSettings {
        int argc;
        char ** argv;
    };

    static DroneClient & singleton(){
        static DroneClient instance;
        return instance;
    }

    bool init( const SInitSettings & _settings );
    void launch();


    // data
    std::vector<char> m_imageBytes;

    // service
    VideoGenerator m_videoGenerator;
    IImageProvider * m_imageProvider;
    ControlSignalReceiver m_controlSignalReceiver;
    DroneController m_droneController;

private:
    DroneClient( const DroneClient & _rhs ) = delete;
    DroneClient & operator=( const DroneClient & _rhs ) = delete;

    DroneClient();
    ~DroneClient();



};

#endif // DRONE_CLIENT_H

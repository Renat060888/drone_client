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

    };

    DroneClient();

    bool init( const SInitSettings & _settings );
    void launch();


    std::vector<char> m_imageBytes;

    VideoGenerator m_videoGenerator;
    IImageProvider * m_imageProvider;
};

#endif // DRONE_CLIENT_H

#ifndef IMAGE_FROM_DRONE_H
#define IMAGE_FROM_DRONE_H

#include <nppntt/rfgroundvideoprocessing.h>

#include "common_stuff.h"

class ImageFromDrone : public IImageProvider
{
public:
    struct SInitSettings {
        std::string configFilePath;
    };

    ImageFromDrone();

    bool init( const SInitSettings & _settings );

    virtual std::pair<TConstDataPointer, TDataSize> getImageData() override;
    virtual SImageProperties getImageProperties() override;


private:


    // service
    OwlGroudControl::RFGroundVideoProcessing rfv;


};

#endif // IMAGE_FROM_DRONE_H

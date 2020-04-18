#ifndef IMAGE_FROM_DRONE_H
#define IMAGE_FROM_DRONE_H

#include <mutex>

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

private slots:
    void slotFrameChanged( QImage _frame );

private:

    // data
    QImage m_droneCurrentFrame;
    OwlDeviceInputData::OwlDeviceFrameDescriptor * m_frameDescr;

    // service
    OwlGroudControl::RFGroundVideoProcessing rfv;
    std::mutex m_mutexImageRef;
    std::mutex m_mutexImageDescrRef;

};

#endif // IMAGE_FROM_DRONE_H

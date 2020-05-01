#ifndef IMAGE_FROM_DRONE_H
#define IMAGE_FROM_DRONE_H

#include <mutex>

#include <opencv2/opencv.hpp>
#include <nppntt/rfgroundvideoprocessing.h>

#include "common_stuff.h"

class ImageFromDrone : public QObject, public IImageProvider
{
Q_OBJECT
public:
    struct SInitSettings {
        std::string configFilePath;
        bool statusOverlay;
    };

    ImageFromDrone();
    ~ImageFromDrone();

    bool init( const SInitSettings & _settings );

    virtual std::pair<TConstDataPointer, TDataSize> getImageData() override;
    virtual SImageProperties getImageProperties() override;


private slots:
    void slotLastFrameChanged( QByteArray & _frame );


private:

    // data
    QByteArray m_droneCurrentFrame;
    OwlDeviceInputData::OwlDeviceFrameDescriptor * m_frameDescr;
    SInitSettings m_settings;

    cv::Mat m_currentImage;
    std::vector<unsigned char> m_currentImageBytes;

    // service
    OwlGroudControl::RFGroundVideoProcessing rfv;
    std::mutex m_mutexImageRef;
    std::mutex m_mutexImageDescrRef;

};

#endif // IMAGE_FROM_DRONE_H

#ifndef IMAGE_FROM_DRONE_H
#define IMAGE_FROM_DRONE_H

#include <mutex>
#include <thread>
#include <queue>

#include <boost/signals2.hpp>
#include <opencv2/opencv.hpp>
#include <nppntt/rfgroundvideoprocessing.h>

#include "common_stuff.h"

class ImageFromDrone : public QObject, public IImageProvider, public ISystemObserver
{
Q_OBJECT
public:
    struct SDumpData {
        QByteArray frameBytes;
        QString telemetryStr;
    };

    struct SInitSettings {
        SInitSettings()
            : statusOverlay(false)
            , dumpIncomingData(false)
        {}
        std::string configFilePath;
        bool statusOverlay;
        bool dumpIncomingData;
    };

    ImageFromDrone();
    ~ImageFromDrone();

    bool init( const SInitSettings & _settings );

    virtual std::pair<TConstDataPointer, TDataSize> getImageData() override;
    virtual SImageProperties getImageProperties() override;

    boost::signals2::signal<void()> m_signalFirstFrameFromDrone;


private slots:
    void slotLastFrameChanged( QByteArray & _frame );


private:
    virtual void callbackSwitchOn( bool _on ) override;

    void threadFrameAndTelemetryDump();

    // data
    QByteArray m_droneCurrentFrame;
    OwlDeviceInputData::OwlDeviceFrameDescriptor * m_frameDescr;
    SInitSettings m_settings;

    cv::Mat m_currentImage;
    std::vector<unsigned char> m_currentImageBytes;
    int m_widthViaOpenCV;
    int m_heightViaOpenCV;
    bool m_shutdownCalled;

    std::queue<SDumpData> m_dumpQueue;

    // service
    OwlGroudControl::RFGroundVideoProcessing rfv;
    std::mutex m_muCurrentFrame;
    std::mutex m_mutexImageDescrRef;
    std::thread * m_trFrameAndTelemetryDump;
    std::mutex m_muDumpQueue;
};

#endif // IMAGE_FROM_DRONE_H

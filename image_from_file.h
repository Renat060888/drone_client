#ifndef IMAGE_FROM_FILE_H
#define IMAGE_FROM_FILE_H

#include <vector>
#include <mutex>
#include <thread>

#include "common_stuff.h"

class ImageFromFile : public IImageProvider
{
public:
    struct SInitSettings {
        std::string imageDir;
        int64_t imageCaptureIntervalMilllisec;
    };

    struct SImage {
        std::string fileName;
        int64_t capturedAtTimeMillisec;
        std::vector<char> imageBytes;
        std::pair<TConstDataPointer, TDataSize> imageMetadata;
    };

    struct SOneSecondImages {
        // NOTE: may be empty slots. For instance: if FPS = 10 and images count = 6 -> [ 1 2 3 4 5 6 - - - - ]
        std::vector<const SImage *> images;
        int32_t lastImageIdx;
    };

    struct SState {
        SInitSettings settings;
        SImageProperties imageProps;
    };

    ImageFromFile();

    bool init( const SInitSettings & _settings );

    virtual std::pair<TConstDataPointer, TDataSize> getImageData() override;
    virtual SImageProperties getImageProperties() override;


private:
    void threadImageRotation();

    void tick();

    bool createImageTimeline( const std::string & _imageDir );
    SImage * loadImage( const std::string & _imagePath );


    // data
    const SImage * m_currentImageRef;
    int32_t m_currentSecondIdx;
    int32_t m_currentFrameIdx;
    std::vector<SOneSecondImages> m_imagesBySeconds;
    SState m_state;


    // service
    std::mutex m_mutexImageRef;
    std::thread * m_trImageRotation;
};

#endif // IMAGE_FROM_FILE_H

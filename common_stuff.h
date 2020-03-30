#ifndef COMMON_STUFF_H
#define COMMON_STUFF_H

#include <iostream>

#define VS_LOG_WARN std::cerr
#define VS_LOG_ERROR std::cerr
#define VS_LOG_CRITICAL std::cerr
#define VS_LOG_INFO std::cout
#define VS_LOG_DEBUG std::cerr
#define VS_LOG_TRACE std::cerr

using TConstDataPointer = const void *;
using TDataSize = int32_t;

struct SImageProperties {
    int32_t width;
    int32_t height;
};

class IImageProvider {
public:
    virtual ~IImageProvider(){}

    virtual std::pair<TConstDataPointer, TDataSize> getImageData() = 0;
    virtual SImageProperties getImageProperties() = 0;
};

class IControlSignalsObserver {
public:
    virtual ~IControlSignalsObserver(){}

    virtual void callbackSetTargetCoord(float lat, float lon, int alt) = 0;
};

#endif // COMMON_STUFF_H

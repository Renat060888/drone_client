#ifndef VIDEO_GENERATOR_H
#define VIDEO_GENERATOR_H

#include <string>
#include <thread>

#include <gst/gst.h>

#include "common_stuff.h"

// How to consume on other side:
// gst-launch-1.0 udpsrc port=5000 ! application/x-rtp,enconding-name=JPEG,payload=26 ! rtpjpegdepay ! jpegdec ! autovideosink
// gst-launch-1.0 udpsrc multicast-group=224.7.7.7 port=5000 auto-multicast=true ! application/x-rtp,enconding-name=JPEG,payload=26 ! rtpjpegdepay ! jpegdec ! autovideosink

// NOTE: RFC-2435 ( RTP Payload format for JPEG-comressed video: limit 2040 x 2040 )

class VideoGenerator
{
    friend gboolean callbackPushData( gpointer * _data );
    friend void callbackStartFeed( GstElement * _element, guint _size, gpointer _data );
    friend void callbackStopFeed( GstElement * _element, gpointer _data );
    friend gboolean callbackGstSourceMessage( GstBus * bus, GstMessage * message, gpointer user_data );
public:
    enum class EImageFormat {
        JPEG,
        RAW,
        UNDEFINED
    };

    struct SInitSettings {
        SInitSettings()
            : imageFormat(EImageFormat::UNDEFINED)
            , rtpEmitUdpPort(0)
            , imageProvider(nullptr)
            , enableMulticast(false)
        {}
        bool enableMulticast;
        std::string rtpEmitIp;
        int16_t rtpEmitUdpPort;
        EImageFormat imageFormat;        

        IImageProvider * imageProvider;        
    };

    struct SDataForTransfer {
        SDataForTransfer()
            : appsrc(nullptr)
            , dataSettings(nullptr)
            , sourceId(0)
            , samplesNum(0)
        {}
        GstElement * appsrc;
        SInitSettings * dataSettings;
        guint sourceId;
        guint64 samplesNum;
    };

    struct SState {
        SInitSettings settings;
        std::string lastError;
    };

    VideoGenerator();
    ~VideoGenerator();

    bool init( const SInitSettings & _settings );
    const SState & getState() const { return m_state; }

    bool connectToSource();


private:
    static gboolean callbackPushData( gpointer * _data );
    static void callbackStartFeed( GstElement * _element, guint _size, gpointer _data );
    static void callbackStopFeed( GstElement * _element, gpointer _data );
    static gboolean callbackGstSourceMessage( GstBus * bus, GstMessage * message, gpointer user_data );

    void disconnectFromSource();
    std::string definePipelineDescription( const SInitSettings & _settings );

    // data
    SState m_state;
    SDataForTransfer m_dataForTransfer;

    // service
    GstElement * m_gstPipeline;
    GMainLoop * m_glibMainLoop;
    std::thread * m_threadGLibMainLoop;

};

#endif // VIDEO_GENERATOR_H

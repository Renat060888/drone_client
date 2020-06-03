#ifndef VIDEO_CONVERTOR_H
#define VIDEO_CONVERTOR_H

#include <string>
#include <thread>
#include <vector>

#include <gst/gst.h>

#include "common_stuff.h"
#include "common_utils.h"

// How to consume on other side:
// gst-launch-1.0 udpsrc port=5000 ! application/x-rtp,enconding-name=JPEG,payload=26 ! rtpjpegdepay ! jpegdec ! autovideosink
// gst-launch-1.0 udpsrc multicast-group=224.7.7.7 port=5000 auto-multicast=true ! application/x-rtp,enconding-name=JPEG,payload=26 ! rtpjpegdepay ! jpegdec ! autovideosink

// NOTE: RFC-2435 ( RTP Payload format for JPEG-comressed video: limit 2040 x 2040 )

class VideoConvertor
{
    friend gboolean callbackGstSourceMessage( GstBus * bus, GstMessage * message, gpointer user_data );
public:
    struct SInitSettings {
        SInitSettings()
            : rtpEmitUdpPort(0)
            , enableMulticast(false)
        {}
        bool enableMulticast;
        std::string rtpEmitIp;
        int16_t rtpEmitUdpPort;
        std::string fileFullPath;
    };

    struct SState {
        SInitSettings settings;
        std::string lastError;
    };

    VideoConvertor();
    ~VideoConvertor();

    bool init( const SInitSettings & _settings );
    const SState & getState() const { return m_state; }

    void addObserver( IDroneStateObserver * _observer );

    bool connectToSource();


private:
    static gboolean callbackPushData( gpointer * _data );
    static void callbackStartFeed( GstElement * _element, guint _size, gpointer _data );
    static void callbackStopFeed( GstElement * _element, gpointer _data );
    static gboolean callbackGstSourceMessage( GstBus * bus, GstMessage * message, gpointer user_data );

    void threadMaintenance();

    void disconnectFromSource();
    std::string definePipelineDescription( const SInitSettings & _settings );

    bool parseTelemetryFile( const std::string & _filePath );
    void sendCurrentTelemetry( int _second, int _secondDecimalFraction );

    // data
    SState m_state;
    bool m_shutdownCalled;
    std::vector<std::vector<common_utils::TRow>> m_telemetryBySecondAndDecimalFraction;
    std::vector<IDroneStateObserver *> m_observers;

    // service
    GstElement * m_gstPipeline;
    GMainLoop * m_glibMainLoop;
    std::thread * m_threadGLibMainLoop;
    std::thread * m_threadMaintenance;
};

#endif // VIDEO_CONVERTOR_H

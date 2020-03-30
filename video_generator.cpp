
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>
#include <boost/format.hpp>

//#include <video_server_common/common/common_utils.h>
//#include <video_server_common/system/logger.h>

#include "video_generator.h"
#include "common_stuff.h"
#include "common_utils.h"

using namespace std;

static constexpr const char * PRINT_HEADER = "VideoGenerator:";
static constexpr const gint SAMPLE_RATE = 10;

VideoGenerator::VideoGenerator()
    : m_gstPipeline(nullptr)
    , m_glibMainLoop(nullptr)
    , m_threadGLibMainLoop(nullptr)
{

}

VideoGenerator::~VideoGenerator()
{
    disconnect();
}

bool VideoGenerator::init( const SInitSettings & _settings ){

//    assert( _settings.imageDataSrc
//            && _settings.rtpEmitUdpPort != 0
//            && _settings.imageFormat != EImageFormat::UNDEFINED );

    m_state.settings = _settings;
    m_dataForTransfer.dataSettings = & m_state.settings;

    if( ! connect(_settings) ){
        return false;
    }




    return true;
}

bool VideoGenerator::connect( const SInitSettings & _settings ){

    if( m_gstPipeline ){
        VS_LOG_WARN << PRINT_HEADER << " [" << _settings.rtpEmitUdpPort << "] already emit video stream" << endl;
        return true;
    }

    const string pipelineDscr = definePipelineDescription( _settings );
    if( pipelineDscr.empty() ){
        return false;
    }

    // launch
    GError * gerr = nullptr;
    m_gstPipeline = gst_parse_launch( pipelineDscr.c_str(), & gerr );
    if( gerr ){
        VS_LOG_ERROR << PRINT_HEADER << " couldn't construct pipeline: [1], gst message: [2] " << pipelineDscr << " " << gerr->message << endl;
        g_clear_error( & gerr );
        return false;
    }

    // to be notified of messages from this pipeline, mostly EOS
    GstBus * bus = gst_element_get_bus( m_gstPipeline );
    gst_bus_add_watch( bus, callbackGstSourceMessage, this );
    gst_object_unref( bus );

    // frames catch
    GstElement * appsrcElement = gst_bin_get_by_name( GST_BIN(m_gstPipeline), "images_src" );
    m_dataForTransfer.appsrc = appsrcElement;
    const gulong rt2 = g_signal_connect( appsrcElement, "need-data", G_CALLBACK(& callbackStartFeed), & m_dataForTransfer );
    const gulong rt3 = g_signal_connect( appsrcElement, "enough-data", G_CALLBACK(& callbackStopFeed), & m_dataForTransfer );

    // state
    gst_element_set_state( GST_ELEMENT( m_gstPipeline ), GST_STATE_PLAYING );

    // run loop
    m_glibMainLoop = g_main_loop_new( nullptr, FALSE );
    m_threadGLibMainLoop = new std::thread( g_main_loop_run, m_glibMainLoop );

    const string msg = ( boost::format( " started with follow pipeline [%1%]" )
                         % pipelineDscr
                         ).str();
    VS_LOG_INFO << PRINT_HEADER << msg << endl;

    return true;
}

void VideoGenerator::disconnect(){

    if( ! m_gstPipeline ){
        return;
    }

    // stop pipeline
    gst_element_send_event( m_gstPipeline, gst_event_new_eos() );

    GstStateChangeReturn ret = gst_element_set_state( m_gstPipeline, GST_STATE_PAUSED );
    ret = gst_element_set_state( m_gstPipeline, GST_STATE_NULL );
    if( GST_STATE_CHANGE_FAILURE == ret ){
        VS_LOG_ERROR << PRINT_HEADER << " unable to set the pipeline to the null state." << endl;
    }

    // destroy pipeline
    g_object_run_dispose( G_OBJECT( m_gstPipeline ) );
    gst_object_unref( m_gstPipeline );
    m_gstPipeline = nullptr;

    // destroy loop
    if( m_glibMainLoop ){
        g_main_loop_quit( m_glibMainLoop );
        g_main_loop_unref( m_glibMainLoop );
    }

    common_utils::threadShutdown( m_threadGLibMainLoop );

    VS_LOG_INFO << PRINT_HEADER
                << " video emit by [" << m_state.settings.rtpEmitUdpPort << "] is disconnected"
                << endl;
}

gboolean VideoGenerator::callbackPushData( gpointer * _data ){

    SDataForTransfer * data = ( SDataForTransfer * )_data;

    const std::pair<TConstDataPointer, TDataSize> & imgData = data->dataSettings->imageProvider->getImageData();
    int32_t bytesCount = imgData.second;
    const void * dataSrc = imgData.first;

    static constexpr gint SAMPLES_NUM = 1;    

    // create new empty buffer
    GstBuffer * buffer = gst_buffer_new_and_alloc( bytesCount );

    // set its timestamp and duration
    GST_BUFFER_TIMESTAMP( buffer ) = gst_util_uint64_scale( data->samplesNum, GST_SECOND, SAMPLE_RATE );
    GST_BUFFER_DURATION( buffer ) = gst_util_uint64_scale( SAMPLES_NUM, GST_SECOND, SAMPLE_RATE );

    // set data into buffer
    GstMapInfo map;
    gst_buffer_map( buffer, & map, GST_MAP_WRITE );

    ::memcpy( map.data, dataSrc, bytesCount );

    gst_buffer_unmap( buffer, & map );
    data->samplesNum += SAMPLES_NUM;

    // push the buffer info the appsrc
    GstFlowReturn rt;
    g_signal_emit_by_name( data->appsrc, "push-buffer", buffer, & rt );

    // free the buffer now that we are done with it
    gst_buffer_unref( buffer );

    if( rt != GST_FLOW_OK ){
        return FALSE;
    }

    return TRUE;
}

void VideoGenerator::callbackStartFeed( GstElement * _element, guint _size, gpointer _data ){

    SDataForTransfer * data = ( SDataForTransfer * )_data;
    if( 0 == data->sourceId ){
        data->sourceId = g_idle_add( (GSourceFunc)callbackPushData, data );
    }
}

void VideoGenerator::callbackStopFeed( GstElement * _element, gpointer _data ){

    SDataForTransfer * data = ( SDataForTransfer * )_data;
    if( data->sourceId != 0 ){
        g_source_remove( data->sourceId );
        data->sourceId = 0;
    }
}

/* called when we get a GstMessage from the source pipeline when we get EOS, we notify the appsrc of it. */
gboolean VideoGenerator::callbackGstSourceMessage( GstBus * _bus, GstMessage * _message, gpointer _userData ){

    switch( GST_MESSAGE_TYPE(_message) ){
    case GST_MESSAGE_EOS : {
        g_print("The source got dry ('END OF STREAM' message)");
        break;
    }
    case GST_MESSAGE_ERROR : {
        GError * err;
        gchar * debug;
        gst_message_parse_error( _message, & err, & debug );

        VS_LOG_ERROR << "VideoGenerator received error from GstObject: [" << _bus->object.name
                  << "] msg: [" << err->message
                  << "] Debug [" << debug
                  << "]"
                  << endl;

        g_error_free( err );
        g_free( debug );

        g_main_loop_quit( (( VideoGenerator * )_userData)->m_glibMainLoop );

        break;
    }
    case GST_MESSAGE_STATE_CHANGED : {

        break;
    }
    case GST_MESSAGE_BUFFERING : {

        break;
    }
    case GST_MESSAGE_ELEMENT : {

        break;
    }
    default:
        break;
    }

    return TRUE;
}

std::string VideoGenerator::definePipelineDescription( const SInitSettings & _settings ){

    const string source =
        ( boost::format( "appsrc name=images_src format=bytes do-timestamp=true caps=image/jpeg,framerate=%4%/1,stream-format=byte-stream,width=%2%,height=%3%" // JUST IN CASE:
                         " ! rtpjpegpay pt=26 name=pay0" // JUST IN CASE: mtu=65000
                         " ! udpsink host=127.0.0.1 port=%1% sync=true enable-last-sample=false send-duplicates=false " // JUST IN CASE: sync=true enable-last-sample=false send-duplicates=false
                       )
        % _settings.rtpEmitUdpPort
        % _settings.imageProvider->getImageProperties().width
        % _settings.imageProvider->getImageProperties().height
        % SAMPLE_RATE
       ).str();

    return source;
}


















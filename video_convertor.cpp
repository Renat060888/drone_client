
#include <iomanip>

#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>
#include <boost/format.hpp>
#include "from_ms_common/system/logger.h"

#include "video_convertor.h"
#include "common_stuff.h"

using namespace std;

static constexpr const char * PRINT_HEADER = "VideoConvertor:";

VideoConvertor::VideoConvertor()
    : m_shutdownCalled(false)
    , m_gstPipeline(nullptr)
    , m_glibMainLoop(nullptr)
    , m_threadGLibMainLoop(nullptr)
    , m_threadMaintenance(nullptr)
{

}

VideoConvertor::~VideoConvertor()
{
    m_shutdownCalled = true;
    common_utils::threadShutdown( m_threadMaintenance );
    disconnectFromSource();
}

bool VideoConvertor::init( const SInitSettings & _settings ){

//    assert( _settings.imageDataSrc
//            && _settings.rtpEmitUdpPort != 0
//            && _settings.imageFormat != EImageFormat::UNDEFINED );

    m_state.settings = _settings;
    m_threadMaintenance = new std::thread( & VideoConvertor::threadMaintenance, this );

    const int lastDotPos = _settings.fileFullPath.find_last_of(".");
    const std::string csvFilePath = _settings.fileFullPath.substr( 0, lastDotPos ) + ".csv";

    if( ! parseTelemetryFile(csvFilePath) ){
        return false;
    }

    return true;
}

void VideoConvertor::addObserver( IDroneStateObserver * _observer ){
    m_observers.push_back( _observer );
}

void VideoConvertor::threadMaintenance(){

    static constexpr int64_t CURRENT_POSITION_POLL_INTERVAL_MILLISEC = 100;
    static int64_t lastCurrentPositionPollAtMillisec = 0;

    while( ! m_shutdownCalled ){

        if( m_gstPipeline && (common_utils::getCurrentTimeMillisec() - lastCurrentPositionPollAtMillisec) > CURRENT_POSITION_POLL_INTERVAL_MILLISEC ){
            lastCurrentPositionPollAtMillisec = common_utils::getCurrentTimeMillisec();

            gint64 positionNanosec = 0;
            gst_element_query_position( m_gstPipeline, GST_FORMAT_TIME, & positionNanosec );
            const int64_t posMillisec = positionNanosec / 1000 / 1000;

//            VS_LOG_INFO << PRINT_HEADER
//                        << " (millisec): " << posMillisec
//                        << " (cutted, -2): " << posMillisec / 100
//                        << " int: " << posMillisec / 100 / 10
//                        << " fract: " << posMillisec / 100 % 10
//                        << endl;

            sendCurrentTelemetry( posMillisec / 100 / 10, posMillisec / 100 % 10 );
        }

        std::this_thread::sleep_for( chrono::milliseconds(10) );
    }
}

bool VideoConvertor::connectToSource(){

    const SInitSettings & settings = m_state.settings;

    if( m_gstPipeline ){
        VS_LOG_WARN << PRINT_HEADER << " [" << settings.rtpEmitUdpPort << "] already emit video stream" << endl;
        return true;
    }

    const string pipelineDscr = definePipelineDescription( settings );
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

void VideoConvertor::disconnectFromSource(){

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

/* called when we get a GstMessage from the source pipeline when we get EOS, we notify the appsrc of it. */
gboolean VideoConvertor::callbackGstSourceMessage( GstBus * _bus, GstMessage * _message, gpointer _userData ){

    switch( GST_MESSAGE_TYPE(_message) ){
    case GST_MESSAGE_EOS : {
        g_print("The source got dry ('END OF STREAM' message)");
        break;
    }
    case GST_MESSAGE_ERROR : {
        GError * err;
        gchar * debug;
        gst_message_parse_error( _message, & err, & debug );

        VS_LOG_ERROR << PRINT_HEADER
                     << " received error from GstObject: [" << _bus->object.name
                     << "] msg: [" << err->message
                     << "] Debug [" << debug
                     << "]"
                     << endl;

        g_error_free( err );
        g_free( debug );
        break;
    }
    case GST_MESSAGE_WARNING : {
        GError * err;
        gchar * debug;
        gst_message_parse_warning( _message, & err, & debug );

        VS_LOG_ERROR << PRINT_HEADER
                     << " received warning from GstObject: [" << _bus->object.name
                     << "] msg: [" << err->message
                     << "] Debug [" << debug
                     << "]"
                     << endl;

        g_error_free( err );
        g_free( debug );
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

std::string VideoConvertor::definePipelineDescription( const SInitSettings & _settings ){

    const string autoMulticastTrue = ( _settings.enableMulticast ? "auto-multicast=true" : "" );

    const string source =
        ( boost::format( "filesrc location=%4% do-timestamp=true"
                         " ! matroskademux "
                         " ! rtpjpegpay pt=26 name=pay0"
                         " ! udpsink host=%1% port=%2% %3% sync=true enable-last-sample=false send-duplicates=false "
                       )
        % _settings.rtpEmitIp
        % _settings.rtpEmitUdpPort
        % autoMulticastTrue
        % _settings.fileFullPath
       ).str();

    return source;
}

void VideoConvertor::sendCurrentTelemetry( int _second, int _secondDecimalFraction ){

    if( _second < m_telemetryBySecondAndDecimalFraction.size() ){
        const std::vector<common_utils::TRow> & secondRows = m_telemetryBySecondAndDecimalFraction[ _second ];

        if( _secondDecimalFraction < secondRows.size() ){
            const common_utils::TRow & row = secondRows[ _secondDecimalFraction ];

            if( ! row.empty() ){
//                VS_LOG_INFO << PRINT_HEADER << row[ 1 ] << endl;

                for( IDroneStateObserver * observer : m_observers ){
                    observer->callbackBoardPositionChanged( std::stod(row[ 3 ]), std::stod(row[ 4 ]), std::stod(row[ 7 ]) );
                    observer->callbackCameraPositionChanged( std::stod(row[ 30 ]), std::stod(row[ 31 ]), std::stod(row[ 32 ]), 0 );
                }
            }
        }
    }
}

bool VideoConvertor::parseTelemetryFile( const string & _filePath ){

    std::locale::global( std::locale("C.UTF-8") );

    // read example file
    std::ifstream csvTelemetryFile( _filePath );
    if( ! csvTelemetryFile.is_open() ){
        VS_LOG_ERROR << PRINT_HEADER << " failed to open [" << _filePath << "]" << endl;
        return false;
    }
    string buffer( {std::istreambuf_iterator<char>(csvTelemetryFile), std::istreambuf_iterator<char>()} );

    const std::vector<common_utils::TRow> parsedCSV  = common_utils::parseCSVFile( buffer.data(), buffer.size() );

    // ( without header )
    for( int i = 1; i < parsedCSV.size(); i++ ){
        const common_utils::TRow & row = parsedCSV[ i ];
        const double timeSec = ::atof( row[ 1 ].c_str() );

        double intPartF = 0;
        const double fractPartF = ::modf( timeSec, & intPartF );
        const int intPart = intPartF;
        const int fractPart = ( 0 == fractPartF ? 0 : (fractPartF + 0.01f) * 10 ); // NOTE 0.01 for rounding 0.099999...

        // new second
        if( intPart > (int)(m_telemetryBySecondAndDecimalFraction.size()-1) ){
            m_telemetryBySecondAndDecimalFraction.resize( m_telemetryBySecondAndDecimalFraction.size()
                    + (intPart - m_telemetryBySecondAndDecimalFraction.size() + 1) );

            std::vector<common_utils::TRow> & secondRows = m_telemetryBySecondAndDecimalFraction[ intPart ];
            secondRows.resize( 10 );
            secondRows[ fractPart ] = row;
        }
        // second already exist
        else{
            std::vector<common_utils::TRow> & secondRows = m_telemetryBySecondAndDecimalFraction[ intPart ];
            secondRows[ fractPart ] = row;
        }
    }

    return true;
}

#if 0
// new second
if( intPart > (int)(m_telemetryBySecondAndDecimalFraction.size()-1) ){
    m_telemetryBySecondAndDecimalFraction.resize( m_telemetryBySecondAndDecimalFraction.size() + 1 );

    std::vector<common_utils::TRow> & secondRows = m_telemetryBySecondAndDecimalFraction[ intPart ];
    secondRows.resize( 47 );

    if( fractPart > (int)(secondRows.size()-1) ){
        secondRows.resize( secondRows.size() + (fractPart - secondRows.size() + 1) );

        secondRows[ fractPart ] = row;
    }
    else{
        assert( false && "it shouldn't happen" );
    }
}
// second already exist
else{
    std::vector<common_utils::TRow> & secondRows = m_telemetryBySecondAndDecimalFraction[ intPart ];

    if( fractPart > (int)(secondRows.size()-1) ){
        secondRows.resize( secondRows.size() + (fractPart - secondRows.size() + 1) );

        secondRows[ fractPart ] = row;
    }
    else{
        assert( false && "it shouldn't happen" );
    }
}
#endif


















#include <regex>

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include "config_reader.h"

using namespace std;

static constexpr const char * PRINT_HEADER = "ConfigReader";

static bool isIpAddress( const string & _str ){

    std::regex rgxIpAddress( "[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}" );
    std::smatch match;

    return std::regex_search( _str.begin(), _str.end(), match, rgxIpAddress );
}

ConfigReader::ConfigReader()
{

}


bool ConfigReader::initDerive( const SIninSettings & _settings ){

    m_parameters.baseParams = AConfigReader::m_parameters;
    return true;
}

bool ConfigReader::parse( const std::string & _content ){

    // parse base part
    boost::property_tree::ptree config;

    istringstream contentStream( _content );
    try{
        boost::property_tree::json_parser::read_json( contentStream, config );
    }
    catch( boost::property_tree::json_parser::json_parser_error & _ex ){
        PRELOG_ERR << ::PRINT_HEADER << " parse failed of [" << _content << "]" << endl
             << "Reason: [" << _ex.what() << "]" << endl;
        return false;
    }

    boost::property_tree::ptree droneControl = config.get_child("drone_control");
    m_parameters.DRONE_CONTROL_CARRIER_OBJECT_ID = setParameterNew<int64_t>( droneControl, "carrier_object_id", 0 );
    m_parameters.DRONE_CONTROL_CAMERA_OBJECT_ID = setParameterNew<int64_t>( droneControl, "camera_object_id", 0 );
    m_parameters.DRONE_CONTROL_LIB_CONFIG_PATH = setParameterNew<std::string>( droneControl, "lib_config_path", "" );
    m_parameters.DRONE_CONTROL_PING_TIMEOUT_MILLISEC = setParameterNew<int64_t>( droneControl, "ping_timeout_millisec", 10000 );
    m_parameters.DRONE_CONTROL_IMITATION_ENABLE = setParameterNew<bool>( droneControl, "moving_imitation", false );

    boost::property_tree::ptree videoStreaming = config.get_child("video_streaming");
    boost::property_tree::ptree videoStreamingInput = videoStreaming.get_child("input");
    boost::property_tree::ptree videoStreamingOutput = videoStreaming.get_child("output");
    m_parameters.VIDEO_STREAMING_SRC_TYPE = setParameterNew<std::string>( videoStreamingInput, "video_source_type", "" );
    m_parameters.VIDEO_STREAMING_IMAGES_DIR_PATH = setParameterNew<std::string>( videoStreamingInput, "images_dir_abs_path", "" );
    m_parameters.VIDEO_STREAMING_VIDEO_FILE_PATH = setParameterNew<std::string>( videoStreamingInput, "video_file_abs_path", "" );
    m_parameters.VIDEO_STREAMING_DUMP_INCOMING_DATA = setParameterNew<bool>( videoStreamingInput, "dump_incoming_data", false );
    m_parameters.VIDEO_STREAMING_INCOMING_DATA_DUMP_PATH = setParameterNew<std::string>( videoStreamingInput, "incoming_data_dump_path", "" );
    m_parameters.VIDEO_STREAMING_UNICAST_DEST_IP = setParameterNew<std::string>( videoStreamingOutput, "unicast_dest_ip", "" );
    m_parameters.VIDEO_STREAMING_DEST_PORT = setParameterNew<int64_t>( videoStreamingOutput, "dest_port", 5000 );
    m_parameters.VIDEO_STREAMING_ENABLE_MULTICAST = setParameterNew<bool>( videoStreamingOutput, "enable_multicast", false );
    m_parameters.VIDEO_STREAMING_MULTICAST_GROUP_IP = setParameterNew<std::string>( videoStreamingOutput, "multicast_group_ip", "" );
    m_parameters.VIDEO_STREAMING_STATUS_OVERLAY = setParameterNew<bool>( videoStreamingOutput, "status_overlay", false );

    if( m_parameters.VIDEO_STREAMING_DUMP_INCOMING_DATA && ! common_utils::isDirectory(m_parameters.VIDEO_STREAMING_INCOMING_DATA_DUMP_PATH) ){
        PRELOG_ERR << ::PRINT_HEADER << " incorrect 'incoming data dump' path" << endl;
        return false;
    }

    if( ! common_utils::isDirectory(m_parameters.VIDEO_STREAMING_IMAGES_DIR_PATH) ){
        PRELOG_ERR << ::PRINT_HEADER << " incorrect 'images dir' path" << endl;
        return false;
    }

    if( ! common_utils::isFile(m_parameters.DRONE_CONTROL_LIB_CONFIG_PATH) ){
        PRELOG_ERR << ::PRINT_HEADER << " incorrect 'lib config' path" << endl;
        return false;
    }

    if( "video-file" == m_parameters.VIDEO_STREAMING_SRC_TYPE ){
        if( ! common_utils::isFile(m_parameters.VIDEO_STREAMING_VIDEO_FILE_PATH) ){
            PRELOG_ERR << ::PRINT_HEADER << " incorrect 'video file' path" << endl;
            return false;
        }
    }

    if( ! isIpAddress(m_parameters.VIDEO_STREAMING_UNICAST_DEST_IP) ){
        PRELOG_ERR << ::PRINT_HEADER << " incorrect 'unicast dest ip' address" << endl;
        return false;
    }

    if( ! isIpAddress(m_parameters.VIDEO_STREAMING_MULTICAST_GROUP_IP) ){
        PRELOG_ERR << ::PRINT_HEADER << " incorrect 'multicast group ip' address" << endl;
        return false;
    }

    return true;
}

bool ConfigReader::createCommandsFromConfig( const std::string & _content ){

    // dummy
    return true;
}

std::string ConfigReader::getConfigExampleDerive(){

    assert( false && "TODO: do" );
    return std::string();
}

#ifndef CONFIG_READER_H
#define CONFIG_READER_H

#include "from_ms_common/system/a_config_reader.h"

class ConfigReader : public AConfigReader
{
public:
    struct SConfigParameters {
        AConfigReader::SConfigParameters baseParams;

        int64_t DRONE_CONTROL_CARRIER_OBJECT_ID;
        int64_t DRONE_CONTROL_CAMERA_OBJECT_ID;
        std::string DRONE_CONTROL_LIB_CONFIG_PATH;
        int64_t DRONE_CONTROL_PING_TIMEOUT_MILLISEC;
        bool DRONE_CONTROL_IMITATION_ENABLE;

        std::string VIDEO_STREAMING_SRC_TYPE;
        std::string VIDEO_STREAMING_IMAGES_DIR_PATH;
        std::string VIDEO_STREAMING_VIDEO_FILE_PATH;

        bool VIDEO_STREAMING_ENABLE_MULTICAST;
        std::string VIDEO_STREAMING_MULTICAST_GROUP_IP;
        std::string VIDEO_STREAMING_UNICAST_DEST_IP;
        int VIDEO_STREAMING_DEST_PORT;
        bool VIDEO_STREAMING_STATUS_OVERLAY;
        bool VIDEO_STREAMING_DUMP_INCOMING_DATA;
        std::string VIDEO_STREAMING_INCOMING_DATA_DUMP_PATH;
    };

    static ConfigReader & singleton(){
        static ConfigReader instance;
        return instance;
    }

    const SConfigParameters & get(){ return m_parameters; }


private:
    ConfigReader();
    ~ConfigReader(){}

    ConfigReader( const ConfigReader & _inst ) = delete;
    ConfigReader & operator=( const ConfigReader & _inst ) = delete;

    virtual bool initDerive( const SIninSettings & _settings ) override;
    virtual bool parse( const std::string & _content ) override;
    virtual bool createCommandsFromConfig( const std::string & _content ) override;
    virtual std::string getConfigExampleDerive() override;


    // data
    SConfigParameters m_parameters;
};
#define CONFIG_READER ConfigReader::singleton()
#define CONFIG_PARAMS ConfigReader::singleton().get()

#endif // CONFIG_READER_H

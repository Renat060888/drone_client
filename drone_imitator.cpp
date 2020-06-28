
#include "from_ms_common/system/logger.h"

#include "drone_imitator.h"

using namespace std;

static constexpr const char * PRINT_HEADER = "DroneImitator:";
static constexpr double ANGLES_IN_ZOOM_UNIT = ( 57.8 - 2.0 ) / 255.0;
static constexpr int ZOOM_STEP = 2;

DroneImitator::DroneImitator()
{

}

void DroneImitator::threadMaintenance(){



}

// signals from objrepr
void DroneImitator::callbackSetTargetCoord(float lat, float lon, int alt){

}

void DroneImitator::callbackSetMode( EDroneMode _mode ){

    VS_LOG_INFO << PRINT_HEADER << " callbackSetMode:" << ((int)_mode) << endl;


}

void DroneImitator::callbackSetWantAzimut( double _azimutAbsDeg ){

}

void DroneImitator::callbackSetAzimutChangeRate( double _azimutIncDeg ){

}

void DroneImitator::callbackSetWantElevation( double _elevationAbsDeg ){

}

void DroneImitator::callbackSetElevationChangeRate( double _elevationIncDeg ){

}

void DroneImitator::callbackSetFocusChangeRate( double _rate ){

}

void DroneImitator::callbackSetZoomChangeRate( double _rate ){

}

void DroneImitator::callbackSetDiaphragmMode( ECameraDiaphragmMode _mode ){

}

void DroneImitator::callbackSetTxRxScale( double _scale ){

}

void DroneImitator::callbackSetShowAim( bool _show ){

}

void DroneImitator::callbackSetShowTelemetry( bool _show ){

}

void DroneImitator::callbackStartVideoStream( bool _start ){

}

void DroneImitator::callbackSwitchOn( bool _on, const std::string _runSettings ){

}

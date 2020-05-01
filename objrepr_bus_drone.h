#ifndef OBJREPR_BUS_DRONE_H
#define OBJREPR_BUS_DRONE_H

#include "from_ms_common/system/objrepr_bus.h"

class ObjreprBusDrone : public ObjreprBus
{
public:
    ObjreprBusDrone();

    // NOTE: stuff for derived classes ( video-server-object, player-object, etc... )



private:





};
#define OBJREPR_BUS ObjreprBusDrone::singleton()

#endif // OBJREPR_BUS_DRONE_H

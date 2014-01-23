#pragma once

#include "../gamecommon/GameObject.h"

BEGIN_DECLNETMESSAGE(SpaceShipConstructionData, 5002, false)
osg::Vec3f pos;
osg::Vec4f orient;
END_DECLNETMESSAGE()

class SpaceShipServer : public GameObject
{
public:

    virtual bool takeMessage(const GameMessage::const_pointer& msg, MessagePeer* sender);
};

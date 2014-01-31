#pragma once

#include "../gamecommon/GameObject.h"
#include <osg/Quat>

BEGIN_DECLGAMEMESSAGE(SpaceShipConstructionData, 5002, false)
osg::Vec3f pos;
osg::Vec4f orient;
uint32_t ownerId;
END_DECLGAMEMESSAGE()

BEGIN_DECLGAMEMESSAGE(SpaceShipControl, 5003, true)
uint16_t inputType;
bool isOn;
END_DECLGAMEMESSAGE()

BEGIN_DECLGAMEMESSAGE(SpaceShipPhysicsUpdate, 5004, true)
osg::Vec3f pos;
osg::Vec4f orient;
osg::Vec3f velocity;
END_DECLGAMEMESSAGE()

BEGIN_DECLGAMEMESSAGE(SpaceShipCollision, 5005, true)
END_DECLGAMEMESSAGE()

class GameInstanceServer;
class PhysicsEngine;
class ShipPhysicsActor;

class SpaceShipServer : public GameObject
{
public:
    typedef std::shared_ptr<SpaceShipServer> pointer;
    enum inputType
    {
        ACCELERATE,
        LEFT,
        RIGHT,
        UP,
        DOWN,
        BACK
    };

    SpaceShipServer(osg::Vec3f startPos, osg::Quat orient, uint32_t ownerId, GameInstanceServer* context, const std::shared_ptr<PhysicsEngine>& eng);
    virtual ~SpaceShipServer();

    virtual bool takeMessage(const GameMessage::const_pointer& msg, MessagePeer* sender);

    GameMessage::pointer creationMessage() const;

protected:
    float m_acceleration;
    float m_steerability;

    bool m_inputState[6];
    float m_timeSinceLastUpdate;
    std::shared_ptr<PhysicsEngine> m_engine;
    ShipPhysicsActor* m_actor;
    unsigned int m_physicsId;

    // event related functions
    void onControlMessage(uint16_t inputType, bool on);
    void onPhysicsUpdate(const osg::Vec3f& newPos, const osg::Quat& newRot);
};

#include "GameInstanceServer.h"
#include "SpaceShipServer.h"
#include "../gamecommon/PhysicsEngine.h"
#include "../gamecommon/ShipPhysicsActor.h"

BEGIN_GAMETORAWMESSAGE_QCONVERT(SpaceShipConstructionData)
out << pos << orient << ownerId;
END_GAMETORAWMESSAGE_QCONVERT()
BEGIN_RAWTOGAMEMESSAGE_QCONVERT(SpaceShipConstructionData)
in >> pos >> orient >> ownerId;
END_RAWTOGAMEMESSAGE_QCONVERT()

BEGIN_GAMETORAWMESSAGE_QCONVERT(SpaceShipControl)
out << inputType << isOn;
END_GAMETORAWMESSAGE_QCONVERT()
BEGIN_RAWTOGAMEMESSAGE_QCONVERT(SpaceShipControl)
in >> inputType >> isOn;
END_RAWTOGAMEMESSAGE_QCONVERT()

BEGIN_GAMETORAWMESSAGE_QCONVERT(SpaceShipPhysicsUpdate)
out << pos << orient << velocity;
END_GAMETORAWMESSAGE_QCONVERT()
BEGIN_RAWTOGAMEMESSAGE_QCONVERT(SpaceShipPhysicsUpdate)
in >> pos >> orient >> velocity;
END_RAWTOGAMEMESSAGE_QCONVERT()

BEGIN_GAMETORAWMESSAGE_QCONVERT(SpaceShipCollision)
END_GAMETORAWMESSAGE_QCONVERT()
BEGIN_RAWTOGAMEMESSAGE_QCONVERT(SpaceShipCollision)
END_RAWTOGAMEMESSAGE_QCONVERT()

SpaceShipServer::SpaceShipServer(osg::Vec3f startPos, osg::Quat orient, uint32_t ownerId, GameInstanceServer* ctx, const std::shared_ptr<PhysicsEngine>& eng):
GameObject(ownerId, ctx),
m_acceleration(30.0f),
m_steerability(0.1f),
m_timeSinceLastUpdate(10000.0f),
m_actor(nullptr),
m_physicsId(0)
{
    for (unsigned int i = 0; i < 6; ++i) m_inputState[i] = false;

    m_engine = eng;
    m_physicsId = eng->addUserVehicle(startPos, osg::Vec3f(0.1f, 0.1f, 0.3f), orient, 10.0f);
    eng->addMotionCallback(m_physicsId, std::bind(&SpaceShipServer::onPhysicsUpdate, this, std::placeholders::_1, std::placeholders::_2));
    m_actor = eng->getActorById(m_physicsId);
}

bool SpaceShipServer::takeMessage(const GameMessage::const_pointer& msg, MessagePeer* sender)
{
    if (msg->gettype() == GameSpaceShipControlMessage::type)
    {
        GameSpaceShipControlMessage::const_pointer realMsg = msg->as<GameSpaceShipControlMessage>();
        onControlMessage(realMsg->inputType, realMsg->isOn);
        return true;
    }
    return false;
}

void SpaceShipServer::onControlMessage(uint16_t inputType, bool on)
{
    if (inputType < 6)
    {
        m_inputState[inputType] = on;
        if (m_inputState[ACCELERATE] != m_inputState[BACK])
        {
            //Set force vector
            m_actor->setForceVector(osg::Vec3f(0, 0, m_inputState[ACCELERATE] ? -m_acceleration : m_acceleration));
        }
        else
            m_actor->setForceVector(osg::Vec3f(0, 0, 0));
        // build torque vector
        osg::Vec3f torque(0, 0, 0);
        if (m_inputState[LEFT] != m_inputState[RIGHT])
        {
            torque += osg::Vec3f(0, 0, m_inputState[LEFT] ? m_steerability : -m_steerability);
        }
        if (m_inputState[UP] != m_inputState[DOWN])
        {
            torque += osg::Vec3f(m_inputState[UP] ? -m_steerability : m_steerability, 0, 0);
        }
        m_actor->setRotationAxis(torque);
    }
}

GameMessage::pointer SpaceShipServer::creationMessage() const
{
    GameSpaceShipConstructionDataMessage::pointer msg{ new GameSpaceShipConstructionDataMessage };
    btTransform trans;
    m_engine->getBodyById(m_physicsId)->getMotionState()->getWorldTransform(trans);
    btVector3 translate(trans.getOrigin());
    btQuaternion rotate(trans.getRotation());

    msg->pos.set(translate.x(), translate.y(), translate.z());
    msg->orient.set(rotate.x(), rotate.y(), rotate.z(), rotate.w());
    msg->objectId = m_myObjectId;
    msg->ownerId = m_myOwnerId;
    return msg;
}

void SpaceShipServer::onPhysicsUpdate(const osg::Vec3f& newPos, const osg::Quat& newRot)
{
    if (true)
    {
        m_timeSinceLastUpdate = 0;
        GameSpaceShipPhysicsUpdateMessage::pointer msg{ new GameSpaceShipPhysicsUpdateMessage };
        msg->pos = newPos;
        msg->orient = newRot.asVec4();
        msg->velocity.set(0, 0, 0);
        msg->objectId = m_myObjectId;
        messageToPartner(msg);
    }
}

unsigned int SpaceShipServer::getPhysicsId()
{
    return m_physicsId;
}


SpaceShipServer::~SpaceShipServer()
{
    m_engine->removeVehicle(m_physicsId);
}

REGISTER_GAMEMESSAGE(SpaceShipConstructionData)
REGISTER_GAMEMESSAGE(SpaceShipControl)
REGISTER_GAMEMESSAGE(SpaceShipPhysicsUpdate)
REGISTER_GAMEMESSAGE(SpaceShipCollision)

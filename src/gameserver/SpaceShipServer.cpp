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

SpaceShipServer::SpaceShipServer(osg::Vec3f startPos, osg::Quat orient, uint32_t ownerId, GameInstanceServer* ctx):
GameObject(ownerId, ctx),
m_pos(startPos),
m_orientation(orient),
m_velocity(osg::Vec3f()),
m_acceleration(20.0f),
m_steerability(0.9f),
m_friction(0.7f),
m_timeSinceLastUpdate(10000.0f),
m_actor(nullptr),
m_physicsId(0)
{
    for (unsigned int i = 0; i < 6; ++i) m_inputState[i] = false;
    //GameMessage::pointer crMessage = creationMessage();
    //m_context->broadcastLocally(crMessage);
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
            torque += osg::Vec3f(0, m_inputState[LEFT] ? m_steerability : -m_steerability, 0);
        }
        if (m_inputState[UP] != m_inputState[DOWN])
        {
            torque += osg::Vec3f(m_inputState[UP] ? -m_steerability : m_steerability, 0, 0);
        }
        m_actor->setRotationAxis(torque);
    }
}

/*void SpaceShipServer::timeTick(float dt)
{
    //position update
    m_pos += m_velocity*dt;

    //Friction
    m_velocity *= pow(m_friction, dt);

    //handle input
    if (m_inputState[ACCELERATE] != m_inputState[BACK])
    {
        if (m_inputState[ACCELERATE])
            m_velocity += m_orientation*osg::Vec3f(0, 0, -m_acceleration * dt);
        else
            m_velocity += m_orientation*osg::Vec3f(0, 0, m_acceleration * dt);
    }

    if (m_inputState[LEFT] != m_inputState[RIGHT]){
        float amount = m_steerability * dt;
        if (m_inputState[LEFT]){
            //roll left
            osg::Quat q(amount, osg::Vec3f(0, 0, 1));
            m_orientation = q * m_orientation;
        }
        else{
            //roll right
            osg::Quat q(-amount, osg::Vec3f(0, 0, 1));
            m_orientation = q * m_orientation;
        }
    }
    if (m_inputState[UP] != m_inputState[DOWN]){
        float amount = m_steerability * dt;
        if (m_inputState[UP]){
            //rear up
            osg::Quat q(-amount, osg::Vec3f(1, 0, 0));
            m_orientation = q * m_orientation;
        }
        else{
            //rear down
            osg::Quat q(amount, osg::Vec3f(1, 0, 0));
            m_orientation = q * m_orientation;
        }
    }

    // Send update messages
    m_timeSinceLastUpdate += dt;
    if (m_timeSinceLastUpdate > 0.02f)
    {
        m_timeSinceLastUpdate = 0;
        GameSpaceShipPhysicsUpdateMessage::pointer msg{ new GameSpaceShipPhysicsUpdateMessage };
        msg->pos = m_pos;
        msg->orient = m_orientation.asVec4();
        msg->velocity = m_velocity;
        msg->objectId = m_myObjectId;
        messageToPartner(msg);
    }
}*/

GameMessage::pointer SpaceShipServer::creationMessage() const
{
    GameSpaceShipConstructionDataMessage::pointer msg{ new GameSpaceShipConstructionDataMessage };
    msg->pos = m_pos;
    msg->orient = m_orientation.asVec4();
    msg->objectId = m_myObjectId;
    msg->ownerId = m_myOwnerId;
    return msg;
}

void SpaceShipServer::addToPhysicsEngine(const std::shared_ptr<PhysicsEngine>& engine)
{
    m_engine = engine;
    m_physicsId = engine->addUserVehicle(m_pos, osg::Vec3f(1.f, 1.f, 3.f), osg::Quat(), 10.0f);
    engine->addMotionCallback(m_physicsId, std::bind(&SpaceShipServer::onPhysicsUpdate, this, std::placeholders::_1, std::placeholders::_2));
    m_actor = engine->getActorById(m_physicsId);
}

void SpaceShipServer::onPhysicsUpdate(const osg::Vec3f& newPos, const osg::Quat& newRot)
{
    if (true)
    {
        m_timeSinceLastUpdate = 0;
        GameSpaceShipPhysicsUpdateMessage::pointer msg{ new GameSpaceShipPhysicsUpdateMessage };
        msg->pos = newPos;
        msg->orient = newRot.asVec4();
        msg->velocity = m_velocity;
        msg->objectId = m_myObjectId;
        messageToPartner(msg);
    }
}

REGISTER_GAMEMESSAGE(SpaceShipConstructionData)
REGISTER_GAMEMESSAGE(SpaceShipControl)
REGISTER_GAMEMESSAGE(SpaceShipPhysicsUpdate)
REGISTER_GAMEMESSAGE(SpaceShipCollision)

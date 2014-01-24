#include "GameInstanceServer.h"
#include "SpaceShipServer.h"

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

SpaceShipServer::SpaceShipServer(osg::Vec3f startPos, osg::Vec4f orient, uint32_t ownerId, GameInstanceServer* ctx):
GameObject(ownerId, ctx),
m_pos(startPos),
m_orientation(orient),
m_velocity(osg::Vec3f()),
m_acceleration(4.0f),
m_steerability(2.0f),
m_friction(0.7f)
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
        m_inputState[inputType] = on;
}

void SpaceShipServer::timeTick(float dt)
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
    GameSpaceShipPhysicsUpdateMessage::pointer msg{ new GameSpaceShipPhysicsUpdateMessage };
    msg->pos = m_pos;
    msg->orient = m_orientation.asVec4();
    msg->velocity = m_velocity;
    msg->objectId = m_myObjectId;
    m_context->broadcastLocally(msg);
}

GameMessage::pointer SpaceShipServer::creationMessage() const
{
    GameSpaceShipConstructionDataMessage::pointer msg{ new GameSpaceShipConstructionDataMessage };
    msg->pos = m_pos;
    msg->orient = m_orientation.asVec4();
    msg->objectId = m_myObjectId;
    msg->ownerId = m_myOwnerId;
    return msg;
}

REGISTER_GAMEMESSAGE(SpaceShipConstructionData)
REGISTER_GAMEMESSAGE(SpaceShipControl)
REGISTER_GAMEMESSAGE(SpaceShipPhysicsUpdate)
REGISTER_GAMEMESSAGE(SpaceShipCollision)

#include "GameInstanceServer.h"
#include "SpaceShipServer.h"
#include "../gamecommon/PhysicsEngine.h"
#include "../gamecommon/ShipPhysicsActor.h"

template <> MessageMetaData
GameSpaceShipConstructionDataMessage::base::m_metaData = MessageMetaData::createMetaData<GameSpaceShipConstructionDataMessage>("pos\norient\nownerId\nplayerName");

template <> MessageMetaData
GameSpaceShipControlMessage::base::m_metaData = MessageMetaData::createMetaData<GameSpaceShipControlMessage>("inputType\nisOn", NetMessage::MESSAGE_PREFERS_UDP | NetMessage::MESSAGE_HIGH_PRIORITY);

template <> MessageMetaData
GameSpaceShipPhysicsUpdateMessage::base::m_metaData = MessageMetaData::createMetaData<GameSpaceShipPhysicsUpdateMessage>("pos\norient\nvelocity", NetMessage::MESSAGE_PREFERS_UDP | NetMessage::MESSAGE_OVERRIDES_PREVIOUS);

template <> MessageMetaData
GameSpaceShipCollisionMessage::base::m_metaData = MessageMetaData::createMetaData<GameSpaceShipCollisionMessage>("", NetMessage::MESSAGE_PREFERS_UDP);

template <> MessageMetaData
GameScoreUpdateMessage::base::m_metaData = MessageMetaData::createMetaData<GameSpaceShipCollisionMessage>("score");

SpaceShipServer::SpaceShipServer(std::string name, osg::Vec3f startPos, osg::Quat orient, uint32_t ownerId, GameInstanceServer* ctx, const std::shared_ptr<PhysicsEngine>& eng):
GameObject(ownerId, ctx),
m_acceleration(60.0f),
m_steerability(0.4f),
m_actor(nullptr),
m_physicsId(0),
m_playerScore(0)
{
    playerName = name;
    m_lastUpdate = std::chrono::steady_clock::now();
    for (unsigned int i = 0; i < 6; ++i) m_inputState[i] = false;

    m_engine = eng;
    m_physicsId = eng->addUserVehicle(startPos, osg::Vec3f(0.1f, 0.1f, 0.3f), orient, 10.0f);
    // das drehen des raumschiffs soll nicht so weich sein: eng->getBodyById(m_physicsId)->setRollingFriction(0.5f);
    eng->addMotionCallback(m_physicsId, std::bind(&SpaceShipServer::onPhysicsUpdate, this, std::placeholders::_1, std::placeholders::_2));
    m_actor = eng->getActorById(m_physicsId);
}

bool SpaceShipServer::takeMessage(const GameMessage::const_pointer& msg, MessagePeer* sender)
{
    if (msg->gettype() == GameSpaceShipControlMessage::type)
    {
        GameSpaceShipControlMessage::const_pointer realMsg = msg->as<GameSpaceShipControlMessage>();
        onControlMessage(realMsg->get<uint16_t>("inputType"), realMsg->get<bool>("isOn"));
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
            torque += osg::Vec3f(0, 0, 0.5f* (m_inputState[LEFT] ? m_steerability : -m_steerability));
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

    msg->get<std::string>("playerName") = playerName;
    msg->get<osg::Vec3f>("pos").set(translate.x(), translate.y(), translate.z());
    msg->get<osg::Vec4f>("orient").set(rotate.x(), rotate.y(), rotate.z(), rotate.w());
    msg->get<uint16_t>("objectId") = m_myObjectId;
    msg->get<uint32_t>("ownerId") = m_myOwnerId;
    return msg;
}

void SpaceShipServer::onPhysicsUpdate(const osg::Vec3f& newPos, const osg::Quat& newRot)
{
    std::chrono::steady_clock::time_point tpNow = std::chrono::steady_clock::now();
    std::chrono::steady_clock::duration dt = tpNow - m_lastUpdate;
    float dtMs = std::chrono::duration_cast<std::chrono::milliseconds>(dt).count();
    if (dtMs < 1. / 30.)
        return;

    m_lastUpdate = tpNow;

    GameSpaceShipPhysicsUpdateMessage::pointer msg{ new GameSpaceShipPhysicsUpdateMessage };
    msg->get<osg::Vec3f>("pos") = newPos;
    msg->get<osg::Vec4f>("orient") = newRot.asVec4();
    msg->get<osg::Vec3f>("velocity").set(0, 0, 0);
    msg->objectId(m_myObjectId);
    messageToPartner(msg);
}

unsigned int SpaceShipServer::getPhysicsId()
{
    return m_physicsId;
}


SpaceShipServer::~SpaceShipServer()
{
    m_engine->removeVehicle(m_physicsId);
}

void SpaceShipServer::incrementPlayerScore()
{   
    std::cout << "incrementing score for ship " << this->getObjectId() << "score = " << m_playerScore << "\n";
    m_playerScore++;

    GameScoreUpdateMessage::pointer msg{ new GameScoreUpdateMessage };
    msg->get<uint32_t>("score") = m_playerScore;
    msg->objectId(m_myObjectId);
    messageToPartner(msg);
}

int SpaceShipServer::getPlayerScore()
{
    return m_playerScore;
}

REGISTER_GAMEMESSAGE(SpaceShipConstructionData)
REGISTER_GAMEMESSAGE(SpaceShipControl)
REGISTER_GAMEMESSAGE(SpaceShipPhysicsUpdate)
REGISTER_GAMEMESSAGE(SpaceShipCollision)

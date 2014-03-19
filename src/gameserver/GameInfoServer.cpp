#include "GameInstanceServer.h"
#include "GameInfoServer.h"

template <> MessageMetaData
GameGameInfoConstructionDataMessage::base::m_metaData = MessageMetaData::createMetaData<GameGameInfoConstructionDataMessage>("ownerId");

template <> MessageMetaData
GameRoundDataMessage::base::m_metaData = MessageMetaData::createMetaData<GameRoundDataMessage>("finishAreaCenter\nfinishAreaRadius\nownerId");

REGISTER_GAMEMESSAGE(GameInfoConstructionData)
REGISTER_GAMEMESSAGE(RoundData)

GameInfoServer::GameInfoServer(uint32_t ownerId, GameInstanceServer* context) :
GameObject(ownerId, context)
{
}

bool GameInfoServer::takeMessage(const GameMessage::const_pointer& msg, MessagePeer* sender)
{
    std::cerr << "GameInfoServer recieved a message. There shouldn't be messages sent to GameInfoServer";
    return false;
}

void GameInfoServer::setObjective(osg::Vec3f finish, float finishRadius)
{
    m_finishArea = finish;
    m_finishAreaSize = finishRadius;
}

bool GameInfoServer::shipInFinishArea(osg::Vec3f pos)
{
    return (pos - m_finishArea).length() < m_finishAreaSize;
}


GameMessage::pointer GameInfoServer::objectiveMessage() const
{
    //build and send message to client
    GameRoundDataMessage::pointer msg{ new GameRoundDataMessage };
    msg->objectId(m_myObjectId);
    msg->get<osg::Vec3f>("finishAreaCenter") = m_finishArea;
    msg->get<float>("finishAreaRadius") = m_finishAreaSize;
    msg->get<uint32_t>("ownerId") = m_myOwnerId;
    return msg;
}

GameMessage::pointer GameInfoServer::creationMessage() const
{
    GameGameInfoConstructionDataMessage::pointer msg{ new GameGameInfoConstructionDataMessage };
    msg->objectId(m_myObjectId);
    msg->get<uint32_t>("ownerId") = m_myOwnerId;
    return msg;
}

GameInfoServer::~GameInfoServer()
{
}


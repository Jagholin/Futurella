#include "GameInstanceServer.h"
#include "GameInfoServer.h"

BEGIN_GAMETORAWMESSAGE_QCONVERT(GameInfoConstructionData)
out << ownerId;
END_GAMETORAWMESSAGE_QCONVERT()
BEGIN_RAWTOGAMEMESSAGE_QCONVERT(GameInfoConstructionData)
in >> ownerId;
END_RAWTOGAMEMESSAGE_QCONVERT()

BEGIN_RAWTOGAMEMESSAGE_QCONVERT(RoundData)
in >> ownerId >> startPoint >> finishAreaCenter >> finishAreaRadius;
END_RAWTOGAMEMESSAGE_QCONVERT()
BEGIN_GAMETORAWMESSAGE_QCONVERT(RoundData)
out << ownerId << startPoint << finishAreaCenter << finishAreaRadius;
END_GAMETORAWMESSAGE_QCONVERT()

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

void GameInfoServer::setObjective(osg::Vec3f start, osg::Vec3f finish, float finishRadius)
{
    m_startingPoint = start;
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
    msg->startPoint = m_startingPoint;
    msg->finishAreaCenter = m_finishArea;
    msg->finishAreaRadius = m_finishAreaSize;
    msg->objectId = m_myObjectId;
    msg->ownerId = m_myOwnerId;
    return msg;
}

GameMessage::pointer GameInfoServer::creationMessage() const
{
    GameGameInfoConstructionDataMessage::pointer msg{ new GameGameInfoConstructionDataMessage };
    msg->objectId = m_myObjectId;
    msg->ownerId = m_myOwnerId;
    return msg;
}

GameInfoServer::~GameInfoServer()
{
}


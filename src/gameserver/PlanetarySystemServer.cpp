#include "GameInstanceServer.h"
#include "PlanetarySystemServer.h"

template<> MessageMetaData
GameCreatePlanetarySystemMessage::m_metaData = MessageMetaData::createMetaData<GameCreatePlanetarySystemMessage>("ownerId");

REGISTER_GAMEMESSAGE(CreatePlanetarySystem)

PlanetarySystemServer::PlanetarySystemServer(uint32_t ownerId, GameMessagePeer* context) :
GameObject(ownerId, context)
{

}

PlanetarySystemServer::~PlanetarySystemServer()
{
    // TODO?
}

bool PlanetarySystemServer::takeMessage(const GameMessage::const_pointer& msg, MessagePeer* sender)
{
    // Respond to messages that are sent to you.

    return true;
}

GameMessage::pointer PlanetarySystemServer::creationMessage() const
{
    GameCreatePlanetarySystemMessage::pointer msg{ new GameCreatePlanetarySystemMessage };
    msg->get<uint32_t>("ownerId") = m_myOwnerId;
    msg->objectId(getObjectId());
    return msg;
}

#include "../gameserver/PlanetarySystemServer.h"
#include "PlanetarySystemClient.h"

REGISTER_GAMEOBJECT_TYPE(PlanetarySystemClient, 3300)

PlanetarySystemClient::PlanetarySystemClient(uint16_t objId, uint32_t ownerId, GameInstanceClient* context):
GameObject(objId, ownerId, context)
{
    std::cout << "planetary system object with id = " << objId << " owner " << ownerId << " successfully created\n";
}

PlanetarySystemClient::~PlanetarySystemClient()
{

}

PlanetarySystemClient::pointer PlanetarySystemClient::createFromGameMessage(const GameMessage::const_pointer& msg, GameMessagePeer* ctx)
{
    auto realMsg = msg->as<GameCreatePlanetarySystemMessage>();
    PlanetarySystemClient::pointer result{ new PlanetarySystemClient(realMsg->objectId(), realMsg->get<uint32_t>("ownerId"), static_cast<GameInstanceClient*>(ctx)) };
    return result;
}

bool PlanetarySystemClient::takeMessage(const GameMessage::const_pointer& msg, MessagePeer* sender)
{
    std::cout << "Unexpected message for planetary system client" << std::endl;
    return true;
}

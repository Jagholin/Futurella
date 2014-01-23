#include "GameInstanceClient.h"
#include "../gamecommon/GameObject.h"

GameInstanceClient::GameInstanceClient():
m_connected(false),
m_orphaned(false)
{
    // nop
}

void GameInstanceClient::connectLocallyTo(MessagePeer* buddy, bool recursive /*= true*/)
{
    // Only one single connection to the server makes sense.
    if (m_connected)
        throw std::runtime_error("GameInstanceClient::connectLocallyTo: A GameInstanceServer cannot be connected to more than 1 server object.");
    GameMessagePeer::connectLocallyTo(buddy, recursive);

    m_connected = true;
    // Do nothing?
}

void GameInstanceClient::disconnectLocallyFrom(MessagePeer* buddy, bool recursive /*= true*/)
{
    GameMessagePeer::disconnectLocallyFrom(buddy, recursive);

    assert(m_connected);
    m_connected = false;
    m_orphaned = true;
    m_clientOrphaned();
}

bool GameInstanceClient::unknownObjectIdMessage(const GameMessage::const_pointer& msg, MessagePeer* sender)
{
    // perhaps this is a new game object, try to create one.
    try {
        GameObjectFactory::createFromGameMessage(msg, this);
    }
    catch (std::runtime_error)
    {
        // Do nothing, just ignore the message.
    }
    return true;
}

#include "GameInstanceServer.h"

GameInstanceServer::GameInstanceServer(const std::string &name) :
m_name(name)
{
    // nop
}

std::string GameInstanceServer::name() const
{
    return m_name;
}

bool GameInstanceServer::unknownObjectIdMessage(const GameMessage::const_pointer& msg, MessagePeer* sender)
{
    // The creation of objects on the client side is not allowed.

    // TODO: terminate the connection
    return false;
}

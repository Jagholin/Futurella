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

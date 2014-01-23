#pragma once

#include "../gamecommon/GameMessagePeer.h"

class GameInstanceServer : public GameMessagePeer
{
public:
    GameInstanceServer(const std::string &name);

    std::string name() const;
protected:
    std::string m_name;
};

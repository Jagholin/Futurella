#pragma once

#include "../gamecommon/GameMessagePeer.h"

class GameInstanceServer : public GameMessagePeer
{
public:
    GameInstanceServer(const std::string &name);

    std::string name() const;

    virtual bool unknownObjectIdMessage(const GameMessage::const_pointer& msg, MessagePeer* sender);

protected:
    std::string m_name;
};

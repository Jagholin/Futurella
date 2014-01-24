#pragma once

#include "../gamecommon/GameMessagePeer.h"
#include "SpaceShipServer.h"

class GameInstanceServer : public GameMessagePeer
{
public:
    GameInstanceServer(const std::string &name);

    std::string name() const;

    virtual bool unknownObjectIdMessage(const GameMessage::const_pointer& msg, MessagePeer* sender);

    virtual void connectLocallyTo(MessagePeer* buddy, bool recursive = true);

    virtual void disconnectLocallyFrom(MessagePeer* buddy, bool recursive = true);

    void physicsTick(float timeInterval);

protected:
    std::map<MessagePeer*, SpaceShipServer::pointer> m_peerSpaceShips;

    std::string m_name;
};

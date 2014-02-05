#pragma once

#include "../gamecommon/GameMessagePeer.h"
#include "SpaceShipServer.h"
#include "AsteroidFieldServer.h"
#include "GameInfoServer.h"

class PhysicsEngine;

class GameInstanceServer : public GameMessagePeer
{
public:
    GameInstanceServer(const std::string &name);
    virtual ~GameInstanceServer();

    std::string name() const;

    virtual bool unknownObjectIdMessage(const GameMessage::const_pointer& msg, MessagePeer* sender);

    virtual void connectLocallyTo(MessagePeer* buddy, bool recursive = true);

    virtual void disconnectLocallyFrom(MessagePeer* buddy, bool recursive = true);

    void physicsTick(float timeInterval);

    void newRound();
    void checkForEndround();

protected:
    
    std::shared_ptr<PhysicsEngine> m_physicsEngine;

    std::map<MessagePeer*, SpaceShipServer::pointer> m_peerSpaceShips;
    AsteroidFieldServer::pointer m_asteroidField;
    GameInfoServer::pointer m_gameInfo;

    std::string m_name;

    bool m_waitingForPlayers;
};

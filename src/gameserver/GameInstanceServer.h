#pragma once

#include "../gamecommon/GameMessagePeer.h"
#include "SpaceShipServer.h"
#include "AsteroidFieldServer.h"

class btDynamicsWorld;
class btDefaultCollisionConfiguration;
class btCollisionDispatcher;
class btBroadphaseInterface;
class btSequentialImpulseConstraintSolver;

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

protected:
    std::map<MessagePeer*, SpaceShipServer::pointer> m_peerSpaceShips;
    AsteroidFieldServer::pointer m_asteroidField;

    // Physics main world
    btDynamicsWorld* m_physicsWorld;

    btDefaultCollisionConfiguration* m_collisionConfig;
    btCollisionDispatcher* m_collisionDispatcher;
    btBroadphaseInterface* m_broadphase;
    btSequentialImpulseConstraintSolver* m_constraintSolver;

    std::string m_name;
};

#pragma once

#include "../gamecommon/GameMessagePeer.h"
#include "SpaceShipServer.h"
#include "AsteroidFieldChunkServer.h"
#include <osg/Vec3i>
#include "GameInfoServer.h"

BEGIN_DECLNETMESSAGE(RequestChunkData, 1777, false)
osg::Vec3i coord;
END_DECLNETMESSAGE()

BEGIN_DECLNETMESSAGE(StopChunkTracking, 1778, false)
osg::Vec3i coord;
END_DECLNETMESSAGE()

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

    typedef osg::Vec3i ChunkCoordinates;
    static ChunkCoordinates positionToChunk(const osg::Vec3f& pos);
    static osg::Vec3f chunkToPosition(const osg::Vec3i& pos);

    virtual bool takeMessage(const NetMessage::const_pointer&, MessagePeer*);

    struct ServerChunkData {
        AsteroidFieldChunkServer::pointer m_asteroidField;
        std::deque<MessagePeer*> m_observers;
    };
protected:
    
    std::shared_ptr<PhysicsEngine> m_physicsEngine;

    std::map<MessagePeer*, SpaceShipServer::pointer> m_peerSpaceShips;
    //AsteroidFieldChunkServer::pointer m_asteroidField;
    GameInfoServer::pointer m_gameInfo;

    std::map<ChunkCoordinates, ServerChunkData> m_universe;
    std::string m_name;

    bool m_waitingForPlayers;
};

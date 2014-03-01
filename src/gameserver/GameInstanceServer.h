#pragma once

#include "../gamecommon/GameMessagePeer.h"
#include "SpaceShipServer.h"
#include "AsteroidFieldChunkServer.h"
#include "GameInfoServer.h"

#include <boost/asio/io_service.hpp>
#include <osg/Vec3i>
#include <OpenThreads/Thread>
#include <chrono>

typedef GenericNetMessage<1777, osg::Vec3i> NetRequestChunkDataMessage; // varNames: "coordinate"
typedef GenericNetMessage<1778, osg::Vec3i> NetStopChunkTrackingMessage; // varNames: "coordinate"
typedef GenericNetMessage<1779, std::string, uint16_t> NetPlayerScoreInfoMessage; // varNames: "playerName", "score"

class PhysicsEngine;
class GameInstanceServer;

class GameServerThread : public OpenThreads::Thread
{
public:
    GameServerThread(GameInstanceServer* srv);

    virtual void run() override;
    void stopThread();
protected:
    GameInstanceServer *m_serverObject;
    // TODO: use atomic_bool here
    volatile bool m_continueRun;
    std::chrono::steady_clock::time_point m_previousTime;
};

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

    virtual boost::asio::io_service* eventService();

    struct ServerChunkData {
        AsteroidFieldChunkServer::pointer m_asteroidField;
        std::deque<MessagePeer*> m_observers;
    };

    struct ScoreData {
        std::string m_playerName;
        unsigned int m_score;
    };

    friend class GameServerThread;
protected:
    
    std::shared_ptr<PhysicsEngine> m_physicsEngine;

    std::map<MessagePeer*, SpaceShipServer::pointer> m_peerSpaceShips;
    std::map<MessagePeer*, ScoreData> m_scores;
    //AsteroidFieldChunkServer::pointer m_asteroidField;
    GameInfoServer::pointer m_gameInfo;

    std::map<ChunkCoordinates, ServerChunkData> m_universe;
    std::string m_name;

    boost::asio::io_service m_eventService;

    GameServerThread m_serverThread;

    bool m_waitingForPlayers;
};

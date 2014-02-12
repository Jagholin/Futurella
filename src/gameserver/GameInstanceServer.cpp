#include "GameInstanceServer.h"
#include "../networking/peermanager.h"
#include "../gamecommon/PhysicsEngine.h"

#include <random>

#define MINPLAYERS 1

BEGIN_NETTORAWMESSAGE_QCONVERT(RequestChunkData)
out << coord;
END_NETTORAWMESSAGE_QCONVERT()
BEGIN_RAWTONETMESSAGE_QCONVERT(RequestChunkData)
in >> coord;
END_RAWTONETMESSAGE_QCONVERT()
REGISTER_NETMESSAGE(RequestChunkData)

BEGIN_NETTORAWMESSAGE_QCONVERT(StopChunkTracking)
out << coord;
END_NETTORAWMESSAGE_QCONVERT()
BEGIN_RAWTONETMESSAGE_QCONVERT(StopChunkTracking)
in >> coord;
END_RAWTONETMESSAGE_QCONVERT();
REGISTER_NETMESSAGE(StopChunkTracking)

BEGIN_NETTORAWMESSAGE_QCONVERT(PlayerScoreInfo)
out << playerName << score;
END_NETTORAWMESSAGE_QCONVERT()
BEGIN_RAWTONETMESSAGE_QCONVERT(PlayerScoreInfo)
in >> playerName >> score;
END_RAWTONETMESSAGE_QCONVERT();
REGISTER_NETMESSAGE(PlayerScoreInfo)

GameInstanceServer::GameInstanceServer(const std::string &name) :
//m_physicsEngine(new PhysicsEngine),
m_name(name),
m_serverThread(this)
{
    m_eventService.dispatch([this](){
        m_physicsEngine = std::make_shared<PhysicsEngine>();
    });
    m_gameInfo = std::make_shared<GameInfoServer>(0, this);
    m_waitingForPlayers = true;
    m_serverThread.setSchedulePriority(OpenThreads::Thread::THREAD_PRIORITY_HIGH);
    m_serverThread.start();
}

GameInstanceServer::~GameInstanceServer()
{
    m_serverThread.stopThread();
    m_serverThread.join();
}

std::string GameInstanceServer::name() const
{
    return m_name;
}

bool GameInstanceServer::unknownObjectIdMessage(const GameMessage::const_pointer& msg, MessagePeer* sender)
{
    // The creation of objects on the client side is not allowed.

    std::cerr << "WTF? GameInstanceServer just received a message from an object with non-existing objectId[" << msg->objectId << "]\n";
    return false;
}

void GameInstanceServer::connectLocallyTo(MessagePeer* buddy, bool recursive /*= true*/)
{
    // we have got a new client! Create a SpaceShip just for him
    GameMessagePeer::connectLocallyTo(buddy, recursive);

    // Dispatch to server thread, if necessary(see docs on difference between 
    // io_service::dispatch() and io_service::post()
    m_eventService.dispatch([this, buddy](){

        SpaceShipServer::pointer hisShip{
            new SpaceShipServer(osg::Vec3f(), osg::Quat(0, osg::Vec3f(1, 0, 0)), RemotePeersManager::getManager()->getPeersId(buddy), this, m_physicsEngine) };

        //GameMessage::pointer constructItMsg = m_asteroidField->creationMessage();
        //buddy->send(constructItMsg);
        GameMessage::pointer constructItMsg;
        for (std::pair<MessagePeer*, SpaceShipServer::pointer> aShip : m_peerSpaceShips)
        {
            constructItMsg = aShip.second->creationMessage();
            buddy->send(constructItMsg);
        }
        broadcastLocally(hisShip->creationMessage());
        m_peerSpaceShips.insert(std::make_pair(buddy, hisShip));

        GameMessage::pointer constructGameInfoMsg = m_gameInfo->creationMessage();
        buddy->send(constructGameInfoMsg);


        if (m_peerSpaceShips.size() >= MINPLAYERS && m_waitingForPlayers)
        {
            m_waitingForPlayers = false;
            newRound();
        }

    });
}

void GameInstanceServer::newRound()
{
    // dispatch to the server thread if necessary
    m_eventService.dispatch([this]() {

        std::random_device randDevice;
        //float range = m_asteroidField->getCubeSideLength();
        float range = 16.0; // todo: new range calculation?
        float maxRand = randDevice.max();

        osg::Vec3f startingPoint = osg::Vec3f(
            range*randDevice() / maxRand,
            range*randDevice() / maxRand,
            range*randDevice() / maxRand);
        osg::Vec3f finishArea = osg::Vec3f(
            range*randDevice() / maxRand,
            range*randDevice() / maxRand,
            range*randDevice() / maxRand);
        m_gameInfo->setObjective(startingPoint, finishArea, 1);
    
    
        GameMessage::pointer msg = m_gameInfo->objectiveMessage();
        btTransform t;
        for (std::map<MessagePeer*, SpaceShipServer::pointer>::iterator peerSpaceShip = m_peerSpaceShips.begin(); peerSpaceShip != m_peerSpaceShips.end(); ++peerSpaceShip)
        {
            //set ship positions to starting point
            osg::Vec3f v1 = osg::Vec3f(0, 0, 1), v2 = startingPoint - finishArea;
            osg::Vec3f a = v1 ^ v2;
            float w = sqrt(v1.length2() * v2.length2()) + v1*v2;
            btQuaternion q = btQuaternion(a.x(), a.y(), a.z(), w).normalize();
            t.setRotation(q);
            t.setOrigin(btVector3(startingPoint.x(), startingPoint.y(), startingPoint.z()));

            unsigned int physId = peerSpaceShip->second->getPhysicsId();
        
            m_physicsEngine->setShipTransformation(physId, t);

            //inform clients about new objective
            MessagePeer* buddy = peerSpaceShip->first;
            buddy->send(msg);

        }
    });
}

void GameInstanceServer::disconnectLocallyFrom(MessagePeer* buddy, bool recursive /*= true*/)
{
    GameMessagePeer::disconnectLocallyFrom(buddy, recursive);

    // Dispatch to the server thread
    m_eventService.dispatch([this, buddy](){

        SpaceShipServer::pointer shipToRemove = m_peerSpaceShips.at(buddy);
        NetRemoveGameObjectMessage::pointer removeMessage{ new NetRemoveGameObjectMessage };
        removeMessage->objectId = shipToRemove->getObjectId();
        broadcastLocally(removeMessage);

        m_peerSpaceShips.erase(buddy);

        if (m_peerSpaceShips.size() < MINPLAYERS)
        {
            m_waitingForPlayers = true;
        }

        std::vector <ChunkCoordinates> chunksToRemove;
        for (auto& chunk : m_universe)
        {
            std::deque<MessagePeer*> &observers = chunk.second.m_observers;
            auto endIter = observers.end();
            auto peerData = std::find(observers.begin(), endIter, buddy);
            if (peerData != endIter)
            {
                observers.erase(peerData);
                if (observers.empty())
                    chunksToRemove.push_back(chunk.first);
            }
        }
        for (ChunkCoordinates const& coord : chunksToRemove)
            m_universe.erase(coord);
    });
}

void GameInstanceServer::physicsTick(float timeInterval)
{
    m_physicsEngine->physicsTick(timeInterval);
    checkForEndround();
}

void GameInstanceServer::checkForEndround()
{
    // Dispatch to the server thread
    m_eventService.dispatch([this]() {

        bool someShipEnteredFinishArea = false;
        for (std::map<MessagePeer*, SpaceShipServer::pointer>::iterator peerSpaceShip = m_peerSpaceShips.begin(); peerSpaceShip != m_peerSpaceShips.end(); ++peerSpaceShip)
        {
            unsigned int physId = peerSpaceShip->second->getPhysicsId();
            osg::Vec3f pos = m_physicsEngine->getShipPosition(physId);
            if (m_gameInfo->shipInFinishArea(pos))
            {
                someShipEnteredFinishArea = true;
                break;
            }
        }
        if (someShipEnteredFinishArea)
        {
            //todo. update the players scores
            newRound();
        }
    });
}

GameInstanceServer::ChunkCoordinates GameInstanceServer::positionToChunk(const osg::Vec3f& pos)
{
    return ChunkCoordinates(std::floor(pos.x() / 16.0),
        std::floor(pos.y() / 16.0),
        std::floor(pos.z() / 16.0));
}

bool GameInstanceServer::takeMessage(const NetMessage::const_pointer& msg, MessagePeer* peer)
{
    // You have to be executed from within this thread.
    assert(OpenThreads::Thread::CurrentThread() == &m_serverThread);

    if (msg->gettype() == NetRequestChunkDataMessage::type)
    {
        NetRequestChunkDataMessage::const_pointer realMsg = msg->as<NetRequestChunkDataMessage>();
        osg::Vec3i chunkCoord = realMsg->coord;
        // A client has requested data about the chunk.
        // First look if we have it in our dictionary
        if (m_universe.count(chunkCoord) == 0)
        {
            // The chunk doesn't exist yet, generate it
            ServerChunkData newChunk;
            newChunk.m_asteroidField = std::make_shared<AsteroidFieldChunkServer>(100, 16, 0, chunkCoord, this, m_physicsEngine.get());
            m_universe.insert(std::make_pair(chunkCoord, newChunk));
        }

        ServerChunkData& myChunk = m_universe[chunkCoord];
        NetMessage::pointer reply = myChunk.m_asteroidField->creationMessage();
        myChunk.m_observers.push_back(peer);

        peer->send(reply, this);
        return true;
    }
    else if (msg->gettype() == NetStopChunkTrackingMessage::type)
    {
        NetRequestChunkDataMessage::const_pointer realMsg = msg->as<NetRequestChunkDataMessage>();
        osg::Vec3i chunkCoord = realMsg->coord;

        if (m_universe.count(chunkCoord) == 0)
            return false;
        ServerChunkData& myChunk = m_universe[chunkCoord];
        
        auto erasePosition = std::find(myChunk.m_observers.cbegin(), myChunk.m_observers.cend(), peer);
        if (erasePosition == myChunk.m_observers.cend())
            return false;

        myChunk.m_observers.erase(erasePosition);
        if (myChunk.m_observers.empty())
        {
            m_universe.erase(chunkCoord);
        }
        return true;
    }
    return GameMessagePeer::takeMessage(msg, peer);
}

osg::Vec3f GameInstanceServer::chunkToPosition(const osg::Vec3i& pos)
{
    return osg::Vec3f(pos.x() * 16.0, pos.y()*16.0, pos.z()*16.0);
}

boost::asio::io_service* GameInstanceServer::eventService()
{
    return &m_eventService;
}

GameServerThread::GameServerThread(GameInstanceServer* srv)
{
    m_serverObject = srv;
    m_continueRun = true;
    m_previousTime = std::chrono::steady_clock::now();
}

void GameServerThread::run()
{
    // VS steady_clock has about 1 ms precision, which is good enough here
    m_previousTime = std::chrono::steady_clock::now();
    while (m_continueRun)
    {
        std::chrono::steady_clock::time_point startTick = std::chrono::steady_clock::now();

        // Don't run more work items than you can do per tick
        // 8 ms is a soft boundary here
        std::chrono::steady_clock::time_point timepoint;
        // TODO: implement a priority queue
        while (m_serverObject->m_eventService.poll_one() > 0)
        {
            timepoint = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::microseconds>(timepoint - startTick).count() >= 8000)
                break;
        }
        m_serverObject->m_eventService.reset();

        timepoint = std::chrono::steady_clock::now();
        float dt = std::chrono::duration_cast<std::chrono::microseconds>(timepoint - m_previousTime).count() / 1000.0f;
        m_serverObject->physicsTick(dt);
        m_previousTime = timepoint;

        timepoint = std::chrono::steady_clock::now();
        dt = std::chrono::duration_cast<std::chrono::microseconds>(timepoint - startTick).count() / 1000.0f;

        if (dt >= 15.999f)
            std::cout << dt << "\n";
        else
            microSleep((16.0f - dt) * 1000.0f);
    }
}

void GameServerThread::stopThread()
{
    m_continueRun = false;
}

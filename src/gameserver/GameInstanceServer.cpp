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

GameInstanceServer::GameInstanceServer(const std::string &name) :
m_physicsEngine(new PhysicsEngine),
m_name(name)
{
    // nop
    m_gameInfo = std::make_shared<GameInfoServer>(0, this);
    m_waitingForPlayers = true;
}

GameInstanceServer::~GameInstanceServer()
{
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
}

void GameInstanceServer::newRound()
{
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
}

void GameInstanceServer::disconnectLocallyFrom(MessagePeer* buddy, bool recursive /*= true*/)
{
    
    GameMessagePeer::disconnectLocallyFrom(buddy, recursive);

    SpaceShipServer::pointer shipToRemove = m_peerSpaceShips.at(buddy);
    NetRemoveGameObjectMessage::pointer removeMessage{ new NetRemoveGameObjectMessage };
    removeMessage->objectId = shipToRemove->getObjectId();
    broadcastLocally(removeMessage);

    m_peerSpaceShips.erase(buddy);

    if (m_peerSpaceShips.size() < MINPLAYERS)
    {
        m_waitingForPlayers = true;
    }
}

void GameInstanceServer::physicsTick(float timeInterval)
{
    m_physicsEngine->physicsTick(timeInterval);
    checkForEndround();
}

void GameInstanceServer::checkForEndround()
{
    bool someShipEnteredFinishArea = false;
    for (std::map<MessagePeer*, SpaceShipServer::pointer>::iterator peerSpaceShip = m_peerSpaceShips.begin(); peerSpaceShip != m_peerSpaceShips.end(); ++peerSpaceShip)
    {
        unsigned int physId = peerSpaceShip->second->getPhysicsId();
        osg::Vec3f pos = m_physicsEngine->getShipPosition(physId);
        if ( m_gameInfo->shipInFinishArea(pos) )
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
}

GameInstanceServer::ChunkCoordinates GameInstanceServer::positionToChunk(const osg::Vec3f& pos)
{
    return ChunkCoordinates(std::floor(pos.x() / 16.0),
        std::floor(pos.y() / 16.0),
        std::floor(pos.z() / 16.0));
}

bool GameInstanceServer::takeMessage(const NetMessage::const_pointer& msg, MessagePeer* peer)
{
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
            newChunk.m_asteroidField = std::make_shared<AsteroidFieldChunkServer>(200, 16, 0, chunkCoord, this, m_physicsEngine.get());
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


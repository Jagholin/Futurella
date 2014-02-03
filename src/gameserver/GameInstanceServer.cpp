#include "GameInstanceServer.h"
#include "../networking/peermanager.h"

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
m_name(name)//,
//m_asteroidField(new AsteroidFieldChunkServer(200, 16, 0, this))
{
    // nop
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

    SpaceShipServer::pointer hisShip{ new SpaceShipServer(osg::Vec3f(), osg::Quat(0, osg::Vec3f(1, 0, 0)), RemotePeersManager::getManager()->getPeersId(buddy), this) };

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
}

void GameInstanceServer::disconnectLocallyFrom(MessagePeer* buddy, bool recursive /*= true*/)
{
    GameMessagePeer::disconnectLocallyFrom(buddy, recursive);

    SpaceShipServer::pointer shipToRemove = m_peerSpaceShips.at(buddy);
    NetRemoveGameObjectMessage::pointer removeMessage{ new NetRemoveGameObjectMessage };
    removeMessage->objectId = shipToRemove->getObjectId();
    broadcastLocally(removeMessage);

    m_peerSpaceShips.erase(buddy);
}

void GameInstanceServer::physicsTick(float timeInterval)
{
    for (auto aShip : m_peerSpaceShips)
    {
        aShip.second->timeTick(timeInterval);
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
            newChunk.m_asteroidField = std::make_shared<AsteroidFieldChunkServer>(200, 16, 0, chunkCoord, this);
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


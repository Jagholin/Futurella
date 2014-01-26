#include "GameInstanceServer.h"
#include "../networking/peermanager.h"

GameInstanceServer::GameInstanceServer(const std::string &name) :
m_name(name),
m_asteroidField(new AsteroidFieldServer(50, 0.5f, 1, 0, this))
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

    GameMessage::pointer constructItMsg = m_asteroidField->creationMessage();
    buddy->send(constructItMsg);
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

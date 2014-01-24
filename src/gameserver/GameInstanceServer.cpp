#include "GameInstanceServer.h"
#include "../networking/peermanager.h"

GameInstanceServer::GameInstanceServer(const std::string &name) :
m_name(name)
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

    // TODO: terminate the connection
    return false;
}

void GameInstanceServer::connectLocallyTo(MessagePeer* buddy, bool recursive /*= true*/)
{
    // we have got a new client! Create a SpaceShip just for him
    GameMessagePeer::connectLocallyTo(buddy, recursive);

    SpaceShipServer::pointer hisShip{ new SpaceShipServer(osg::Vec3f(), osg::Vec4f(), RemotePeersManager::getManager()->getPeersId(buddy), this) };
    m_peerSpaceShips.insert(std::make_pair(buddy, hisShip));

    for (std::pair<MessagePeer*, SpaceShipServer::pointer> aShip : m_peerSpaceShips)
    {
        GameMessage::pointer constructItMsg = aShip.second->creationMessage();
        buddy->send(constructItMsg);
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
}

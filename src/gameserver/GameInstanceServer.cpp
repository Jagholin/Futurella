#include "GameInstanceServer.h"
#include "../networking/peermanager.h"
#include "../gamecommon/PhysicsEngine.h"


#include <random>

#define MINPLAYERS 1


GameInstanceServer::GameInstanceServer(const std::string &name) :
m_physicsEngine(new PhysicsEngine),
m_name(name)
{
    m_asteroidField = std::make_shared<AsteroidFieldServer>(200, 0.5f, 1, 0, this, m_physicsEngine.get());
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

    GameMessage::pointer constructItMsg = m_asteroidField->creationMessage();
    buddy->send(constructItMsg);
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
    float range = m_asteroidField->getCubeSideLength();
    float maxRand = randDevice.max();
    osg::Vec3f startingPoint = osg::Vec3f(
        range*randDevice() / maxRand,
        range*randDevice() / maxRand,
        range*randDevice() / maxRand);
    osg::Vec3f finishArea = osg::Vec3f(
        range*randDevice() / maxRand,
        range*randDevice() / maxRand,
        range*randDevice() / maxRand);
    m_gameInfo->setObjective(startingPoint, finishArea, 2);
    GameMessage::pointer msg = m_gameInfo->objectiveMessage();
    for (std::map<MessagePeer*, SpaceShipServer::pointer>::iterator peerSpaceShip = m_peerSpaceShips.begin(); peerSpaceShip != m_peerSpaceShips.end(); ++peerSpaceShip)
    {
        //set ship positions to starting point
        unsigned int physId = peerSpaceShip->second->getPhysicsId();
        m_physicsEngine->setShipPosition(physId, startingPoint);

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
    static int debugcount= 0;
    if (++debugcount > 200)
    {
     //   newRound();
        debugcount = 0;
    }
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
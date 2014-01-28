#include "GameInstanceClient.h"
#include "../gamecommon/GameObject.h"
#include "../networking/peermanager.h"

#include <osgGA/TrackballManipulator>

GameInstanceClient::GameInstanceClient(osg::Group* rootGroup, osgViewer::Viewer* viewer):
m_rootGraphicsGroup(rootGroup),
m_viewer(viewer),
m_connected(false),
m_orphaned(false)
{
    // nop
    m_viewportSizeUniform = new osg::Uniform("viewportSize", osg::Vec2f(800, 600));
    osg::Viewport* myViewport = m_viewer->getCamera()->getViewport();
    m_viewportSizeUniform->set(osg::Vec2f(myViewport->width(), myViewport->height()));
    m_rootGraphicsGroup->getOrCreateStateSet()->addUniform(m_viewportSizeUniform);

}

GameInstanceClient::~GameInstanceClient()
{
    m_viewer->setCameraManipulator(new osgGA::TrackballManipulator);
}

void GameInstanceClient::connectLocallyTo(MessagePeer* buddy, bool recursive /*= true*/)
{
    // Only one single connection to the server makes sense.
    if (m_connected)
        throw std::runtime_error("GameInstanceClient::connectLocallyTo: A GameInstanceServer cannot be connected to more than 1 server object.");
    GameMessagePeer::connectLocallyTo(buddy, recursive);

    m_connected = true;
    // Do nothing?
}

void GameInstanceClient::disconnectLocallyFrom(MessagePeer* buddy, bool recursive /*= true*/)
{
    GameMessagePeer::disconnectLocallyFrom(buddy, recursive);

    assert(m_connected);
    m_connected = false;
    m_orphaned = true;
    m_clientOrphaned();
}

bool GameInstanceClient::unknownObjectIdMessage(const GameMessage::const_pointer& msg, MessagePeer* sender)
{
    // perhaps this is a new game object, try to create one.
    try {
        GameObjectFactory::createFromGameMessage(msg, this);
    }
    catch (std::runtime_error)
    {
        // Do nothing, just ignore the message.
    }
    return true;
}

void GameInstanceClient::addExternalSpaceShip(SpaceShipClient::pointer ship)
{
    if (ship->getOwnerId() == RemotePeersManager::getManager()->getMyId())
    {
        m_myShip = ship;
        m_shipCamera = new ChaseCam(m_myShip.get());
        m_viewer->setCameraManipulator(m_shipCamera);
        m_viewer->getCamera()->setProjectionMatrixAsPerspective(60, m_viewer->getCamera()->getViewport()->aspectRatio(), 0.1f, 100000.0f);
    }
    else
        m_otherShips.push_back(ship);
}

bool GameInstanceClient::takeMessage(const NetMessage::const_pointer& msg, MessagePeer* sender)
{
    if (msg->gettype() == NetRemoveGameObjectMessage::type)
    {
        NetRemoveGameObjectMessage::const_pointer realMsg = msg->as<NetRemoveGameObjectMessage>();
        auto it = std::find_if(m_otherShips.cbegin(), m_otherShips.cend(), [realMsg]
            (const SpaceShipClient::pointer& aShip) -> bool {
            return aShip->getObjectId() == realMsg->objectId;
        });
        if (it != m_otherShips.cend())
        {
            m_otherShips.erase(it);
        }
        return true;
    }
    return GameMessagePeer::takeMessage(msg, sender);
}

osg::Group* GameInstanceClient::sceneGraphRoot()
{
    return m_rootGraphicsGroup;
}

void GameInstanceClient::setAsteroidField(AsteroidFieldClient::pointer asts)
{
    m_myAsteroids = asts;
}

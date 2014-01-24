#include "SpaceShipClient.h"
#include "GameInstanceClient.h"

#include <osg/ShapeDrawable>
#include <osg/Geode>
#include <osg/MatrixTransform>

REGISTER_GAMEOBJECT_TYPE(SpaceShipClient, 5002)

SpaceShipClient::SpaceShipClient(osg::Vec3f pos, osg::Vec4f orient, uint16_t objId, uint32_t ownerId, GameInstanceClient* ctx):
GameObject(objId, ownerId, ctx)
{
    m_rootGroup = ctx->sceneGraphRoot();

    // Setup osg hierarchy.
    osg::ref_ptr<osg::Shape> box = new osg::Box(osg::Vec3f(0, 0, 0), 0.1f, 0.1f, 0.3f);
    osg::ref_ptr<osg::ShapeDrawable> s = new osg::ShapeDrawable(box);
    s->setColor(osg::Vec4(1, 1, 1, 1));
    osg::ref_ptr<osg::Geode> d = new osg::Geode();
    d->addDrawable(s);

    m_shipNode = d;
    m_transformGroup = new osg::MatrixTransform;
    m_transformGroup->setMatrix(osg::Matrix::translate(0, 0, 0));
    m_transformGroup->addChild(m_shipNode);
}

SpaceShipClient::pointer SpaceShipClient::createFromGameMessage(const GameMessage::const_pointer& msg, GameMessagePeer* ctx)
{
    GameSpaceShipConstructionDataMessage::const_pointer realMsg = msg->as<GameSpaceShipConstructionDataMessage>();
    GameInstanceClient* context = static_cast<GameInstanceClient*>(ctx);
    SpaceShipClient::pointer newShip{ new SpaceShipClient(realMsg->pos, realMsg->orient, realMsg->objectId, realMsg->ownerId, context) };

    context->addExternalSpaceShip(newShip);

    return newShip;
}

bool SpaceShipClient::takeMessage(const GameMessage::const_pointer& msg, MessagePeer* sender)
{
    if (msg->gettype() == GameSpaceShipPhysicsUpdateMessage::type)
    {
        GameSpaceShipPhysicsUpdateMessage::const_pointer realMsg = msg->as<GameSpaceShipPhysicsUpdateMessage>();

        // TODO
        return true;
    }
    return false;
}

#include "SpaceShipClient.h"
#include "GameInstanceClient.h"

#include <osg/ShapeDrawable>
#include <osg/Geode>
#include <osg/MatrixTransform>

#include <chrono>

REGISTER_GAMEOBJECT_TYPE(SpaceShipClient, 5002)

// We use Update Callback to update ship's transformation group
class ShipTransformUpdate : public osg::NodeCallback
{
    SpaceShipClient* m_ship;
    std::chrono::steady_clock::time_point m_lastTick;
public:
    META_Object(futurella, ShipTransformUpdate);

    ShipTransformUpdate()
    {
        m_ship = nullptr;
    }
    ShipTransformUpdate(SpaceShipClient* obj){
        m_ship = obj;
    }

    ShipTransformUpdate(const ShipTransformUpdate& rhs, const osg::CopyOp&)
    {
        m_ship = rhs.m_ship;
    }

    void operator()(osg::Node*, osg::NodeVisitor*)
    {
        // Keep track of time
        std::chrono::steady_clock::time_point currentTime = std::chrono::steady_clock::now();
        m_ship->tick(std::chrono::duration_cast<std::chrono::seconds>(currentTime - m_lastTick).count());
    }
};

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
    setTransform(pos, orient);
    m_transformGroup->addChild(m_shipNode);

    m_rootGroup->addChild(m_transformGroup);
    m_transformGroup->setUpdateCallback(new ShipTransformUpdate(this));
}

SpaceShipClient::~SpaceShipClient()
{
    m_rootGroup->removeChild(m_transformGroup);
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
        setTransform(realMsg->pos, realMsg->orient);
        GameInstanceClient* context = static_cast<GameInstanceClient*>(m_context);
        context->shipChangedPosition(realMsg->pos, this);
        m_projVelocity = realMsg->velocity;
        return true;
    }
    return false;
}

void SpaceShipClient::setTransform(osg::Vec3f pos, osg::Vec4f orient)
{
    m_lastPosition = pos;
    m_lastOrientation = orient;
    m_transformGroup->setMatrix(osg::Matrix::translate(pos));
    osg::Matrix rotationMatrix;
    osg::Quat(orient).get(rotationMatrix);
    m_transformGroup->preMult(rotationMatrix);
}

void SpaceShipClient::tick(float deltaTime)
{
    osg::Vec3f newProjPosition = m_lastPosition + m_projVelocity*deltaTime;
    //setTransform(newProjPosition, m_lastOrientation);
}

void SpaceShipClient::sendInput(SpaceShipServer::inputType inType, bool isOn)
{
    GameSpaceShipControlMessage::pointer msg{ new GameSpaceShipControlMessage };
    msg->objectId = m_myObjectId;
    msg->inputType = inType;
    msg->isOn = isOn;
    messageToPartner(msg);
}

osg::Quat SpaceShipClient::getOrientation()
{
    return osg::Quat(m_lastOrientation);
}

osg::Vec3f SpaceShipClient::getPivotLocation()
{
    return m_lastPosition;
}

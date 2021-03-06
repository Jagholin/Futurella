#include "SpaceShipClient.h"
#include "GameInstanceClient.h"
#include "../NodeCallbackService.h"

#include <osg/ShapeDrawable>
#include <osg/Geode>
#include <osg/MatrixTransform>

#include <chrono>

REGISTER_GAMEOBJECT_TYPE(SpaceShipClient, 5002)

// We use Update Callback to update ship's transformation group
// (NOT USED)
class ShipTransformUpdate : public osg::NodeCallback
{
    SpaceShipClient* m_ship;
    std::chrono::steady_clock::time_point m_lastTick;
public:
    ShipTransformUpdate(SpaceShipClient* obj){
        m_ship = obj;
    }

    void operator()(osg::Node*, osg::NodeVisitor*)
    {
        // Keep track of time
        std::chrono::steady_clock::time_point currentTime = std::chrono::steady_clock::now();
        m_ship->tick(std::chrono::duration_cast<std::chrono::seconds>(currentTime - m_lastTick).count());
    }
};

SpaceShipClient::SpaceShipClient(std::string name, osg::Vec3f pos, osg::Vec4f orient, uint16_t objId, uint32_t ownerId, GameInstanceClient* ctx):
GameObject(objId, ownerId, ctx),
m_playerName(name),
m_score(0)
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

    //m_rootGroup->addChild(m_transformGroup);
    m_transformGroup->setUpdateCallback(new NodeCallbackService(m_shipUpdateService));
    //m_transformGroup->setUpdateCallback(new ShipTransformUpdate(this));
    m_transformGroup->setDataVariance(osg::Object::DYNAMIC);
    ctx->addNodeToScene(m_transformGroup);

    for (int i = 0; i < 6; ++i) m_inputCache[i] = false;
}

SpaceShipClient::~SpaceShipClient()
{
    static_cast<GameInstanceClient*>(m_context)->removeNodeFromScene(m_transformGroup);
}

SpaceShipClient::pointer SpaceShipClient::createFromGameMessage(const GameMessage::const_pointer& msg, GameMessagePeer* ctx)
{
    GameSpaceShipConstructionDataMessage::const_pointer realMsg = msg->as<GameSpaceShipConstructionDataMessage>();
    GameInstanceClient* context = static_cast<GameInstanceClient*>(ctx);
    SpaceShipClient::pointer newShip{ new SpaceShipClient(realMsg->get<std::string>("playerName"), realMsg->get<osg::Vec3f>("pos"), realMsg->get<osg::Vec4f>("orient"), realMsg->get<uint16_t>("objectId"), realMsg->get<uint32_t>("ownerId"), context) };

    context->addExternalSpaceShip(newShip);

    return newShip;
}

bool SpaceShipClient::takeMessage(const GameMessage::const_pointer& msg, MessagePeer* sender)
{
    if (msg->gettype() == GameSpaceShipPhysicsUpdateMessage::type)
    {
        GameSpaceShipPhysicsUpdateMessage::const_pointer realMsg = msg->as<GameSpaceShipPhysicsUpdateMessage>();

        // TODO
        setTransform(realMsg->get<osg::Vec3f>("pos"), realMsg->get<osg::Vec4f>("orient"));
        GameInstanceClient* context = static_cast<GameInstanceClient*>(m_context);
        context->shipChangedPosition(realMsg->get<osg::Vec3f>("pos"), this);
        m_projVelocity = realMsg->get<osg::Vec3f>("velocity");
        
        return true;
    }
    else if (msg->gettype() == GameScoreUpdateMessage::type)
    {
        GameScoreUpdateMessage::const_pointer realMsg = msg->as<GameScoreUpdateMessage>();
        m_score = realMsg->get<uint32_t>("score");
        std::cout << "Score received on client side: " << m_score << "\n"; // it seems like SCORE is sent but OBJECTID is understood as Store here!! i've got no clue why
    }
    return false;
}

void SpaceShipClient::setTransform(osg::Vec3f pos, osg::Vec4f orient)
{
    m_lastPosition = pos;
    m_lastOrientation = orient;
    m_shipUpdateService.dispatch([this](){
        m_transformGroup->setMatrix(osg::Matrix::translate(m_lastPosition));
        osg::Matrix rotationMatrix;
        osg::Quat(m_lastOrientation).get(rotationMatrix);
        m_transformGroup->preMult(rotationMatrix);
    });
}

void SpaceShipClient::tick(float deltaTime)
{
    osg::Vec3f newProjPosition = m_lastPosition + m_projVelocity*deltaTime;
    //setTransform(newProjPosition, m_lastOrientation);
}

void SpaceShipClient::sendInput(SpaceShipServer::inputType inType, bool isOn)
{
    if (m_inputCache[inType] == isOn)
        return;

    m_inputCache[inType] = isOn;
    GameSpaceShipControlMessage::pointer msg{ new GameSpaceShipControlMessage };
    msg->objectId(m_myObjectId);
    msg->get<uint16_t>("inputType") = inType;
    msg->get<bool>("isOn") = isOn;
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

std::string SpaceShipClient::getPlayerName()
{
    return m_playerName;
}

int SpaceShipClient::getPlayerScore()
{
    return m_score;
}
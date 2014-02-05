#include "GameInfoClient.h"
#include "GameInstanceClient.h"

REGISTER_GAMEOBJECT_TYPE(GameInfoClient, 5006)

// We use Update Callback to update ship's transformation group
/* maybe a good idea to update the finsh-area-rendering
class ShipTransformUpdate : public osg::NodeCallback
{
    GameInfoClient* m_ship;
    std::chrono::steady_clock::time_point m_lastTick;
public:
    META_Object(futurella, ShipTransformUpdate);

    ShipTransformUpdate()
    {
        m_ship = nullptr;
    }
    ShipTransformUpdate(GameInfoClient* obj){
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
};*/

GameInfoClient::GameInfoClient(uint16_t objId, uint32_t ownerId, GameInstanceClient* ctx):
GameObject(objId, ownerId, ctx)
{
    m_startingPoint = osg::Vec3f(0,0,0);
    m_finishArea = osg::Vec3f(0, 0, 0);
    m_finishAreaSize = 0;
    /*m_rootGroup = ctx->sceneGraphRoot();

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
    m_transformGroup->setUpdateCallback(new ShipTransformUpdate(this)); */
}

GameInfoClient::~GameInfoClient()
{
    /*m_rootGroup->removeChild(m_transformGroup);*/
}

bool GameInfoClient::takeMessage(const GameMessage::const_pointer& msg, MessagePeer* sender)
{
    if (msg->gettype() == GameRoundDataMessage::type)
    {
        GameRoundDataMessage::const_pointer realMsg = msg->as<GameRoundDataMessage>();

        m_startingPoint = realMsg->startPoint;
        m_finishArea = realMsg->finishAreaCenter;
        m_finishAreaSize = realMsg->finishAreaRadius;

        return true;
    }
    return false;
}


GameInfoClient::pointer GameInfoClient::createFromGameMessage(const GameMessage::const_pointer& msg, GameMessagePeer* ctx)
{
    GameGameInfoConstructionDataMessage::const_pointer realMsg = msg->as<GameGameInfoConstructionDataMessage>();
    GameInstanceClient* context = static_cast<GameInstanceClient*>(ctx);
    GameInfoClient::pointer gameInfo{ new GameInfoClient(realMsg->objectId, realMsg->ownerId, context) };

    context->setGameInfo(gameInfo);

    return gameInfo;
}
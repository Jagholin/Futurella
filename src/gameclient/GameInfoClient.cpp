#include "GameInfoClient.h"
#include "GameInstanceClient.h"

#include <osg/ShapeDrawable>
#include <osg/Geode>
#include <osg/MatrixTransform>

REGISTER_GAMEOBJECT_TYPE(GameInfoClient, 5006)


GameInfoClient::GameInfoClient(uint16_t objId, uint32_t ownerId, GameInstanceClient* ctx):
GameObject(objId, ownerId, ctx)
{
    m_startingPoint = osg::Vec3f(0,0,0);
    m_finishArea = osg::Vec3f(0, 0, 0);
    m_finishAreaSize = 0;
    
    m_rootGroup = ctx->sceneGraphRoot();

    osg::ref_ptr<osg::Shape> sphere = new osg::Sphere(osg::Vec3f(0, 0, 0), 1);
    osg::ref_ptr<osg::ShapeDrawable> s = new osg::ShapeDrawable(sphere);
    s->setColor(osg::Vec4(0, 1, 1, 0.5f));
    osg::ref_ptr<osg::Geode> d = new osg::Geode();
    d->addDrawable(s);

    m_finishAreaNode = d;
    m_transformGroup = new osg::MatrixTransform;
    m_transformGroup->addChild(m_finishAreaNode);

    m_rootGroup->addChild(m_transformGroup);
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

        osg::Matrix mat;
        mat.setTrans(m_finishArea);
        mat.preMultScale(osg::Vec3f(m_finishAreaSize, m_finishAreaSize, m_finishAreaSize));
        m_transformGroup->setMatrix(mat);

        ((GameInstanceClient*)m_context)->gameInfoUpdated();

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

osg::Vec3f GameInfoClient::finishArea() const
{
    return m_finishArea;
}

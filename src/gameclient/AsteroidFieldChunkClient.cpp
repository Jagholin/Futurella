#include "AsteroidFieldChunkClient.h"
#include "GameInstanceClient.h"
#include "LevelDrawable.h"
#include "../ShaderWrapper.h"

#include <osg/ShapeDrawable>
#include <osg/MatrixTransform>

REGISTER_GAMEOBJECT_TYPE(AsteroidFieldChunkClient, 5001);

AsteroidFieldChunkClient::AsteroidFieldChunkClient(const GameAsteroidFieldDataMessage::const_pointer& createMessage, uint16_t objId, uint32_t ownId, GameInstanceClient* ctx):
GameObject(objId, ownId, ctx)
{
    m_rootGroup = ctx->sceneGraphRoot();
    m_chunkCoord = createMessage->chunkCoord;
    m_asteroidsGroup = new osg::MatrixTransform(osg::Matrix::translate(GameInstanceServer::chunkToPosition(m_chunkCoord)));
    osg::Geode * asteroidsGeode = new osg::Geode;
    LevelDrawable* myLevel = new LevelDrawable;
    asteroidsGeode->addDrawable(myLevel);
    m_asteroidsGroup->addChild(asteroidsGeode);

    for (int i = 0; i < createMessage->position.size(); i++)
    {
        myLevel->addAsteroid(createMessage->position.at(i), createMessage->radius.at(i));
    }
    //m_rootGroup->addChild(m_asteroidsGroup);
    ctx->addNodeToScene(m_asteroidsGroup);
}

AsteroidFieldChunkClient::~AsteroidFieldChunkClient()
{
    //m_rootGroup->removeChild(m_asteroidsGroup);
    static_cast<GameInstanceClient*>(m_context)->removeNodeFromScene(m_asteroidsGroup);
}

AsteroidFieldChunkClient::pointer AsteroidFieldChunkClient::createFromGameMessage(const GameMessage::const_pointer& msg, GameMessagePeer* ctx)
{
    GameAsteroidFieldDataMessage::const_pointer realMsg = msg->as<GameAsteroidFieldDataMessage>();
    GameInstanceClient* context = static_cast<GameInstanceClient*>(ctx);
    AsteroidFieldChunkClient::pointer result{ new AsteroidFieldChunkClient(realMsg, realMsg->objectId, realMsg->ownerId, context) };

    context->addAsteroidFieldChunk(realMsg->chunkCoord, result);
    return result;
}

bool AsteroidFieldChunkClient::takeMessage(const GameMessage::const_pointer& msg, MessagePeer* sender)
{
    return false;
}

void AsteroidFieldChunkClient::setUseTesselation(bool on)
{
    unsigned int childCount = m_asteroidsGroup->getNumChildren();
    for (unsigned int i = 0; i < childCount; ++i)
    {
        osg::Geode* child = m_asteroidsGroup->getChild(i)->asGeode();
        LevelDrawable* chunk = static_cast<LevelDrawable*>(child->getDrawable(0));
        chunk->setUseTesselation(on);
    }
}

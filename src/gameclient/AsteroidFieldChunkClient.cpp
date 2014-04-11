#include "AsteroidFieldChunkClient.h"
#include "GameInstanceClient.h"
#include "LevelDrawable.h"
#include "../ShaderWrapper.h"

//#include <osg/ShapeDrawable>
//#include <osg/MatrixTransform>

REGISTER_GAMEOBJECT_TYPE(AsteroidFieldChunkClient, 5001);

AsteroidFieldChunkClient::AsteroidFieldChunkClient(const GameAsteroidFieldDataMessage::const_pointer& createMessage, uint16_t objId, uint32_t ownId, GameInstanceClient* ctx):
GameObject(objId, ownId, ctx)
{
    //m_rootGroup = ctx->sceneGraphRoot();
    m_chunkCoord = createMessage->get<Vector3i>("chunkCoord");
    //m_asteroidsGroup = new osg::MatrixTransform(osg::Matrix::translate(GameInstanceServer::chunkToPosition(m_chunkCoord)));
    m_asteroids = new LevelDrawable;
    m_asteroids->translate(GameInstanceServer::chunkToPosition(m_chunkCoord));
    //osg::Geode * asteroidsGeode = new osg::Geode;
    //LevelDrawable* myLevel = new LevelDrawable;
    //asteroidsGeode->addDrawable(myLevel);
    //m_asteroidsGroup->addChild(asteroidsGeode);

    auto const& posVector = createMessage->get<std::vector<Vector3>>("position");
    auto const& radVector = createMessage->get<std::vector<float>>("radius");
    for (int i = 0; i < posVector.size(); i++)
    {
        m_asteroids->addAsteroid(posVector.at(i), radVector.at(i));
    }
    //m_rootGroup->addChild(m_asteroidsGroup);
    ctx->addNodeToScene(m_asteroids);
}

AsteroidFieldChunkClient::~AsteroidFieldChunkClient()
{
    //m_rootGroup->removeChild(m_asteroidsGroup);
    static_cast<GameInstanceClient*>(m_context)->removeNodeFromScene(m_asteroids);
}

AsteroidFieldChunkClient::pointer AsteroidFieldChunkClient::createFromGameMessage(const GameMessage::const_pointer& msg, GameMessagePeer* ctx)
{
    GameAsteroidFieldDataMessage::const_pointer realMsg = msg->as<GameAsteroidFieldDataMessage>();
    GameInstanceClient* context = static_cast<GameInstanceClient*>(ctx);
    AsteroidFieldChunkClient::pointer result{ new AsteroidFieldChunkClient(realMsg, realMsg->objectId(), realMsg->get<uint32_t>("ownerId"), context) };

    context->addAsteroidFieldChunk(realMsg->get<Vector3i>("chunkCoord"), result);
    return result;
}

bool AsteroidFieldChunkClient::takeMessage(const GameMessage::const_pointer& msg, MessagePeer* sender)
{
    return false;
}

void AsteroidFieldChunkClient::setUseTesselation(bool on)
{
    //unsigned int childCount = m_asteroidsGroup->getNumChildren();
    //for (unsigned int i = 0; i < childCount; ++i)
    //{
    //    osg::Geode* child = m_asteroidsGroup->getChild(i)->asGeode();
    //    LevelDrawable* chunk = static_cast<LevelDrawable*>(child->getDrawable(0));
    //    chunk->setUseTesselation(on);
    //}
    m_asteroids->setUseTesselation(on);
}

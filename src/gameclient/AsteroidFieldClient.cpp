#include "AsteroidFieldClient.h"
#include "GameInstanceClient.h"
#include "LevelDrawable.h"
#include "../ShaderWrapper.h"

#include <osg/ShapeDrawable>

REGISTER_GAMEOBJECT_TYPE(AsteroidFieldClient, 5001);

AsteroidFieldClient::AsteroidFieldClient(const GameAsteroidFieldDataMessage::const_pointer& createMessage, uint16_t objId, uint32_t ownId, GameInstanceClient* ctx):
GameObject(objId, ownId, ctx)
{
    m_rootGroup = ctx->sceneGraphRoot();
    m_asteroidsGroup = new osg::Group;

    //osg::ref_ptr<osg::Shader> octahedronVS = new osg::Shader(osg::Shader::VERTEX);
    //osg::ref_ptr<osg::Shader> octahedronFS = new osg::Shader(osg::Shader::FRAGMENT);
    //octahedronVS->loadShaderSourceFromFile("shader/vs_octahedron.txt");
    //octahedronFS->loadShaderSourceFromFile("shader/fs_octahedron.txt");

    //osg::ref_ptr<osg::Program> octahedronShader = new osg::Program();
    //octahedronShader->addShader(octahedronVS);
    //octahedronShader->addShader(octahedronFS);
    ShaderWrapper* octahedronShader = new ShaderWrapper;
    octahedronShader->load(osg::Shader::VERTEX, "shader/vs_octahedron.txt");
    octahedronShader->load(osg::Shader::TESSCONTROL, "shader/tc_octahedron.txt");
    octahedronShader->load(osg::Shader::TESSEVALUATION, "shader/te_octahedron.txt");
    octahedronShader->load(osg::Shader::FRAGMENT, "shader/fs_octahedron.txt");
    octahedronShader->addBindAttribLocation("position", 0);
    octahedronShader->addBindAttribLocation("offset", 1);
    octahedronShader->addBindAttribLocation("scale", 2);

    osg::Geode * asteroidsGeode = new osg::Geode;
    LevelDrawable* myLevel = new LevelDrawable;
    asteroidsGeode->addDrawable(myLevel);
    m_asteroidsGroup->addChild(asteroidsGeode);
    m_asteroidsGroup->getOrCreateStateSet()->setAttributeAndModes(octahedronShader, osg::StateAttribute::ON);

    for (int i = 0; i < createMessage->position.size(); i++)
    {
        myLevel->addAsteroid(createMessage->position.at(i), createMessage->radius.at(i));
    }
    myLevel->addAsteroid(osg::Vec3f(0, 0, 0), 1.f);
    m_rootGroup->addChild(m_asteroidsGroup);
}

AsteroidFieldClient::~AsteroidFieldClient()
{
    m_rootGroup->removeChild(m_asteroidsGroup);
}

AsteroidFieldClient::pointer AsteroidFieldClient::createFromGameMessage(const GameMessage::const_pointer& msg, GameMessagePeer* ctx)
{
    GameAsteroidFieldDataMessage::const_pointer realMsg = msg->as<GameAsteroidFieldDataMessage>();
    GameInstanceClient* context = static_cast<GameInstanceClient*>(ctx);
    AsteroidFieldClient::pointer result{ new AsteroidFieldClient(realMsg, realMsg->objectId, realMsg->ownerId, context) };

    context->setAsteroidField(result);
    return result;
}

bool AsteroidFieldClient::takeMessage(const GameMessage::const_pointer& msg, MessagePeer* sender)
{
    return false;
}

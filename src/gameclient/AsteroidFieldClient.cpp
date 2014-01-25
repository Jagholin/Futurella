#include "AsteroidFieldClient.h"
#include "GameInstanceClient.h"
#include "LevelDrawable.h"

#include <osg/ShapeDrawable>

REGISTER_GAMEOBJECT_TYPE(AsteroidFieldClient, 5001);

AsteroidFieldClient::AsteroidFieldClient(const GameAsteroidFieldDataMessage::const_pointer& createMessage, uint16_t objId, uint32_t ownId, GameInstanceClient* ctx):
GameObject(objId, ownId, ctx)
{
    m_rootGroup = ctx->sceneGraphRoot();
    m_asteroidsGroup = new osg::Group;
    //int i = 0;

    // compile the shaders
    osg::ref_ptr<osg::Shader> octahedronVS = new osg::Shader(osg::Shader::VERTEX);
    osg::ref_ptr<osg::Shader> octahedronFS = new osg::Shader(osg::Shader::FRAGMENT);
    octahedronVS->loadShaderSourceFromFile("shader/vs_octahedron.txt");
    octahedronFS->loadShaderSourceFromFile("shader/fs_octahedron.txt");

    osg::ref_ptr<osg::Program> octahedronShader = new osg::Program();
    octahedronShader->addShader(octahedronVS);
    octahedronShader->addShader(octahedronFS);
    octahedronShader->addBindAttribLocation("position", 0);
    octahedronShader->addBindAttribLocation("offset", 1);

    osg::Geode * asteroidsGeode = new osg::Geode;
    LevelDrawable* myLevel = new LevelDrawable;
    asteroidsGeode->addDrawable(myLevel);
    m_asteroidsGroup->addChild(asteroidsGeode);
    m_asteroidsGroup->getOrCreateStateSet()->setAttributeAndModes(octahedronShader, osg::StateAttribute::ON);
    myLevel->addAsteroid(osg::Vec3f(0, 0, 0), 1);

    for (int i = 0; i < createMessage->position.size(); i++)
    {
        //level drawable erstellen
        //osg::ref_ptr<osg::Shape> sphere = new osg::Sphere(createMessage->position.at(i), createMessage->radius.at(i));
        //osg::ref_ptr<osg::ShapeDrawable> ast = new osg::ShapeDrawable(sphere);
        //ast->setUseDisplayList(true);
        //ast->setUseVertexBufferObjects(true);
        //ast->setColor(osg::Vec4(0.3f, 0.7f, 0.1f, 1));
        //asteroidsGeode->addDrawable(ast);
        //m_asteroidsGroup->addChild(asteroidsGeode);

        myLevel->addAsteroid(createMessage->position.at(i), createMessage->radius.at(i));
    }
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

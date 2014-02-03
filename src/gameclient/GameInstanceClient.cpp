#include "GameInstanceClient.h"
#include "../gamecommon/GameObject.h"
#include "../networking/peermanager.h"
#include "../ShaderWrapper.h"

#include <osgGA/TrackballManipulator>
#include <osg/TextureCubeMap>
#include <osg/Texture2DArray>
#include <osg/Texture2D>
#include <osgDB/ReadFile>
#include <osg/ShapeDrawable>
#include <osg/Geode>
#include <osg/BlendFunc>
#include <osgUtil/CullVisitor>

GameInstanceClient::GameInstanceClient(osg::Group* rootGroup, osgViewer::Viewer* viewer):
m_rootGraphicsGroup(rootGroup),
m_viewer(viewer),
m_connected(false),
m_orphaned(false)
{
    setupPPPipeline();

    m_viewportSizeUniform = new osg::Uniform("viewportSize", osg::Vec2f(800, 600));
    osg::Viewport* myViewport = m_viewer->getCamera()->getViewport();
    m_viewportSizeUniform->set(osg::Vec2f(myViewport->width(), myViewport->height()));
    m_rootGraphicsGroup->getOrCreateStateSet()->addUniform(m_viewportSizeUniform);
    // Create a cube with texture cubemap

    osg::TextureCubeMap *skyboxTexture = new osg::TextureCubeMap;
    skyboxTexture->setWrap(osg::Texture::WRAP_R, osg::Texture::CLAMP_TO_EDGE);
    skyboxTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
    skyboxTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
    osg::Image *cubeMapFaces[6];
    std::string cubeFileNames[] = {
        "textures/skybox1_right1.png",
        "textures/skybox1_left2.png",
        "textures/skybox1_bottom4.png",
        "textures/skybox1_top3.png",
        "textures/skybox1_front5.png",
        "textures/skybox1_back6.png"
    };
    for (unsigned int i = 0; i < 6; ++i)
    {
        cubeMapFaces[i] = osgDB::readImageFile(cubeFileNames[i]);
        assert(cubeMapFaces[i]);
        skyboxTexture->setImage(osg::TextureCubeMap::POSITIVE_X + i, cubeMapFaces[i]);
    }

    m_viewer->getCamera()->setClearMask(GL_DEPTH_BUFFER_BIT);

    ShaderWrapper *myProgram = new ShaderWrapper;
    myProgram->load(osg::Shader::VERTEX, "shader/vs_skybox.txt");
    myProgram->load(osg::Shader::FRAGMENT, "shader/fs_skybox.txt");

    // Create simple node and geode for the skybox
    // First create new prerender camera
    osg::Camera* skyboxCamera = new osg::Camera;
    skyboxCamera->setRenderOrder(osg::Camera::PRE_RENDER);
    skyboxCamera->setClearMask(GL_DEPTH_BUFFER_BIT);
    skyboxCamera->setReferenceFrame(osg::Transform::RELATIVE_RF);
    //skyboxCamera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER);
    skyboxCamera->setCullingActive(false);
    // IMPORTANT: OSG will still clamp the projection matrix with culling disabled, and
    // we don't want it to do it here.
    // So disable near/far planes calculation.
    skyboxCamera->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);

    osg::Geode* skyboxBox = new osg::Geode;
    skyboxBox->addDrawable(new osg::ShapeDrawable(new osg::Box(osg::Vec3f(0, 0, 0), 10.0f)));
    skyboxBox->getOrCreateStateSet()->setAttributeAndModes(myProgram, osg::StateAttribute::ON);
    skyboxBox->getOrCreateStateSet()->setTextureAttributeAndModes(0, skyboxTexture, osg::StateAttribute::ON);
    skyboxBox->getOrCreateStateSet()->addUniform(new osg::Uniform("skyboxTex", 0));
    skyboxBox->setCullingActive(false);
    skyboxCamera->addChild(skyboxBox);

    rootGroup->addChild(skyboxCamera);

    createTextureArrays();
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

void GameInstanceClient::setAsteroidField(AsteroidFieldChunkClient::pointer asts)
{
    m_myAsteroids = asts;
}

void GameInstanceClient::createTextureArrays()
{
    osg::ref_ptr<osg::Texture2DArray> myTex512Array = new osg::Texture2DArray;

    const unsigned int textures = 1;
    std::string textureNames[] = {
        "textures/spherical_noise2.png"
    };
    myTex512Array->setTextureHeight(512);
    myTex512Array->setTextureWidth(512);
    myTex512Array->setTextureDepth(textures);

    myTex512Array->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
    myTex512Array->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
    for (int i = 0; i < textures; ++i)
    {
        myTex512Array->setImage(i, osgDB::readImageFile(textureNames[i]));
    }

    m_rootGraphicsGroup->getOrCreateStateSet()->setTextureAttribute(1, myTex512Array);
    m_rootGraphicsGroup->getOrCreateStateSet()->addUniform(new osg::Uniform("array512Tex", 1));
}

void GameInstanceClient::setupPPPipeline()
{
    osg::ref_ptr<osg::Group> realRoot = m_rootGraphicsGroup;
    //m_rootGraphicsGroup = new osg::Group;

    osg::ref_ptr<osg::Texture2D> m_normalsTexture, m_colorTexture/*, m_backgroundColor*/;

    m_normalsTexture = new osg::Texture2D;
    m_colorTexture = new osg::Texture2D;
    //m_backgroundColor = new osg::Texture2D;
    osg::Texture2D* textures[] = {
        m_colorTexture, m_normalsTexture, //m_backgroundColor
    };
    for (osg::Texture2D* tex : textures)
    {
        tex->setTextureSize(m_viewer->getCamera()->getViewport()->width(), 
            m_viewer->getCamera()->getViewport()->height());
        tex->setInternalFormat(GL_RGBA);
        tex->setBorderWidth(0);
        tex->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
        tex->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
    }
    // create RTT Camera
    osg::ref_ptr<osg::Camera> sceneCamera = new osg::Camera;
    m_rootGraphicsGroup = sceneCamera;
    sceneCamera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
    sceneCamera->setRenderOrder(osg::Camera::PRE_RENDER);
    sceneCamera->setClearColor(osg::Vec4(0, 0, 0, 0));
    sceneCamera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //sceneCamera->attach(osg::Camera::COLOR_BUFFER0, m_backgroundColor);
    sceneCamera->attach(osg::Camera::COLOR_BUFFER0, m_colorTexture);
    sceneCamera->attach(osg::Camera::COLOR_BUFFER1, m_normalsTexture);
    sceneCamera->attach(osg::Camera::DEPTH_BUFFER, GL_DEPTH_COMPONENT16);

    realRoot->addChild(sceneCamera);

    osg::ref_ptr<osg::Geode> m_screenQuad = new osg::Geode;
    osg::ref_ptr<osg::Geometry> m_drawableQuad = new osg::Geometry;
    osg::Vec2 verts[] = {
        osg::Vec2(-1, -1), osg::Vec2(-1, 1), osg::Vec2(1, -1), osg::Vec2(1, 1)
    };
    m_drawableQuad->setVertexAttribArray(0, new osg::Vec2Array(4, verts), osg::Array::BIND_PER_VERTEX);
    m_drawableQuad->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLE_STRIP, 0, 4));

    osg::ref_ptr<ShaderWrapper> screenQuadProgram = new ShaderWrapper;
    screenQuadProgram->load(osg::Shader::VERTEX, "shader/vs_screenquad.txt");
    screenQuadProgram->load(osg::Shader::FRAGMENT, "shader/fs_screenquad.txt");

    m_screenQuad->addDrawable(m_drawableQuad);
    m_screenQuad->getOrCreateStateSet()->setTextureAttributeAndModes(0, m_colorTexture, osg::StateAttribute::ON);
    m_screenQuad->getOrCreateStateSet()->setTextureAttributeAndModes(1, m_normalsTexture, osg::StateAttribute::ON);
    m_screenQuad->getOrCreateStateSet()->addUniform(new osg::Uniform("texColor", 0));
    m_screenQuad->getOrCreateStateSet()->addUniform(new osg::Uniform("texNormals", 1));
    m_screenQuad->getOrCreateStateSet()->setAttributeAndModes(screenQuadProgram);
    m_screenQuad->getOrCreateStateSet()->setAttributeAndModes(new osg::BlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA));
    m_screenQuad->setCullingActive(false);
    realRoot->addChild(m_screenQuad);

    m_viewer->getCamera()->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
}

#include "../glincludes.h"
#include "GameInstanceClient.h"
#include "../gamecommon/GameObject.h"
#include "../networking/peermanager.h"
#include "../ShaderWrapper.h"
#include "../GUIApplication.h"
#include "../NodeCallbackService.h"

#include <osgGA/TrackballManipulator>
#include <osg/TextureCubeMap>
#include <osg/Texture2DArray>
#include <osg/Texture2D>
#include <osgDB/ReadFile>
#include <osg/ShapeDrawable>
#include <osg/Geode>
#include <osg/BlendFunc>
#include <osgUtil/CullVisitor>
#include <fmod.hpp>
#include <fmod_errors.h>

#ifdef _DEBUG
const int g_cubeOfChunksSize = 1;
const unsigned int g_removeChunksDistanceSquared = 4;
#else
const int g_cubeOfChunksSize = 2;
const unsigned int g_removeChunksDistanceSquared = 13;
#endif

class TrackGameInfoUpdate : public osg::NodeCallback
{
public:
    TrackGameInfoUpdate(GUIApplication* hud)
    {
        m_hud = hud;
        m_active = false;
    }

    void setGoal(const GameInfoClient::pointer &gi)
    {
        m_active = true;
        m_posInSpace = gi->finishArea();
    }

    virtual void operator()(osg::Node* n, osg::NodeVisitor* nv) override
    {
        if (!m_active)
            return;

        osg::Camera* realNode = static_cast<osg::Camera*>(n);
        osg::Matrix view = realNode->getViewMatrix();
        osg::Matrixd proj = view * realNode->getProjectionMatrix();

        osg::Vec4f realPos = osg::Vec4f(m_posInSpace, 1.0) * proj;
        realPos /= realPos.w();

        // Now setup hud data
        realPos.set(osg::clampBetween(realPos.x(), -1.0f, 1.0f), osg::clampBetween(realPos.y(), -1.0f, 1.0f), realPos.z(), 1.0);
        //if (realPos.z() > -1 && realPos.z() < 1)
        if (realPos.z() > 1)
        {
            if (std::abs(realPos.x()) > std::abs(realPos.y()))
            {
                float m = 1.f / std::abs(realPos.x());
                realPos *= -m;
            }
            else
            {
                float m = 1.f / std::abs(realPos.y());
                realPos *= -m;
            }
        }
            m_hud->showGoalCursorAt((realPos.x() + 1.0) * 0.5, 1.0 - (realPos.y() + 1.0) * 0.5);
        //else
        //    m_hud->hideGoalCursor();
    }
protected:
    bool m_active;
    osg::Vec3f m_posInSpace;
    GUIApplication* m_hud;
};

class MessageUpdateCallback : public osg::NodeCallback
{
public:
    MessageUpdateCallback(GameInstanceClient* cl):
        m_client(cl) {}
    virtual void operator()(osg::Node* n, osg::NodeVisitor* nv) override
    {
        m_client->onUpdatePhase();
        if (n->getNumChildrenRequiringUpdateTraversal() > 0)
            nv->traverse(*n);
    }
protected:
    GameInstanceClient* m_client;
};

GameInstanceClient::GameInstanceClient(osg::Group* rootGroup, osgViewer::Viewer* viewer, GUIApplication* guiApp):
m_rootGraphicsGroup(rootGroup),
m_viewer(viewer),
m_oldCoords(-10000, -10000, -10000),
m_HUD(guiApp),
m_connected(false),
m_orphaned(false)
{
    setupPPPipeline();
    m_fieldGoalUpdater = new TrackGameInfoUpdate(m_HUD);

    m_viewportSizeUniform = new osg::Uniform("viewportSize", osg::Vec2f(800, 600));
    osg::Viewport* myViewport = m_viewer->getCamera()->getViewport();
    m_viewportSizeUniform->set(osg::Vec2f(myViewport->width(), myViewport->height()));
    m_rootGraphicsGroup->getOrCreateStateSet()->addUniform(m_viewportSizeUniform);

    m_viewer->getCamera()->setClearMask(GL_DEPTH_BUFFER_BIT);

    osg::ref_ptr<ShaderWrapper> myProgram = new ShaderWrapper;
    myProgram->load(osg::Shader::VERTEX, "shader/vs_skybox.txt");
    myProgram->load(osg::Shader::FRAGMENT, "shader/fs_skybox.txt");

    // Create simple node and geode for the skybox
    // First create new prerender camera
    osg::ref_ptr<osg::Camera> skyboxCamera = new osg::Camera;
    skyboxCamera->setRenderOrder(osg::Camera::PRE_RENDER);
    skyboxCamera->setClearMask(GL_DEPTH_BUFFER_BIT);
    skyboxCamera->setReferenceFrame(osg::Transform::RELATIVE_RF);
    //skyboxCamera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER);
    skyboxCamera->setCullingActive(false);
    // IMPORTANT: OSG will still clamp the projection matrix with culling disabled, and
    // we don't want it to do it here.
    // So disable near/far planes calculation.
    skyboxCamera->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);

    //osg::ref_ptr<osg::Camera> environCamera = createEnvironmentCamera();
    //skyboxCamera->addChild(environCamera);
    createEnvironmentCamera(skyboxCamera);

    osg::ref_ptr<osg::Geode> skyboxBox = new osg::Geode;
    skyboxBox->addDrawable(new osg::ShapeDrawable(new osg::Box(osg::Vec3f(0, 0, 0), 10.0f)));
    skyboxBox->getOrCreateStateSet()->setAttributeAndModes(myProgram, osg::StateAttribute::ON);
    skyboxBox->getOrCreateStateSet()->setTextureAttributeAndModes(0, m_environmentMap, osg::StateAttribute::ON);
    skyboxBox->getOrCreateStateSet()->addUniform(new osg::Uniform("skyboxTex", 0));
    skyboxBox->setCullingActive(false);
    skyboxCamera->addChild(skyboxBox);

    rootGroup->addChild(skyboxCamera);

    createTextureArrays();

    m_viewer->getCamera()->setUpdateCallback(m_fieldGoalUpdater);
    //m_rootGraphicsGroup->setUpdateCallback(new NodeCallbackService(m_updateCallbackService));
    m_rootGraphicsGroup->setUpdateCallback(new MessageUpdateCallback(this));
    m_rootGraphicsGroup->setDataVariance(osg::Object::DYNAMIC);

    // Initialize FMOD engine
    FMOD_RESULT result;

    result = FMOD::System_Create(&soundSystem);     // Create the main system object.
    result = soundSystem->init(100, FMOD_INIT_NORMAL, 0);   // Initialize FMOD.

    soundSystem->createStream("music/ambience2.mp3", FMOD_DEFAULT, nullptr, &backgroundSound);

    soundSystem->playSound(FMOD_CHANNEL_FREE, backgroundSound, false, 0);
}

GameInstanceClient::~GameInstanceClient()
{
    m_viewer->setCameraManipulator(new osgGA::TrackballManipulator);
    m_viewer->getCamera()->removeUpdateCallback(m_fieldGoalUpdater);
    backgroundSound->release();
    soundSystem->release();
    //delete m_fieldGoalUpdater;
}

void GameInstanceClient::connectLocallyTo(MessagePeer* buddy, bool recursive /*= true*/)
{
    // Only one single connection to the server makes sense.
    if (m_connected)
        throw std::runtime_error("GameInstanceClient::connectLocallyTo: A GameInstanceClient cannot be connected to more than 1 server object.");
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

        shipChangedPosition(ship->getPivotLocation(), ship.get());
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
            return aShip->getObjectId() == std::get<0>(realMsg->m_values);
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

void GameInstanceClient::addAsteroidFieldChunk(GameInstanceClient::ChunkCoordinates coord, AsteroidFieldChunkClient::pointer asts)
{
    //m_myAsteroids = asts;
    m_asteroidFieldChunks[coord] = asts;
    osg::Vec3i dPos = coord - m_oldCoords;
    if (std::abs(dPos.x()) > 1 || std::abs(dPos.y()) > 1 || std::abs(dPos.z()) > 1)
        asts->setUseTesselation(false);
    else
        asts->setUseTesselation(true);
}

void GameInstanceClient::setGameInfo(GameInfoClient::pointer gi)
{
    m_myGameInfo = gi;
    //m_viewer->getCamera()->setUpdateCallback(new TrackGameInfoUpdate(m_myGameInfo, m_HUD));
    m_fieldGoalUpdater->setGoal(gi);
}

void GameInstanceClient::createTextureArrays()
{
    osg::ref_ptr<osg::Texture2DArray> myTex512Array = new osg::Texture2DArray;

    const unsigned int textures = 17;
    std::string textureNames[] = {
        "sphericalnoise0.png",
        "sphericalnoise1.png",
        "sphericalnoise2.png",
        "sphericalnoise3.png",
        "sphericalnoise4.png",
        "sphericalnoise5.png",
        "sphericalnoise6.png",
        "sphericalnoise7.png",
        "sphericalnoise8.png",
        "sphericalnoise9.png",
        "sphericalnoise10.png",
        "sphericalnoise11.png",
        "sphericalnoise12.png",
        "sphericalnoise13.png",
        "sphericalnoise14.png",
        "sphericalnoise15.png",
        "sphericalnoise16.png",
    };
    myTex512Array->setTextureHeight(512);
    myTex512Array->setTextureWidth(512);
    myTex512Array->setTextureDepth(textures);

    myTex512Array->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
    myTex512Array->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
    for (int i = 0; i < textures; ++i)
        myTex512Array->setImage(i, osgDB::readImageFile(std::string("textures/asteroids/geometry/").append(textureNames[i])));

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
        tex->setInternalFormat(GL_RGBA16F);
        tex->setSourceType(GL_FLOAT);
        tex->setSourceFormat(GL_RGBA);
        tex->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
        tex->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
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

    osg::ref_ptr<osg::Geode> screenQuad = new osg::Geode;
    osg::ref_ptr<osg::Geometry> drawableQuad = new osg::Geometry;
    osg::Vec3Array *quadVertices = new osg::Vec3Array;
    quadVertices->push_back(osg::Vec3(-1, -1, 0));
    quadVertices->push_back(osg::Vec3(1, -1, 0));
    quadVertices->push_back(osg::Vec3(-1, 1, 0));
    quadVertices->push_back(osg::Vec3(1, 1, 0));
    drawableQuad->setVertexArray(quadVertices);
    drawableQuad->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLE_STRIP, 0, 4));

    osg::ref_ptr<ShaderWrapper> screenQuadProgram = new ShaderWrapper;
    screenQuadProgram->load(osg::Shader::VERTEX, "shader/vs_screenquad.txt");
    screenQuadProgram->load(osg::Shader::FRAGMENT, "shader/fs_screenquad.txt");

    osg::ref_ptr<osg::TextureCubeMap> envLightMap = new osg::TextureCubeMap;
    //we should somehow obtain cubemap from postrender camera here, that creates a low res blurred env light texture which is for now loaded from file.
    std::string face[] = {
        "textures/lightmap/lightmap_r.png",
        "textures/lightmap/lightmap_l.png",
        "textures/lightmap/lightmap_d.png",
        "textures/lightmap/lightmap_t.png",
        "textures/lightmap/lightmap_f.png",
        "textures/lightmap/lightmap_b.png"
    };
    for (unsigned int i = 0; i < 6; ++i)
        envLightMap->setImage(osg::TextureCubeMap::POSITIVE_X + i, osgDB::readImageFile(face[i]));

    envLightMap->setWrap(osg::Texture::WRAP_R, osg::Texture::CLAMP_TO_EDGE);
    envLightMap->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
    envLightMap->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
    
    screenQuad->addDrawable(drawableQuad);
    osg::StateSet* stateset = screenQuad->getOrCreateStateSet();
    stateset->setTextureAttributeAndModes(0, m_colorTexture, osg::StateAttribute::ON);
    stateset->setTextureAttributeAndModes(1, m_normalsTexture, osg::StateAttribute::ON);
    stateset->setTextureAttributeAndModes(2, envLightMap, osg::StateAttribute::ON);
    stateset->addUniform(new osg::Uniform("texColor", 0));
    stateset->addUniform(new osg::Uniform("texNormals", 1));
    stateset->addUniform(new osg::Uniform("lightEnvMap", 2)); //use normals to add diffuse light thingy
    stateset->setAttributeAndModes(screenQuadProgram);
    stateset->setAttributeAndModes(new osg::BlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA));
    screenQuad->setCullingActive(false);
    realRoot->addChild(screenQuad);


    m_viewer->getCamera()->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
}

void GameInstanceClient::shipChangedPosition(const osg::Vec3f& pos, SpaceShipClient* ship)
{
    if (ship != m_myShip.get())
        return;
    // iterate over all asteroidFieldChunks and remove those that are no longer necessary
    std::vector<ChunkCoordinates> erasedCoords;
    ChunkCoordinates currentCoords = GameInstanceServer::positionToChunk(pos);

    // Do nothing if chunk position hasn't changed
    if (currentCoords == m_oldCoords)
        return;

    for (auto fieldChunk : m_asteroidFieldChunks)
    {
        osg::Vec3i dPos = currentCoords - fieldChunk.first;
        if (dPos.x() * dPos.x() + dPos.y()*dPos.y() + dPos.z()*dPos.z() > g_removeChunksDistanceSquared)
            erasedCoords.push_back(fieldChunk.first);
        else if (std::abs(dPos.x()) > 1 || std::abs(dPos.y()) > 1 || std::abs(dPos.z()) > 1)
            fieldChunk.second->setUseTesselation(false);
        else
            fieldChunk.second->setUseTesselation(true);
    }

    // Erase old chunks that lie too far
    for (const ChunkCoordinates& chunkCoord : erasedCoords)
    {
        // Message to the server: stop chunk tracking.
        NetStopChunkTrackingMessage::pointer msg{ new NetStopChunkTrackingMessage };
        std::get<0>(msg->m_values) = chunkCoord;
        broadcastLocally(msg);

        m_asteroidFieldChunks.erase(chunkCoord);
    }

    // Get all other chunks around yourself.
    for (int dx = -g_cubeOfChunksSize; dx <= g_cubeOfChunksSize; ++dx) 
        for (int dy = -g_cubeOfChunksSize; dy <= g_cubeOfChunksSize; ++dy) 
            for (int dz = -g_cubeOfChunksSize; dz <= g_cubeOfChunksSize; ++dz)
    {
        ChunkCoordinates newCoords = currentCoords + osg::Vec3i(dx, dy, dz);

        if (m_asteroidFieldChunks.count(newCoords) == 0)
        {
            NetRequestChunkDataMessage::pointer msg{ new NetRequestChunkDataMessage };
            std::get<0>(msg->m_values) = newCoords;
            broadcastLocally(msg);
        }
    }
    m_oldCoords = currentCoords;
}

void GameInstanceClient::gameInfoUpdated()
{
    m_fieldGoalUpdater->setGoal(m_myGameInfo);
}

void GameInstanceClient::addNodeToScene(osg::Node* aNode)
{
    // Will be run within m_rootGraphicsGroup's UpdateCallback instance
    // see m_rootGraphicsGroup->setUpdateCallback(...) call in the constructor
    //m_updateCallbackService.dispatch([aNode, this](){
        m_rootGraphicsGroup->addChild(aNode);
    //});
}

void GameInstanceClient::removeNodeFromScene(osg::Node* aNode)
{
    // Same as above
    m_updateCallbackService.dispatch([aNode, this](){
        m_rootGraphicsGroup->removeChild(aNode);
    });
}

boost::asio::io_service* GameInstanceClient::eventService()
{
    return &m_updateCallbackService;
}

void GameInstanceClient::createEnvironmentCamera(osg::Group* parentGroup)
{
    // Create a cube with texture cubemap

    osg::ref_ptr<osg::TextureCubeMap> skyboxTexture = new osg::TextureCubeMap;
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
        cubeMapFaces[i] = osgDB::readImageFile(cubeFileNames[i]);//Mem leak?
        assert(cubeMapFaces[i]);
        skyboxTexture->setImage(osg::TextureCubeMap::POSITIVE_X + i, cubeMapFaces[i]);
    }

    osg::ref_ptr<ShaderWrapper> myProgram = new ShaderWrapper;
    myProgram->load(osg::Shader::VERTEX, "shader/vs_background.txt");
    //myProgram->load(osg::Shader::GEOMETRY, "shader/gs_background.txt");
    myProgram->load(osg::Shader::FRAGMENT, "shader/fs_background.txt");

    osg::ref_ptr<osg::Geode> skyboxBox = new osg::Geode;
    skyboxBox->addDrawable(new osg::ShapeDrawable(new osg::Box(osg::Vec3f(0, 0, 0), 0.9f)));
    skyboxBox->getOrCreateStateSet()->setAttributeAndModes(myProgram, osg::StateAttribute::ON);
    skyboxBox->getOrCreateStateSet()->setTextureAttributeAndModes(0, skyboxTexture, osg::StateAttribute::ON);
    skyboxBox->getOrCreateStateSet()->addUniform(new osg::Uniform("skyboxTex", 0));
    skyboxBox->setCullingActive(false);

    osg::Vec3f lookAtVector[6] = {
        osg::Vec3f(1, 0, 0),
        osg::Vec3f(-1, 0, 0),
        osg::Vec3f(0, -1, 0),
        osg::Vec3f(0, 1, 0),
        osg::Vec3f(0, 0, -1),
        osg::Vec3f(0, 0, 1)
    };
    osg::Vec3f upVector[6] = {
        osg::Vec3f(0, 1, 0),
        osg::Vec3f(0, 1, 0),
        osg::Vec3f(0, 0, -1),
        osg::Vec3f(0, 0, 1),
        osg::Vec3f(0, 1, 0),
        osg::Vec3f(0, 1, 0)
    };

    m_environmentMap = new osg::TextureCubeMap;
    m_environmentMap->setTextureSize(512, 512);
    m_environmentMap->setInternalFormat(GL_RGBA16F);
    m_environmentMap->setSourceFormat(GL_RGBA);
    m_environmentMap->setSourceType(GL_FLOAT);
    m_environmentMap->setBorderWidth(0);
    m_environmentMap->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
    m_environmentMap->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);
    m_environmentMap->setWrap(osg::Texture::WRAP_R, osg::Texture::CLAMP_TO_EDGE);
    m_environmentMap->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
    m_environmentMap->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);

    osg::ref_ptr<osg::Camera> renderToCube[6];
    for (unsigned int i = 0; i < 6; ++i)
    {
        renderToCube[i] = new osg::Camera;

        renderToCube[i]->setRenderOrder(osg::Camera::PRE_RENDER);
        renderToCube[i]->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
        renderToCube[i]->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
        renderToCube[i]->setViewport(0, 0, 512, 512);

        renderToCube[i]->attach(osg::Camera::COLOR_BUFFER0, m_environmentMap, 0, i);
        renderToCube[i]->attach(osg::Camera::DEPTH_BUFFER, GL_DEPTH_COMPONENT16);

        renderToCube[i]->addChild(skyboxBox);

        renderToCube[i]->setViewMatrixAsLookAt(osg::Vec3(0, 0, 0), lookAtVector[i], upVector[i]);
        renderToCube[i]->setProjectionMatrixAsPerspective(90, 1, 0.1, 5000);
        renderToCube[i]->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
        renderToCube[i]->setCullingActive(false);

        parentGroup->addChild(renderToCube[i]);
    }
}

void GameInstanceClient::onUpdatePhase()
{
    // We don't do much optimizations here, just take all messages from highPriorityQueue and then from the other queue

    auto runQueue = [this](std::deque<MessagePeer::workPair>& msgQueue) {
        while (true)
        {
            workPair wP;
            {
                OpenThreads::ScopedLock<OpenThreads::Mutex> lock(m_messageQueueMutex);
                if (msgQueue.empty())
                    return;
                wP = msgQueue.front();
                msgQueue.pop_front();
            }

            takeMessage(wP.first, wP.second);
        }
    };

    runQueue(m_highPriorityQueue);
    runQueue(m_messageQueue);

    m_updateCallbackService.poll();
    m_updateCallbackService.reset();
}

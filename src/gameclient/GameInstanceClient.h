#pragma once

#include "../gamecommon/GameMessagePeer.h"
#include "../gameserver/GameInstanceServer.h"
#include "SpaceShipClient.h"
#include "AsteroidFieldChunkClient.h"
#include "GameInfoClient.h"
#include "../ChaseCam.h"
#include <Magnum/CubeMapTexture.h>
#include <Magnum/Resource.h>

//#include <osgViewer/Viewer>
#include <boost/asio/io_service.hpp>

class GUIApplication;
class TrackGameInfoUpdate;

namespace FMOD{
    class System;
    class Sound;
}

class GameInstanceClient : public GameMessagePeer
{
public:
    typedef GameInstanceServer::ChunkCoordinates ChunkCoordinates;

    GameInstanceClient(Object3D* rootGroup,/* osgViewer::Viewer* viewer,*/ GUIApplication* app);
    virtual ~GameInstanceClient();
    //...

    // Connect clientOrphaned signal to the given slot.
    // You can use this function to destroy clients that lost their connection to the server.
    template<typename T=int> void onClientOrphaned(const std::function<void()>& slot, T&& closure = int(-1)) { m_clientOrphaned.connect(slot, std::forward<T>(closure)); }

    virtual void connectLocallyTo(MessagePeer* buddy, bool recursive = true);
    virtual void disconnectLocallyFrom(MessagePeer* buddy, bool recursive = true);

    virtual bool unknownObjectIdMessage(const GameMessage::const_pointer& msg, MessagePeer* sender);

    void addExternalSpaceShip(SpaceShipClient::pointer ship);
    void addAsteroidFieldChunk(ChunkCoordinates coord, AsteroidFieldChunkClient::pointer asts);
    void setGameInfo(GameInfoClient::pointer gameInfo);
    Object3D* sceneGraphRoot();
    void createEnvironmentCamera(Object3D* parentGroup);

    virtual bool takeMessage(const NetMessage::const_pointer&, MessagePeer*);
    void shipChangedPosition(const Vector3& pos, SpaceShipClient* ship);
    void gameInfoUpdated();

    void createGameOverScreenText();

    std::multimap<int, std::string> getPlayerScores();

    // Scene graph manipulation functions, can be called by game objects
    // These methods are thread-safe
    void addNodeToScene(Object3D* aNode);
    void removeNodeFromScene(Object3D* aNode);

    // Function called once during an update callback
    void onUpdatePhase();
protected:
    addstd::signal<void()> m_clientOrphaned;
    //rootgraphicsgroup is for planets/ships/... and m_realRoot is for prerender cameras and screenquads and rootgraphicsgroup
    Object3D* m_rootGraphicsGroup, m_realRoot;     
    //osg::ref_ptr<osgViewer::Viewer> m_viewer;
    std::shared_ptr<ChaseCam> m_shipCamera;
    Resource<CubeMapTexture> m_environmentMap;

    std::vector<SpaceShipClient::pointer> m_otherShips;
    SpaceShipClient::pointer m_myShip;
    std::map<ChunkCoordinates, AsteroidFieldChunkClient::pointer> m_asteroidFieldChunks;
    ChunkCoordinates m_oldCoords;
    GameInfoClient::pointer m_myGameInfo;

    // TODO: there is no direct Magnum equivalent for osg::Uniform
    //osg::ref_ptr<osg::Uniform> m_viewportSizeUniform;
    GUIApplication* m_HUD;
    TrackGameInfoUpdate* m_fieldGoalUpdater;

    boost::asio::io_service m_updateCallbackService;

    FMOD::System* soundSystem;
    FMOD::Sound* backgroundSound;

    void createTextureArrays();
    void setupPPPipeline();

    void setupGameOverScreen();
    Object3D* m_HUDCamera;

    virtual boost::asio::io_service* eventService();

    bool m_connected;
    bool m_orphaned;
};

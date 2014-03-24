#pragma once

#include "../gamecommon/GameMessagePeer.h"
#include "../gameserver/GameInstanceServer.h"
#include "SpaceShipClient.h"
#include "AsteroidFieldChunkClient.h"
#include "GameInfoClient.h"
#include "../ChaseCam.h"

#include <osgViewer/Viewer>
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

    GameInstanceClient(osg::Group* rootGroup, osgViewer::Viewer* viewer, GUIApplication* app);
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
    osg::Group* sceneGraphRoot();
    void createEnvironmentCamera(osg::Group* parentGroup);

    virtual bool takeMessage(const NetMessage::const_pointer&, MessagePeer*);
    void shipChangedPosition(const osg::Vec3f& pos, SpaceShipClient* ship);
    void gameInfoUpdated();

    void createGameOverScreenText(int numOfPlayers, std::vector<std::string>* names, int* scores);

    // Scene graph manipulation functions, can be called by game objects
    // These methods are thread-safe
    void addNodeToScene(osg::Node* aNode);
    void removeNodeFromScene(osg::Node* aNode);

    // Function called once during an update callback
    void onUpdatePhase();
protected:
    addstd::signal<void()> m_clientOrphaned;
    //rootgraphicsgroup is for planets/ships/... and m_realRoot is for prerender cameras and screenquads and rootgraphicsgroup
    osg::ref_ptr<osg::Group> m_rootGraphicsGroup, m_realRoot;     
    osg::ref_ptr<osgViewer::Viewer> m_viewer;
    osg::ref_ptr<ChaseCam> m_shipCamera;
    osg::ref_ptr<osg::TextureCubeMap> m_environmentMap;

    std::vector<SpaceShipClient::pointer> m_otherShips;
    SpaceShipClient::pointer m_myShip;
    std::map<ChunkCoordinates, AsteroidFieldChunkClient::pointer> m_asteroidFieldChunks;
    ChunkCoordinates m_oldCoords;
    GameInfoClient::pointer m_myGameInfo;

    osg::ref_ptr<osg::Uniform> m_viewportSizeUniform;
    GUIApplication* m_HUD;
    TrackGameInfoUpdate* m_fieldGoalUpdater;

    boost::asio::io_service m_updateCallbackService;

    FMOD::System* soundSystem;
    FMOD::Sound* backgroundSound;

    void createTextureArrays();
    void setupPPPipeline();

    void setupGameOverScreen();
    osg::ref_ptr<osg::Camera> m_HUDCamera;

    virtual boost::asio::io_service* eventService();

    bool m_connected;
    bool m_orphaned;
};

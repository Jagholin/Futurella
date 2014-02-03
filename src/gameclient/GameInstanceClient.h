#pragma once

#include "../gamecommon/GameMessagePeer.h"
#include "../gameserver/GameInstanceServer.h"
#include "SpaceShipClient.h"
#include "AsteroidFieldChunkClient.h"
#include "../ChaseCam.h"

#include <osgViewer/Viewer>

class GameInstanceClient : public GameMessagePeer
{
public:
    typedef GameInstanceServer::ChunkCoordinates ChunkCoordinates;

    GameInstanceClient(osg::Group* rootGroup, osgViewer::Viewer* viewer);
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
    osg::Group* sceneGraphRoot();

    virtual bool takeMessage(const NetMessage::const_pointer&, MessagePeer*);
    void shipChangedPosition(const osg::Vec3f& pos, SpaceShipClient* ship);
protected:
    addstd::signal<void()> m_clientOrphaned;
    osg::ref_ptr<osg::Group> m_rootGraphicsGroup;
    osg::ref_ptr<osgViewer::Viewer> m_viewer;
    osg::ref_ptr<ChaseCam> m_shipCamera;

    std::vector<SpaceShipClient::pointer> m_otherShips;
    SpaceShipClient::pointer m_myShip;
    //AsteroidFieldChunkClient::pointer m_myAsteroids;
    std::map<ChunkCoordinates, AsteroidFieldChunkClient::pointer> m_asteroidFieldChunks;
    ChunkCoordinates m_oldCoords;

    osg::ref_ptr<osg::Uniform> m_viewportSizeUniform;

    void createTextureArrays();
    void setupPPPipeline();

    bool m_connected;
    bool m_orphaned;
};

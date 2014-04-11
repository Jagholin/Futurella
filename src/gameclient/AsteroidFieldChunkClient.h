#pragma once

#include "../gameserver/AsteroidFieldChunkServer.h"
#include "../gamecommon/GameObject.h"

#include <Magnum/Magnum.h>
#include <Magnum/SceneGraph/SceneGraph.h>
#include <Magnum/SceneGraph/Object.h>
#include "../magnumdefs.h"

class GameInstanceClient;
class LevelDrawable;

class AsteroidFieldChunkClient : public GameObject
{
public:
    typedef std::shared_ptr<AsteroidFieldChunkClient> pointer;

    AsteroidFieldChunkClient(const GameAsteroidFieldDataMessage::const_pointer& createMessage, uint16_t objId, uint32_t ownId, GameInstanceClient* ctx);
    ~AsteroidFieldChunkClient();

    static pointer createFromGameMessage(const GameMessage::const_pointer& msg, GameMessagePeer* ctx);

    virtual bool takeMessage(const GameMessage::const_pointer& msg, MessagePeer* sender);

    void setUseTesselation(bool on);

protected:
    //osg::ref_ptr<osg::Group> m_asteroidsGroup;
    LevelDrawable *m_asteroids;
    //Object3D *m_rootGroup;
    Vector3i m_chunkCoord;
    static int m_dummy;
};

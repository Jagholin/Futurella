#pragma once

#include "../gameserver/AsteroidFieldChunkServer.h"
#include "../gamecommon/GameObject.h"

#include <osg/Group>

class GameInstanceClient;

class AsteroidFieldChunkClient : public GameObject
{
public:
    typedef std::shared_ptr<AsteroidFieldChunkClient> pointer;

    AsteroidFieldChunkClient(const GameAsteroidFieldDataMessage::const_pointer& createMessage, uint16_t objId, uint32_t ownId, GameInstanceClient* ctx);
    ~AsteroidFieldChunkClient();

    static pointer createFromGameMessage(const GameMessage::const_pointer& msg, GameMessagePeer* ctx);

    virtual bool takeMessage(const GameMessage::const_pointer& msg, MessagePeer* sender);

protected:
    osg::ref_ptr<osg::Group> m_asteroidsGroup;
    osg::ref_ptr<osg::Group> m_rootGroup;
    osg::Vec3i m_chunkCoord;
    static int m_dummy;
};

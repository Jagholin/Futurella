#pragma once

#include "../gameserver/AsteroidFieldServer.h"
#include "../gamecommon/GameObject.h"

#include <osg/Group>

class GameInstanceClient;

class AsteroidFieldClient : public GameObject
{
public:
    typedef std::shared_ptr<AsteroidFieldClient> pointer;

    AsteroidFieldClient(const GameAsteroidFieldDataMessage::const_pointer& createMessage, uint16_t objId, uint32_t ownId, GameInstanceClient* ctx);
    ~AsteroidFieldClient();

    static pointer createFromGameMessage(const GameMessage::const_pointer& msg, GameMessagePeer* ctx);

    virtual bool takeMessage(const GameMessage::const_pointer& msg, MessagePeer* sender);

protected:
    osg::ref_ptr<osg::Group> m_asteroidsGroup;
    osg::ref_ptr<osg::Group> m_rootGroup;

    static int m_dummy;
};

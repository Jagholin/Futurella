#pragma once

#include "../gamecommon/GameObject.h"
#include "../gameserver/GameInfoServer.h"

#include <osg/Group>

class GameInstanceClient;

class GameInfoClient : public GameObject
{
public:
    typedef std::shared_ptr<GameInfoClient> pointer;

    GameInfoClient(uint16_t objId, uint32_t ownerId, GameInstanceClient* ctx);
    virtual ~GameInfoClient();

    static pointer createFromGameMessage(const GameMessage::const_pointer& msg, GameMessagePeer* ctx);

    virtual bool takeMessage(const GameMessage::const_pointer& msg, MessagePeer* sender);

protected:
    osg::Vec3f m_startingPoint, m_finishArea;
    float m_finishAreaSize;
};

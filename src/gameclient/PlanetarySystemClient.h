#pragma once

#include "../gamecommon/GameObject.h"
#include "GameInstanceClient.h"

class PlanetarySystemClient : public GameObject
{
public:
    typedef std::shared_ptr<PlanetarySystemClient> pointer;

    PlanetarySystemClient(uint16_t objId, uint32_t ownerId, GameInstanceClient* ctx);
    virtual ~PlanetarySystemClient();

    static pointer createFromGameMessage(const GameMessage::const_pointer& msg, GameMessagePeer* ctx);

    virtual bool takeMessage(const GameMessage::const_pointer& msg, MessagePeer* sender);

protected:
    static int m_dummy;
};

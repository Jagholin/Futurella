#pragma once

#include "../gamecommon/GameObject.h"
#include "../gameserver/GameInfoServer.h"

#include <Magnum/Magnum.h>
#include <Magnum/Math/Vector3.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/SceneGraph/Object.h>
#include "magnumdefs.h"

class GameInstanceClient;

class GameInfoClient : public GameObject
{
public:
    typedef std::shared_ptr<GameInfoClient> pointer;

    GameInfoClient(uint16_t objId, uint32_t ownerId, GameInstanceClient* ctx);
    virtual ~GameInfoClient();

    static pointer createFromGameMessage(const GameMessage::const_pointer& msg, GameMessagePeer* ctx);

    virtual bool takeMessage(const GameMessage::const_pointer& msg, MessagePeer* sender);
    Vector3 finishArea() const;

protected:

    Object3D* m_rootGroup;
    Object3D* m_finishAreaNode;

    Matrix4 m_transformGroup;

    Vector3 m_finishArea;
    float m_finishAreaSize;

    static int m_dummy;
};

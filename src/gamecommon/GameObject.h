#pragma once
#include "GameMessagePeer.h"

class GameObject
{
public:
    GameObject(GameMessagePeer* context);
    GameObject(uint16_t myId, GameMessagePeer* context);
    ~GameObject();

    virtual uint16_t getOwnerId() const;
    uint16_t getObjectId() const;

    virtual bool takeMessage(const GameMessage::const_pointer& msg, MessagePeer* sender) = 0;
protected:
    GameMessagePeer* m_context;
    uint16_t m_myObjectId;
};


class GameObjectFactory
{

};

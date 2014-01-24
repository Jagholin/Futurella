#pragma once
#include "GameMessagePeer.h"

BEGIN_DECLNETMESSAGE(RemoveGameObject, 7394, false)
uint16_t objectId;
END_DECLNETMESSAGE()

class GameObject
{
public:
    typedef std::shared_ptr<GameObject> pointer;

    // Server-side constructor doesn't need to be provided with an objectId
    GameObject(uint32_t ownerId, GameMessagePeer* context);
    // Client side constructor
    GameObject(uint16_t myId, uint32_t ownerId, GameMessagePeer* context);
    virtual ~GameObject();

    uint32_t getOwnerId() const;
    uint16_t getObjectId() const;

    virtual bool takeMessage(const GameMessage::const_pointer& msg, MessagePeer* sender) = 0;

    static pointer createFromGameMessage(const GameMessage::const_pointer& msg, GameMessagePeer* ctx);
protected:
    GameMessagePeer* m_context;
    uint16_t m_myObjectId;
    uint32_t m_myOwnerId;

    void messageToPartner(const GameMessage::pointer& msg);
private:
    static int m_dummy;
};

class GameObjectFactory
{
public:
    typedef std::function<GameObject::pointer(const GameMessage::const_pointer&, GameMessagePeer*)> creatorFuncType;

    static GameObject::pointer createFromGameMessage(const GameMessage::const_pointer& msg, GameMessagePeer*);
    static int registerGameObjectType(unsigned int createMessageType, const creatorFuncType &creatorFunc);

protected:
    static std::map<unsigned int, creatorFuncType> m_factories;
};

#define REGISTER_GAMEOBJECT_TYPE(className, typeId) int className::m_dummy = GameObjectFactory::registerGameObjectType(typeId, &className::createFromGameMessage);

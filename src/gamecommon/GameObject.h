#pragma once
#include "GameMessagePeer.h"

class GameObject
{
public:
    typedef std::shared_ptr<GameObject> pointer;

    GameObject(GameMessagePeer* context);
    GameObject(uint16_t myId, GameMessagePeer* context);
    ~GameObject();

    virtual uint16_t getOwnerId() const;
    uint16_t getObjectId() const;

    virtual bool takeMessage(const GameMessage::const_pointer& msg, MessagePeer* sender) = 0;

    static pointer createFromGameMessage(const GameMessage::const_pointer& msg, GameMessagePeer* ctx);
protected:
    GameMessagePeer* m_context;
    uint16_t m_myObjectId;

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

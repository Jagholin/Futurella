#include "GameObject.h"
#include <boost/lexical_cast.hpp>

REGISTER_NETMESSAGE(RemoveGameObject)

GameObject::GameObject(uint32_t ownerId, GameMessagePeer* context):
m_context(context),
m_myOwnerId(ownerId)
{
    m_myObjectId = context->registerGameObject(this);
}

GameObject::GameObject(uint16_t myId, uint32_t ownerId, GameMessagePeer* context):
m_context(context),
m_myObjectId(myId),
m_myOwnerId(ownerId)
{
    context->registerGameObject(myId, this);
}

GameObject::~GameObject()
{
    m_context->unregisterGameObject(m_myObjectId);
}

uint32_t GameObject::getOwnerId() const
{
    return m_myOwnerId;
}

uint16_t GameObject::getObjectId() const
{
    return m_myObjectId;
}

GameObject::pointer GameObject::createFromGameMessage(const GameMessage::const_pointer& msg, GameMessagePeer*)
{
    throw std::runtime_error("Not implemented: GameObject::createFromGameMessage");
}

void GameObject::messageToPartner(const GameMessage::pointer& msg)
{
    msg->objectId = m_myObjectId;
    m_context->messageToPartner(msg);
}

GameObject::pointer GameObjectFactory::createFromGameMessage(const GameMessage::const_pointer& msg, GameMessagePeer* ctx)
{
    unsigned int objectTypeId = msg->gettype();
    if (m_factories.count(objectTypeId) < 1)
        throw std::runtime_error(std::string("Can't create a game object with type id = ") + boost::lexical_cast<std::string>(objectTypeId));

    return m_factories.at(objectTypeId)(msg, ctx);
}

int GameObjectFactory::registerGameObjectType(unsigned int createMessageType, const creatorFuncType &creatorFunc)
{
    m_factories.insert(std::make_pair(createMessageType, creatorFunc));
    return 0;
}

std::map<unsigned int, GameObjectFactory::creatorFuncType> GameObjectFactory::m_factories;

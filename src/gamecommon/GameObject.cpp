#include "GameObject.h"

GameObject::GameObject(GameMessagePeer* context):
m_context(context)
{
    m_myObjectId = context->registerGameObject(this);
}

GameObject::GameObject(uint16_t myId, GameMessagePeer* context):
m_context(context)
{
    context->registerGameObject(myId, this);
}

GameObject::~GameObject()
{
    m_context->unregisterGameObject(m_myObjectId);
}

uint16_t GameObject::getOwnerId() const
{
    return m_context->getOwnerId();
}

uint16_t GameObject::getObjectId() const
{
    return m_myObjectId;
}

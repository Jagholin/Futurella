#include "GameMessagePeer.h"
#include "GameObject.h"

#include <stdexcept>

GameMessagePeer::GameMessagePeer()
{
    m_lastObjectId = 0;
}

GameMessagePeer::~GameMessagePeer()
{
    // nop
}

bool GameMessagePeer::takeMessage(const NetMessage::const_pointer& msg, MessagePeer* sender)
{
    if (msg->isGameMessage())
    {
        GameMessage::const_pointer gameMsg = msg->as<GameMessage>();
        if (m_messageListeners.count(gameMsg->objectId()) == 0)
            return unknownObjectIdMessage(gameMsg, sender);
        GameObject* receiver = m_messageListeners.at(gameMsg->objectId());
        receiver->takeMessage(gameMsg, sender);
        // takeMessage(msg->as<GameMessage>(), sender);
    }
    return false;
}

uint16_t GameMessagePeer::getOwnerId() const
{
    return 0;
}

void GameMessagePeer::registerGameObject(uint16_t objId, GameObject* obj)
{
    if (m_messageListeners.count(objId) > 0)
        throw std::runtime_error("Object ID collision in GameMessagePeer::registerGameObject");

    m_messageListeners[objId] = obj;
}

uint16_t GameMessagePeer::registerGameObject(GameObject* obj)
{
    ++m_lastObjectId;
    if (m_messageListeners.count(m_lastObjectId) > 0)
        throw std::runtime_error("Object ID collision in automatic GameMessagePeer::registerGameObject");

    m_messageListeners[m_lastObjectId] = obj;
    return m_lastObjectId;
}

void GameMessagePeer::unregisterGameObject(uint16_t objId)
{
    m_messageListeners.erase(objId);
}

void GameMessagePeer::messageToPartner(const GameMessage::const_pointer& msg)
{
    broadcastLocally(msg);
}

bool GameMessagePeer::send(const NetMessage::const_pointer& msg, MessagePeer* from /*= nullptr*/)
{
    // Adding the message to the queue
    boost::asio::io_service* eventServ = eventService();
    m_messageQueue.push_back(std::make_pair(msg, from));
    if (!m_isWorking)
    {
        m_isWorking = true;
        do
        {
            workPair curMessage = m_messageQueue.front();
            m_messageQueue.pop_front();
            eventServ->dispatch([this, curMessage](){
                takeMessage(curMessage.first, curMessage.second);
            });
        } while (m_messageQueue.empty() == false);
        m_isWorking = false;
    }
    return true;
}

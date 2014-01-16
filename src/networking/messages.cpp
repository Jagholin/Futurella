/*
    Copyright (C) 2009 - 2014 Pavel Belskiy, github.com/Jagholin
 */
#include "messages.h"
#include <algorithm>
#include <cassert>

MsgFactory::factoryMap MsgFactory::msgFactories;

MessagePeer::MessagePeer(): m_isWorking(false)
{}

MessagePeer::~MessagePeer()
{
    assert(!m_isWorking);
    std::for_each(m_localPeers.begin(), m_localPeers.end(), std::bind(
            &MessagePeer::disconnectLocallyFrom, std::placeholders::_1, this, false));
}

void MessagePeer::connectLocallyTo(MessagePeer* buddy, bool recursive /* = true */)
{
    m_localPeers.push_back(buddy);
    if (recursive)
        buddy->connectLocallyTo(this, false);
}

void MessagePeer::disconnectLocallyFrom(MessagePeer* buddy, bool recursive /* = true */)
{
    vector<MessagePeer*>::iterator toClear = std::find(m_localPeers.begin(), m_localPeers.end(), buddy);
    assert(toClear != m_localPeers.end());
    m_localPeers.erase(toClear);
    assert(std::find(m_localPeers.begin(), m_localPeers.end(), buddy) == m_localPeers.end());
    // We need to drop all the work which came from the buddy
    if (!m_messageQueue.empty()) for (std::deque<workPair>::iterator it = m_messageQueue.begin(); it != m_messageQueue.end(); ++it)
    {
        if (it->second == buddy)
        {
            m_messageQueue.erase(it);
            // The Iterator is invalidated, begin once again
            it = m_messageQueue.begin();
        }
    }
    if (recursive)
        buddy->disconnectLocallyFrom(this, false);
}

void MessagePeer::broadcastLocally(const NetMessage::const_pointer& msg)
{
    std::for_each(m_localPeers.begin(), m_localPeers.end(), std::bind(&MessagePeer::send, std::placeholders::_1, msg, this));
}

bool MessagePeer::send(const NetMessage::const_pointer& msg, MessagePeer* from)
{
    // Adding the message to the queue
    m_messageQueue.push_back(std::make_pair(msg, from));
    bool result = true;
    if (!m_isWorking)
    {
        m_isWorking = true;
        do 
        {
            workPair curMessage = m_messageQueue.front();
            m_messageQueue.pop_front();
            result = takeMessage(curMessage.first, curMessage.second);
        } while (m_messageQueue.empty() == false);
        m_isWorking = false;
    }
    return result;
}

unsigned int MsgFactory::regFactory(unsigned int id, const factoryFunc& f)
{
    msgFactories[id] = f;
    return id;
}

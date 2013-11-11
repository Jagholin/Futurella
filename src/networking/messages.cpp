/*
	Copyright (C) 2009 - 2014 Pavel Belskiy, github.com/Jagholin
 */
#include "messages.h"
#include <algorithm>
#include <cassert>

MsgFactory::factoryMap* MsgFactory::msgFactories = 0;

MessagePeer::MessagePeer(): isWorking(false)
{}

MessagePeer::~MessagePeer()
{
	assert(!isWorking);
	std::for_each(transPointers.begin(), transPointers.end(), std::bind(
			&MessagePeer::disconnectFrom, std::placeholders::_1, this, false));
}

void MessagePeer::connectTo(MessagePeer* buddy, bool recursive /* = true */)
{
	transPointers.push_back(buddy);
	if (recursive)
		buddy->connectTo(this, false);
}

void MessagePeer::disconnectFrom(MessagePeer* buddy, bool recursive /* = true */)
{
	vector<MessagePeer*>::iterator toClear = std::find(transPointers.begin(), transPointers.end(), buddy);
	assert(toClear != transPointers.end());
	transPointers.erase(toClear);
	assert(std::find(transPointers.begin(), transPointers.end(), buddy) == transPointers.end());
	// We need to drop all the work which came from the buddy
	if (!messageQueue.empty()) for (std::deque<workPair>::iterator it = messageQueue.begin(); it != messageQueue.end(); ++it)
	{
		if (it->second == buddy)
		{
			messageQueue.erase(it);
			// The Iterator is invalidated, begin once again
			it = messageQueue.begin();
		}
	}
	if (recursive)
		buddy->disconnectFrom(this, false);
}

void MessagePeer::sendMessageToAll(const NetMessage::const_pointer& msg)
{
	std::for_each(transPointers.begin(), transPointers.end(), std::bind(&MessagePeer::queueMessage, std::placeholders::_1, msg, this));
}

void MessagePeer::queueMessage(const NetMessage::const_pointer& msg, MessagePeer* from)
{
	// Adding the message to the queue
	messageQueue.push_back(std::make_pair(msg, from));
	if (!isWorking)
	{
		isWorking = true;
		do 
		{
			workPair curMessage = messageQueue.front();
			messageQueue.pop_front();
			this->translateMessage(curMessage.first, curMessage.second);
		} while (messageQueue.empty() == false);
		isWorking = false;
	}
}

unsigned int MsgFactory::regFactory(unsigned int id, const factoryFunc& f)
{
	if (!msgFactories)
		msgFactories = new factoryMap();
	(*msgFactories)[id] = f;
	return id;
}

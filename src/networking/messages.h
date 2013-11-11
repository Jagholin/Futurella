/*
	Copyright (C) 2009 - 2014 Pavel Belskiy, github.com/Jagholin
 */
#pragma once

#include <map>
#include <vector>
#include <deque>
#include <memory>
#include "networking.h"
#include <osgDB/OutputStream>

class NetMessage
{
public:
	virtual ~NetMessage(){}
	virtual RawMessage::pointer toRaw()const = 0;
	virtual unsigned int gettype()const = 0;

	typedef std::shared_ptr<NetMessage> pointer;
	typedef std::shared_ptr<const NetMessage> const_pointer;
};

#define DECLMESSAGE_BASE(name, number, prefix, baseclass) class prefix##name##Message : public baseclass { \
	public: RawMessage::pointer toRaw()const; \
	enum { type = number }; \
	protected: static unsigned int mtype; \
	public: unsigned int gettype()const { return mtype; } \
	static NetMessage::const_pointer fromRaw(const std::string&);

#define BEGIN_DECLNETMESSAGE(name, number) DECLMESSAGE_BASE(name, number, Net, NetMessage)

#define END_DECLNETMESSAGE() };

#define REGISTER_MESSAGE_BASE(name, prefix) unsigned int prefix##name##Message::mtype = MsgFactory::regFactory(prefix##name##Message::type, &prefix##name##Message::fromRaw);

#define REGISTER_NETMESSAGE(name) REGISTER_MESSAGE_BASE(name, Net)

#define BEGIN_TORAWMESSAGE_QCONVERTBASE(name, prefix) RawMessage::pointer prefix##name##Message::toRaw() const { \
	RawMessage::pointer result(new RawMessage); \
	result->msgType = mtype; \
	std::stringstream outStr; {

#define BEGIN_NETTORAWMESSAGE_QCONVERT(name) BEGIN_TORAWMESSAGE_QCONVERTBASE(name, Net)

#define END_NETTORAWMESSAGE_QCONVERT() } result->msgBytes = outStr.str(); \
	return result; }

#define BEGIN_TONETMESSAGE_QCONVERTBASE(name, prefix) NetMessage::const_pointer prefix##name##Message::fromRaw(const std::string& str) { \
	std::istringstream ourStr(str); \
	prefix##name##Message* temp = new prefix##name##Message; 

#define BEGIN_RAWTONETMESSAGE_QCONVERT(name) BEGIN_TONETMESSAGE_QCONVERTBASE(name, Net)

#define END_RAWTONETMESSAGE_QCONVERT() return NetMessage::pointer(temp);}


class MessagePeer
{
protected:
	std::vector<MessagePeer*> transPointers;
	virtual bool translateMessage(const NetMessage::const_pointer&, MessagePeer*) = 0;
	//virtual void alone(Translator::pointer);
public:
	MessagePeer();
	virtual ~MessagePeer();
	//virtual void regTranslator(Translator::pointer);
	virtual void connectTo(MessagePeer* buddy, bool recursive = true);
	virtual void disconnectFrom(MessagePeer* buddy, bool recursive = true);
	void sendMessageToAll(const NetMessage::const_pointer&);
	void queueMessage(const NetMessage::const_pointer&, MessagePeer*);
	//friend class Translator;
private:
	typedef std::pair<NetMessage::const_pointer, MessagePeer*> workPair;
	std::deque<workPair> messageQueue;
	bool isWorking;
};

class MsgFactory
{
public:
	typedef std::function<NetMessage::const_pointer(const std::string&)> factoryFunc;
	typedef std::map<unsigned int, factoryFunc > factoryMap;
private:
	static factoryMap* msgFactories;
	MsgFactory();
public:
	static unsigned int regFactory(unsigned int, const factoryFunc&);
	static NetMessage::const_pointer create(const RawMessage& msg)
	{
		if (msgFactories->count(msg.msgType) > 0)
			return (*msgFactories)[msg.msgType](msg.msgBytes);
		throw std::bad_cast();
	}
};

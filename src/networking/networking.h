/*
	networking.h
	Created on: Feb 16, 2009
	Copyright (C) 2009 - 2014 Pavel Belskiy, github.com/Jagholin
 */
#pragma once
#include <vector>
#include <memory>
#include <string>
#include <functional>
#include <algorithm>
#include <boost/asio/io_service.hpp>

using std::vector;
struct RawMessage
{
	unsigned int msgType;
	std::string msgBytes;
	typedef std::shared_ptr<RawMessage> pointer;
	typedef std::shared_ptr<const RawMessage> const_pointer;
};

struct NetworkError
{
	std::string str;
};

class NetConnectionListener
{
public:
	virtual void onMessageSent(/*unsigned int,*/ unsigned int)=0;
	virtual void onMessageReceived(/*unsigned int,*/ RawMessage::pointer)=0;
	virtual void onConnected(/*unsigned int*/)=0;
	virtual void onDisconnect(/*unsigned int,*/ const std::string&)=0;
};

class NetConnection;
class NetServerListener
{
public:
	virtual void onConnection(NetConnection*);
	virtual void onStop();
};

class NetConnImpl;
class NetConnection
{
private:
	std::shared_ptr<NetConnImpl> privData;
	//unsigned int myId;
public:
	NetConnection(boost::asio::io_service& s);
	~NetConnection();

	void connectTo(std::string addr, unsigned int port);
	unsigned int sendMessage(RawMessage::const_pointer);
	std::string getAddrString()const;
	bool isActive()const;
	//void setId(unsigned int);
	void close();

	void scheduleDeletion();

	void setNetworkListener(NetConnectionListener* newListener)
	{
		onMessageSent = std::bind(&NetConnectionListener::onMessageSent, newListener, std::placeholders::_1);
		onMessageReceived = std::bind(&NetConnectionListener::onMessageReceived, newListener, std::placeholders::_1);
		onConnected = std::bind(&NetConnectionListener::onConnected, newListener);
		onDisconnect = std::bind(&NetConnectionListener::onDisconnect, newListener, std::placeholders::_1);
	}

	friend class NetConnImpl;
	friend class NetServerImpl;

protected:
	void onNetworkError(const NetworkError&);
	void deleteNow();

	std::function<void (/*unsigned int,*/ unsigned int)> onMessageSent;
	std::function<void (/*unsigned int,*/ RawMessage::pointer)> onMessageReceived;
	std::function<void (/*unsigned int*/)> onConnected;
	std::function<void (/*unsigned int,*/ std::string)> onDisconnect;

	boost::asio::io_service& m_service;
};

class NetServerImpl;
class NetServer
{
private:
	std::shared_ptr<NetServerImpl> privData;
public:
	NetServer(boost::asio::io_service& s);
	~NetServer();

	bool listen(unsigned int port, std::string& errMsg);
	void stop();
	void setNetworkListener(NetServerListener* newListener)
	{
		onConnection = std::bind(&NetServerListener::onConnection, newListener, std::placeholders::_1);
		onStop = std::bind(&NetServerListener::onStop, newListener);
	}

	friend class NetServerImpl;
protected:
	std::function<void (NetConnection*)> onConnection;
	std::function<void ()> onStop;

	//boost::asio::io_service& m_service;
};

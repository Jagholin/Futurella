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
#include <boost/asio/ip/udp.hpp>
#include "sigslot.h"

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

class NetConnImpl;
class NetConnection
{
private:
	std::shared_ptr<NetConnImpl> privData;
	//unsigned int myId;
public:
	NetConnection(boost::asio::io_service& s);
	~NetConnection();

	void connectTo(std::string addr, unsigned int portTCP, unsigned int portUDP);
	unsigned int sendMessage(RawMessage::const_pointer);
    unsigned int sendAsUDP(RawMessage::const_pointer);
	std::string getAddrString()const;
	bool isActive()const;
	void close();

	void scheduleDeletion();

    template<typename T> void onMessageSent(const std::function<void(unsigned int)>& func, T&& closure) { m_messageSent.connect(func, std::forward<T>(closure)); }
    template<typename T> void onMessageReceived(const std::function<void(RawMessage::pointer)>& func, T&& closure) { m_messageReceived.connect(func, std::forward<T>(closure)); }
    template<typename T> void onConnected(const std::function<void()>& func, T&& closure) { m_connected.connect(func, std::forward<T>(closure)); }
    template<typename T> void onDisconnected(const std::function<void(std::string)>& func, T&& closure) { m_disconnected.connect(func, std::forward<T>(closure)); }
    template<typename T> void onWarning(const std::function<void(std::string)>& func, T&& closure) { m_warn.connect(func, std::forward<T>(closure)); }

	friend class NetConnImpl;
	friend class NetServerImpl;

protected:
	void onNetworkError(const NetworkError&);
	void deleteNow();

	addstd::signal<void (/*unsigned int,*/ unsigned int)> m_messageSent;
    addstd::signal<void(/*unsigned int,*/ RawMessage::pointer)> m_messageReceived;
    addstd::signal<void(/*unsigned int*/)> m_connected;
    addstd::signal<void(/*unsigned int,*/ std::string)> m_disconnected;
    addstd::signal<void(std::string)> m_warn;

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

    bool listen(unsigned int portTCP, unsigned int portUDP, std::string& errMsg);
    void stop();
    template<typename T> void onNewConnection(const std::function<void(NetConnection*)>& func, T&& closure) { m_newConnection.connect(func, std::forward<T>(closure)); }
    template<typename T> void onStopped(const std::function<void()>& func, T&& closure) { m_stopped.connect(func, std::forward<T>(closure)); }

	friend class NetServerImpl;
protected:
	addstd::signal<void (NetConnection*)> m_newConnection;
    addstd::signal<void(RawMessage::const_pointer, boost::asio::ip::udp::endpoint)> m_newDatagram;
	addstd::signal<void ()> m_stopped;
};

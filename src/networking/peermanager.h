/*
Copyright (C) 2009 - 2014 Belskiy Pavel, github.com/Jagholin
*/
#pragma once

#include "messages.h"
#include <map>
#include <ctime>
#include <functional>

#include <osg/Referenced>
class NetHalloMessage;
class NetConnection;

class FuturellaPeer: public MessagePeer, public NetConnectionListener, public osg::Referenced
{
protected:
	typedef std::function<void (NetMessage::const_pointer, FuturellaPeer*)> msgFunc;
	NetConnection* myConnect;
	std::deque<msgFunc> receivers;
	std::deque<NetMessage::const_pointer> activationWaitingQueue;
	bool activated;
	mutable unsigned int halloStatus;
	std::string buddyName, myName;
	uint32_t peerIdentityKey;
	//QStandardItem* peerDataItem;
	virtual bool translateMessage(const NetMessage::const_pointer&, MessagePeer*);
	// This constructor should be called only from TunnelingFuturellaPeer class
	FuturellaPeer();
public:
	enum {protokoll_version = 1002};
	enum PeerStatus {MP_CONNECTING = 0, MP_ACTIVE, MP_SERVER, MP_DISCONNECTED};
	FuturellaPeer(NetConnection*, bool serverSide);
	virtual ~FuturellaPeer();

	void regReceiver(const msgFunc&);
	virtual bool sendMessage(const NetMessage::const_pointer&);
	PeerStatus getStatus()const;
	std::string getRemoteName()const;
	//QModelIndex getModelIndex()const;
	virtual bool isDirect()const
	{
		return true;
	}

	typedef osg::ref_ptr<FuturellaPeer> pointer;

	void onPeerDestruction(const std::function<void()> &callBack) { m_tobeDestroyed = callBack; }
	void onError(const std::function<void(std::string)> &callBack) { m_errorSignal = callBack; }
	void onActivation(const std::function<void()> &callBack) { m_onActivated = callBack; }
	void onStatusChange(const std::function<void()> &callBack) { m_statusChanged = callBack; }

protected:
	virtual void onMessageReceived(RawMessage::pointer);
	virtual void onDisconnect(const std::string&);
	virtual void onConnected();
	virtual void onMessageSent(unsigned int) {}

	std::function<void ()> m_tobeDestroyed;
	std::function<void (std::string)> m_errorSignal;
	std::function<void ()> m_onActivated;
	std::function<void ()> m_statusChanged;
protected:
	void halloProceed(const std::shared_ptr<const NetHalloMessage>& hallo);
	NetMessage::pointer createHalloMsg()const;
	PeerStatus ourStatus;
};

class PeersManager
{
protected:
	static PeersManager* mSingleton;
	std::deque<FuturellaPeer::pointer> msgPeers;
	std::map<uint32_t, FuturellaPeer::pointer> registeredPeerIds;
	uint32_t myPeerId;
	//QStandardItemModel* mPeersModel;
	std::string peerName;

	PeersManager();
	void unregisterPeer(FuturellaPeer::pointer);
	void regFuturellaPeer(FuturellaPeer::pointer);
	bool registerPeerId(uint32_t id, FuturellaPeer::pointer);
	void unregisterPeerId(uint32_t id);

	FuturellaPeer::pointer peerFromId(uint32_t id)const;
public:
	static PeersManager* getManager();

	bool isAnyoneConnected()const;
	void broadcast(NetMessage::const_pointer, FuturellaPeer::pointer exclude = 0);

	//QStandardItemModel* getPeersModel()const;
	void setMyName(std::string name);
	std::string getMyName()const;
	//FuturellaPeer::pointer peerFromIndex(QModelIndex index);

	void onNewPeerRegistration(const std::function<void(FuturellaPeer::pointer)> &callBack)
	{
		m_peerRegistred = callBack;
	}

	friend class FuturellaPeer;
	friend class TunnelingFuturellaPeer;
protected:
	std::function<void(FuturellaPeer::pointer)> m_peerRegistred;
public:
	void sendChatMessage(const std::string&);
protected:
	void onPeerActivated();
};

BEGIN_DECLNETMESSAGE(Chat, 100)
std::string message;
std::time_t sentTime;
END_DECLNETMESSAGE()

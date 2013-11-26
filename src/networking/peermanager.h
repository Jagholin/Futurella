/*
Copyright (C) 2009 - 2014 Belskiy Pavel, github.com/Jagholin
*/
#pragma once

#include "messages.h"
#include <map>
#include <ctime>
#include <functional>

#include <osg/Referenced>
#include "sigslot.h"
class NetHalloMessage;
class NetConnection;

class FuturellaPeer: public MessagePeer, public NetConnectionListener, public osg::Referenced
{
protected:
	typedef std::function<void (NetMessage::const_pointer, FuturellaPeer*)> msgFunc;

	NetConnection* m_connectLine;
	std::deque<msgFunc> m_receivers;
	std::deque<NetMessage::const_pointer> m_activationWaitingQueue;
	std::deque<osg::observer_ptr<FuturellaPeer> > m_dependencyList;
	bool m_active;
	mutable unsigned int m_halloStatus;
	std::string m_buddyName, m_myName;
	uint32_t m_peerIdentityKey;

	virtual bool translateMessage(const NetMessage::const_pointer&, MessagePeer*);
	FuturellaPeer();

public:
	enum {protokoll_version = 1002};
	enum PeerStatus {MP_CONNECTING = 0, MP_ACTIVE, MP_SERVER, MP_DISCONNECTED};

	FuturellaPeer(NetConnection*, bool serverSide);
	virtual ~FuturellaPeer();

	virtual bool sendMessage(const NetMessage::const_pointer&);
	PeerStatus getStatus()const;
	std::string getRemoteName()const;
	//QModelIndex getModelIndex()const;
	virtual bool isDirect()const
	{
		return true;
	}

	typedef osg::ref_ptr<FuturellaPeer> pointer;

	void onPeerDestruction(const std::function<void()> &callBack, osg::Referenced *closure) { m_tobeDestroyed.connect(callBack, closure); }
	void onError(const std::function<void(std::string)> &callBack, osg::Referenced *closure) { m_errorSignal.connect(callBack, closure); }
	void onActivation(const std::function<void()> &callBack, osg::Referenced *closure) { m_onActivated.connect(callBack, closure); }
	void onStatusChange(const std::function<void()> &callBack, osg::Referenced *closure) { m_statusChanged.connect(callBack, closure); }
	void onMessage(const msgFunc &callBack);

	void addDependablePeer(FuturellaPeer* child);

protected:
	virtual void onMessageReceived(RawMessage::pointer);
	virtual void onDisconnect(const std::string&);
	virtual void onConnected();
	virtual void onMessageSent(unsigned int) {}

	addstd::signal <void()> m_tobeDestroyed;
	addstd::signal <void (std::string)> m_errorSignal;
	addstd::signal <void ()> m_onActivated;
	addstd::signal <void ()> m_statusChanged;

	void halloProceed(const std::shared_ptr<const NetHalloMessage>& hallo);
	NetMessage::pointer createHalloMsg()const;
	PeerStatus m_status;
};

class PeersManager : public osg::Referenced
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

	void onNewPeerRegistration(const std::function<void(FuturellaPeer::pointer)> &callBack, osg::Referenced* closure)
	{
		m_peerRegistred.connect(callBack, closure);
	}

	friend class FuturellaPeer;
	friend class TunnelingFuturellaPeer;
protected:
	addstd::signal<void(FuturellaPeer::pointer)> m_peerRegistred;
public:
	void sendChatMessage(const std::string&);
protected:
	void onPeerActivated(const FuturellaPeer::pointer& sender);
};

BEGIN_DECLNETMESSAGE(Chat, 100)
std::string message;
std::time_t sentTime;
END_DECLNETMESSAGE()

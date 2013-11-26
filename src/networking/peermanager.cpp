/*
	Copyright (C) 2009 - 2014 Belskiy Pavel, github.com/Jagholin
*/
#include "peermanager.h"
#include <cstdlib>
#include <cassert>
#include <algorithm>
#include <set>
#include <chrono>

#ifdef _WIN32
#	include <winsock2.h>
#else
#	include <netinet/in.h>
#endif

PeersManager* PeersManager::mSingleton = 0;

BEGIN_DECLNETMESSAGE(Hallo, 1000)
std::string buddyName;
uint32_t halloStat;
uint32_t peerId;
uint16_t version;
END_DECLNETMESSAGE()

BEGIN_DECLNETMESSAGE(AvailablePeer, 1001)
std::string buddyName;
uint32_t peerId;
END_DECLNETMESSAGE()

BEGIN_DECLNETMESSAGE(Tunneled, 1002)
uint32_t peerIdWhom;
uint32_t peerIdFrom;
uint32_t msgType;
std::string msgData;
END_DECLNETMESSAGE()

BEGIN_DECLNETMESSAGE(PeerUnavailable, 1003)
uint32_t peerId;
END_DECLNETMESSAGE()

class TunnelingFuturellaPeer: public FuturellaPeer
{
public:
	TunnelingFuturellaPeer(FuturellaPeer::pointer tunnel, const NetAvailablePeerMessage* msg);
	TunnelingFuturellaPeer(FuturellaPeer::pointer tunnel, uint32_t peerId);
	~TunnelingFuturellaPeer();
	virtual bool sendMessage(const NetMessage::const_pointer& msg);
	bool isDirect()const
	{
		return false;
	}
protected:
	FuturellaPeer::pointer mTunnel;
};

TunnelingFuturellaPeer::TunnelingFuturellaPeer(FuturellaPeer::pointer tunnel, const NetAvailablePeerMessage* msg):
	mTunnel(tunnel)
{
	m_peerIdentityKey = msg->peerId;
	m_buddyName = msg->buddyName;
	if (PeersManager::getManager()->registerPeerId(m_peerIdentityKey, this) == false)
		// this->deleteLater(); TODO: think over owner strategy
		;
	PeersManager::getManager()->regFuturellaPeer(this);
}

TunnelingFuturellaPeer::TunnelingFuturellaPeer(FuturellaPeer::pointer tunnel, uint32_t peerId):
mTunnel(tunnel)
{
	m_peerIdentityKey = peerId;
	m_buddyName = std::string("[Unknown Source]");
	assert(PeersManager::getManager()->registerPeerId(m_peerIdentityKey, this));
	PeersManager::getManager()->regFuturellaPeer(this);
	tunnel->addDependablePeer(this);
}

TunnelingFuturellaPeer::~TunnelingFuturellaPeer()
{
}

bool TunnelingFuturellaPeer::sendMessage(const NetMessage::const_pointer& msg)
{
	if (!m_active)
		return false;
	if (msg->gettype() == NetTunneledMessage::type)
		return mTunnel->sendMessage(msg);
	NetTunneledMessage* message = new NetTunneledMessage;
	message->peerIdWhom = m_peerIdentityKey;
	message->peerIdFrom = PeersManager::getManager()->myPeerId;
	message->msgType = msg->gettype();
	RawMessage::const_pointer toSend = msg->toRaw();
	if (!toSend->msgBytes.empty())
		message->msgData = toSend->msgBytes;
	return mTunnel->sendMessage(NetMessage::const_pointer(message));
}

FuturellaPeer::FuturellaPeer():
	m_active(true)
{
	m_connectLine = nullptr;
	m_halloStatus = 0;
	m_status = MP_ACTIVE;
	m_myName = PeersManager::getManager()->peerName;
}

FuturellaPeer::FuturellaPeer(NetConnection* con, bool serverSide) :
	m_active(false), m_halloStatus(1), m_buddyName("[Broken Connection]")
{
	m_peerIdentityKey = 0;
	m_connectLine = con;
	m_status = MP_CONNECTING;
	m_myName = PeersManager::getManager()->peerName;
	assert(con);
	bool runHallo = false;
	if (con)
	{
		if (con->isActive())
			runHallo = true;
		else
			con->setNetworkListener(this);
	}
	PeersManager::getManager()->regFuturellaPeer(this);
	if (runHallo && !serverSide)
		onConnected();
}

FuturellaPeer::~FuturellaPeer()
{
	m_tobeDestroyed();
	PeersManager::getManager()->unregisterPeer(this);
	if (m_active && m_connectLine != 0)
	{
		NetPeerUnavailableMessage* msg = new NetPeerUnavailableMessage;
		msg->peerId = m_peerIdentityKey;
		PeersManager::getManager()->broadcast(NetMessage::const_pointer(msg), this);
	}
	if (m_peerIdentityKey)
		PeersManager::getManager()->unregisterPeerId(m_peerIdentityKey);
	if (m_connectLine)
		m_connectLine->scheduleDeletion();
}

void FuturellaPeer::onMessage(const msgFunc &f)
{
	m_receivers.push_back(f);
}

bool FuturellaPeer::translateMessage(const NetMessage::const_pointer& msg,
								   MessagePeer*)
{
	return sendMessage(msg);
}

bool FuturellaPeer::sendMessage(const NetMessage::const_pointer& msg)
{
	// We can't send any messages till we are ready with greetings procedure
	if (m_status == MP_DISCONNECTED)
		return false;
	if (m_active)
		m_connectLine->sendMessage(msg->toRaw());
	else
		m_activationWaitingQueue.push_back(msg);
	return m_active;
}

void FuturellaPeer::onMessageReceived(RawMessage::pointer msg)
{
	try
	{
		NetMessage::const_pointer realMsg = MsgFactory::create(*msg);
		// If we are not ready, only greetings messages are allowed
		if (m_active)
		{
			if (realMsg->gettype() == NetAvailablePeerMessage::type)
			{
				// Create new tunneling peer object#
				const NetAvailablePeerMessage* tunnelMsg = static_cast<const NetAvailablePeerMessage*>(realMsg.get());
				new TunnelingFuturellaPeer(this, tunnelMsg);
			}
			else if (realMsg->gettype() == NetTunneledMessage::type)
			{
				const NetTunneledMessage* tunnelMsg = static_cast<const NetTunneledMessage*>(realMsg.get());
				if (tunnelMsg->peerIdWhom == PeersManager::getManager()->myPeerId)
				{
					RawMessage::pointer innerMsg(new RawMessage);
					FuturellaPeer::pointer tunneledPeer = PeersManager::getManager()->peerFromId(tunnelMsg->peerIdFrom);
					if (tunneledPeer == 0)
					{
						// It shouldn't happen, but it's still possible to create a new TunnelingFuturellaPeer object
						//qDebug() << "Unexpected message from unknown source, id " << tunnelMsg->peerIdFrom << ". Creating tunnel...";
						tunneledPeer = new TunnelingFuturellaPeer(this, tunnelMsg->peerIdFrom);
					}
					innerMsg->msgType = tunnelMsg->msgType;
					innerMsg->msgBytes.assign(tunnelMsg->msgData.data(), tunnelMsg->msgData.data() + tunnelMsg->msgData.size());
					tunneledPeer->onMessageReceived(innerMsg);
					return;
				}
				// The message isn't ours,
				// send it further
				FuturellaPeer::pointer destinPeer = PeersManager::getManager()->peerFromId(tunnelMsg->peerIdWhom);
				if (destinPeer)
					destinPeer->sendMessage(realMsg);
				else
				{
					// We must tell the sender that the receiver doesn't exist any more
					NetPeerUnavailableMessage* unavailableMsg = new NetPeerUnavailableMessage;
					unavailableMsg->peerId = tunnelMsg->peerIdWhom;
					destinPeer = PeersManager::getManager()->peerFromId(tunnelMsg->peerIdFrom);
					if (destinPeer)
						destinPeer->sendMessage(NetMessage::const_pointer(unavailableMsg));
					else
						delete unavailableMsg;
				}
			}
			else if (realMsg->gettype() == NetPeerUnavailableMessage::type)
			{
				const NetPeerUnavailableMessage* tunnelMsg = static_cast<const NetPeerUnavailableMessage*>(realMsg.get());
				FuturellaPeer::pointer deletedPeer = PeersManager::getManager()->peerFromId(tunnelMsg->peerId);
				if (deletedPeer && deletedPeer->isDirect() == false)
					//deletedPeer->deleteLater();
					;
			}
			else
			{
				//std::for_each(m_receivers.begin(), m_receivers.end(), std::bind(&msgFunc::operator(), std::placeholders::_1, realMsg, this));
				for (auto callBack : m_receivers)
				{
					callBack(realMsg, this);
				}
				assert(transPointers.size() <= 1);
				sendMessageToAll(realMsg);
			}
		}
		else
		{
			std::shared_ptr<const NetHalloMessage> halloMsg = std::dynamic_pointer_cast<const NetHalloMessage>(realMsg);
			if (halloMsg)
				halloProceed(halloMsg);
			else
				throw std::bad_cast();
		}
	}
	catch(std::bad_cast)
	{
		// Wrong or unsupported packet received. Terminate the connection.
		m_connectLine->close();
		
		m_errorSignal("Wrong or unsupported packet received. Terminating the connection...");
	}
}

void FuturellaPeer::onDisconnect(const std::string&)
{
	m_status = MP_DISCONNECTED;
	m_statusChanged();
	// this->deleteLater();
}

void FuturellaPeer::onConnected()
{
	--m_halloStatus;
	m_connectLine->sendMessage(createHalloMsg()->toRaw());
}

void FuturellaPeer::halloProceed(const std::shared_ptr<const NetHalloMessage>& hallo)
{
	if (hallo->version != protokoll_version || hallo->halloStat != m_halloStatus)
	{
		// Wrong hallo packet, break it down
		m_connectLine->close();
		
		m_errorSignal("Wrong or unsupported packet received. Terminating the connection...");
		return;
	}
	if (m_peerIdentityKey == 0)
	{
		m_peerIdentityKey = hallo->peerId;
		if (PeersManager::getManager()->registerPeerId(m_peerIdentityKey, this) == false)
		{
			m_peerIdentityKey = 0;
			m_connectLine->close();
			
			m_errorSignal("Already registered peer connected again. Disconnecting...");
			return;
		}
	}
	m_buddyName = hallo->buddyName;
	if (m_halloStatus != 3)
		m_connectLine->sendMessage(createHalloMsg()->toRaw());
	if (m_halloStatus >= 2)
	{
		m_active = true;
		if (m_halloStatus == 4)
			m_status = MP_SERVER;
		else
			m_status = MP_ACTIVE;
		// sending all messages from the waiting queue
		while(!m_activationWaitingQueue.empty())
		{
			sendMessage(m_activationWaitingQueue.front());
			m_activationWaitingQueue.pop_front();
		}
		// Send a message to other peers about the new peer
		NetAvailablePeerMessage* msg = new NetAvailablePeerMessage;
		msg->buddyName = m_buddyName;
		msg->peerId = m_peerIdentityKey;
		PeersManager::getManager()->broadcast(NetMessage::const_pointer(msg), this);

		m_statusChanged();
		m_onActivated();
	}
}

NetMessage::pointer FuturellaPeer::createHalloMsg() const
{
	NetHalloMessage* myMsg = new NetHalloMessage;
	myMsg->buddyName = m_myName;
	myMsg->halloStat = ++m_halloStatus;
	++m_halloStatus;
	myMsg->version = protokoll_version;
	myMsg->peerId = PeersManager::getManager()->myPeerId;
	return NetMessage::pointer(myMsg);
}

FuturellaPeer::PeerStatus FuturellaPeer::getStatus() const
{
	return m_status;
}

std::string FuturellaPeer::getRemoteName() const
{
	return m_buddyName;
}

PeersManager::PeersManager()
{
	myPeerId = rand();
}

PeersManager* PeersManager::getManager()
{
	if (!mSingleton)
		mSingleton = new PeersManager();
	return mSingleton;
}

void PeersManager::regFuturellaPeer(FuturellaPeer::pointer ptr)
{
	msgPeers.push_back(ptr);
	ptr->onActivation(std::bind(&PeersManager::onPeerActivated, this, ptr), this);
	m_peerRegistred(ptr);
}

void PeersManager::unregisterPeer(FuturellaPeer::pointer peer)
{
	std::deque<FuturellaPeer::pointer>::iterator it = std::find(msgPeers.begin(),
		msgPeers.end(), peer);
	assert(it != msgPeers.end());
	msgPeers.erase(it);
}

bool PeersManager::registerPeerId(uint32_t id, FuturellaPeer::pointer peer)
{
	return id == myPeerId ? false : registeredPeerIds.insert(std::make_pair(id, peer)).second;
}

void PeersManager::unregisterPeerId(uint32_t id)
{
	std::map<uint32_t, FuturellaPeer::pointer>::iterator test = registeredPeerIds.find(id);
	if (test != registeredPeerIds.end())
		registeredPeerIds.erase(test);
}

FuturellaPeer::pointer PeersManager::peerFromId(uint32_t id)const
{
	std::map<uint32_t, FuturellaPeer::pointer>::const_iterator peer = registeredPeerIds.find(id);
	if (peer != registeredPeerIds.end())
		return peer->second;
	return FuturellaPeer::pointer(0);
}

void PeersManager::setMyName(std::string name)
{
	peerName = name;
}

std::string PeersManager::getMyName() const
{
	return peerName;
}

bool PeersManager::isAnyoneConnected() const
{
	return msgPeers.empty() == false;
}

void PeersManager::sendChatMessage(const std::string &msg)
{
	NetChatMessage* myMsg = new NetChatMessage;
	myMsg->message = msg;
	myMsg->sentTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	broadcast(NetMessage::const_pointer(myMsg));
}

void PeersManager::broadcast(NetMessage::const_pointer msg, FuturellaPeer::pointer exclude)
{
	for(FuturellaPeer::pointer peer: msgPeers)
	{
		if (peer != exclude)
			peer->sendMessage(msg);
	}
}

void PeersManager::onPeerActivated(const FuturellaPeer::pointer& sendPeer)
{
	// Send the peer all info about other peers available
	for(std::map<uint32_t, FuturellaPeer::pointer>::const_iterator it = registeredPeerIds.begin();
		it != registeredPeerIds.end();
		++it)
	{
		if (it->second == sendPeer) continue;
		NetAvailablePeerMessage* msg = new NetAvailablePeerMessage;
		msg->buddyName = it->second->getRemoteName();
		msg->peerId = it->first;
		sendPeer->sendMessage(NetMessage::const_pointer(msg));
	}
}

BEGIN_NETTORAWMESSAGE_QCONVERT(Chat)
uint16_t messageLength = message.size();
uint16_t netMsgLength = htons(messageLength);
outStr.write(reinterpret_cast<const char*>(&netMsgLength), 2);
outStr.write(message.c_str(), messageLength);
outStr.write(reinterpret_cast<const char*>(&sentTime), sizeof(time_t));
END_NETTORAWMESSAGE_QCONVERT()

BEGIN_RAWTONETMESSAGE_QCONVERT(Chat)
ourStr >> temp->message >> temp->sentTime;
END_RAWTONETMESSAGE_QCONVERT()

BEGIN_NETTORAWMESSAGE_QCONVERT(Hallo)
outStr << buddyName << halloStat << peerId << version;
END_NETTORAWMESSAGE_QCONVERT()

BEGIN_RAWTONETMESSAGE_QCONVERT(Hallo)
ourStr >> temp->buddyName >> temp->halloStat >> temp->peerId >> temp->version;
END_RAWTONETMESSAGE_QCONVERT()

BEGIN_NETTORAWMESSAGE_QCONVERT(AvailablePeer)
outStr << buddyName << peerId;
END_NETTORAWMESSAGE_QCONVERT()

BEGIN_RAWTONETMESSAGE_QCONVERT(AvailablePeer)
ourStr >> temp->buddyName >> temp->peerId;
END_RAWTONETMESSAGE_QCONVERT()

BEGIN_NETTORAWMESSAGE_QCONVERT(Tunneled)
outStr << peerIdWhom << peerIdFrom << msgType << msgData;
END_NETTORAWMESSAGE_QCONVERT()

BEGIN_RAWTONETMESSAGE_QCONVERT(Tunneled)
ourStr >> temp->peerIdWhom >> temp->peerIdFrom >> temp->msgType >> temp->msgData;
END_RAWTONETMESSAGE_QCONVERT()

BEGIN_NETTORAWMESSAGE_QCONVERT(PeerUnavailable)
outStr << peerId;
END_NETTORAWMESSAGE_QCONVERT()

BEGIN_RAWTONETMESSAGE_QCONVERT(PeerUnavailable)
ourStr >> temp->peerId;
END_RAWTONETMESSAGE_QCONVERT()

REGISTER_NETMESSAGE(Chat)
REGISTER_NETMESSAGE(Hallo)
REGISTER_NETMESSAGE(AvailablePeer)
REGISTER_NETMESSAGE(Tunneled)
REGISTER_NETMESSAGE(PeerUnavailable)
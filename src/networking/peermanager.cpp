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

RemotePeersManager* RemotePeersManager::mSingleton = nullptr;

BEGIN_DECLNETMESSAGE(Hallo, 1000, false)
std::string buddyName;
uint32_t halloStat;
uint32_t peerId;
uint16_t version;
END_DECLNETMESSAGE()

BEGIN_DECLNETMESSAGE(AvailablePeer, 1001, false)
std::string buddyName;
uint32_t peerId;
END_DECLNETMESSAGE()

BEGIN_DECLNETMESSAGE(Tunneled, 1002, false)
uint32_t peerIdWhom;
uint32_t peerIdFrom;
uint32_t msgType;
std::string msgData;
END_DECLNETMESSAGE()

BEGIN_DECLNETMESSAGE(PeerUnavailable, 1003, false)
uint32_t peerId;
END_DECLNETMESSAGE()

class TunnelingFuturellaPeer: public RemoteMessagePeer
{
public:
    TunnelingFuturellaPeer(RemoteMessagePeer::pointer tunnel, const NetAvailablePeerMessage* msg);
    TunnelingFuturellaPeer(RemoteMessagePeer::pointer tunnel, uint32_t peerId);
    ~TunnelingFuturellaPeer();
    bool isTunnel()const
    {
        return true;
    }
protected:
    virtual bool takeMessage(const NetMessage::const_pointer& msg, MessagePeer* sender);
    RemoteMessagePeer::pointer mTunnel;
};

TunnelingFuturellaPeer::TunnelingFuturellaPeer(RemoteMessagePeer::pointer tunnel, const NetAvailablePeerMessage* msg):
    RemoteMessagePeer(tunnel->m_netService), mTunnel(tunnel)
{
    bool failedBorn = false;
    m_peerIdentityKey = msg->peerId;
    m_buddyName = msg->buddyName;
    if (RemotePeersManager::getManager()->registerPeerId(m_peerIdentityKey, this) == false)
        failedBorn = true;
    RemotePeersManager::getManager()->regFuturellaPeer(this);

    if (failedBorn)
        m_netService.post([this](){
        delete this;
    });
}

TunnelingFuturellaPeer::TunnelingFuturellaPeer(RemoteMessagePeer::pointer tunnel, uint32_t peerId):
    RemoteMessagePeer(tunnel->m_netService), mTunnel(tunnel)
{
    m_peerIdentityKey = peerId;
    m_buddyName = std::string("[Unknown Source]");
    assert(RemotePeersManager::getManager()->registerPeerId(m_peerIdentityKey, this));
    RemotePeersManager::getManager()->regFuturellaPeer(this);
    tunnel->addDependablePeer(this);
}

TunnelingFuturellaPeer::~TunnelingFuturellaPeer()
{
}

bool TunnelingFuturellaPeer::takeMessage(const NetMessage::const_pointer& msg, MessagePeer*)
{
    if (!m_active)
        return false;
    if (msg->gettype() == NetTunneledMessage::type)
        return mTunnel->send(msg);
    NetTunneledMessage* message = new NetTunneledMessage;
    message->peerIdWhom = m_peerIdentityKey;
    message->peerIdFrom = RemotePeersManager::getManager()->m_myPeerId;
    message->msgType = msg->gettype();
    RawMessage::const_pointer toSend = msg->toRaw();
    if (!toSend->msgBytes.empty())
        message->msgData = toSend->msgBytes;
    return mTunnel->send(NetMessage::const_pointer(message));
}

RemoteMessagePeer::RemoteMessagePeer(boost::asio::io_service &networkThread):
m_active(true), m_netService(networkThread),
m_tobeDestroyed("RemoteMessagePeer::tobeDestroyed"),
m_errorSignal("RemoteMessagePeer::errorSignal"),
m_onActivated("RemoteMessagePeer::onActivated"),
//m_statusChanged("RemoteMessagePeer::statusChanged"),
m_messageReceived("RemoteMessagePeer::messageReceived")
{
    m_connectLine = nullptr;
    m_halloStatus = 0;
    m_status = MP_ACTIVE;
    m_myName = RemotePeersManager::getManager()->m_peerName;
}

RemoteMessagePeer::RemoteMessagePeer(NetConnection* con, bool serverSide, boost::asio::io_service& networkService) :
    m_active(false), m_halloStatus(1), m_buddyName("[Broken Connection]"), m_netService(networkService),
    m_tobeDestroyed("RemoteMessagePeer::tobeDestroyed"),
    m_errorSignal("RemoteMessagePeer::errorSignal"),
    m_onActivated("RemoteMessagePeer::onActivated"),
    //m_statusChanged("RemoteMessagePeer::statusChanged"),
    m_messageReceived("RemoteMessagePeer::messageReceived")
{
    m_peerIdentityKey = 0;
    m_connectLine = con;
    m_status = MP_CONNECTING;
    m_myName = RemotePeersManager::getManager()->m_peerName;
    assert(con);
    bool runHallo = false;
    if (con)
    {
        if (con->isActive())
            runHallo = true;
        //else
        if (!serverSide) // if NetConnection is created by NetServer, it is already "connected"(except perhaps UDP side)
            // prevent it call netConnected here, which going to mess up with handshaking procedure.
            con->onConnected(std::bind(&RemoteMessagePeer::netConnected, this), m_tobeDestroyed);
        con->onDisconnected(std::bind(&RemoteMessagePeer::netDisconnected, this, std::placeholders::_1), m_tobeDestroyed);
        con->onMessageReceived(std::bind(&RemoteMessagePeer::netMessageReceived, this, std::placeholders::_1), m_tobeDestroyed);
    }
    RemotePeersManager::getManager()->regFuturellaPeer(this);
    if (runHallo && !serverSide)
        netConnected();
}

RemoteMessagePeer::~RemoteMessagePeer()
{
    m_tobeDestroyed();
    RemotePeersManager::getManager()->unregisterPeer(this);
    if (m_active && m_connectLine != 0)
    {
        NetPeerUnavailableMessage* msg = new NetPeerUnavailableMessage;
        msg->peerId = m_peerIdentityKey;
        RemotePeersManager::getManager()->broadcast(NetMessage::const_pointer(msg), this);
    }
    if (m_peerIdentityKey)
        RemotePeersManager::getManager()->unregisterPeerId(m_peerIdentityKey);
    if (m_connectLine)
        m_connectLine->scheduleDeletion();
}

bool RemoteMessagePeer::takeMessage(const NetMessage::const_pointer& msg,
                                   MessagePeer*)
{
    std::cout << "Message send: type=" << msg->gettype() << " ";
    // We can't send any messages till we are ready with greetings procedure
    if (m_status == MP_DISCONNECTED)
    {
        std::cout << "DISCONNECTED\n";
        return false;
    }
    if (m_active)
    {
        std::cout << "active=true\n";
        if (msg->prefersUdp())  // TODO: sophisticated udp/tcp choice algorithm?
            m_connectLine->sendAsUDP(msg->toRaw());
        else
            m_connectLine->sendMessage(msg->toRaw());
    }
    else
    {
        std::cout << "active=false\n";
        m_activationWaitingQueue.push_back(msg);
    }
    return m_active;
}

void RemoteMessagePeer::netMessageReceived(RawMessage::pointer msg)
{
    try
    {
        std::cout << "Message receive: type=" << msg->msgType << " ";
        NetMessage::const_pointer realMsg = MsgFactory::create(*msg);
        // If we are not ready, only greetings messages are allowed
        if (m_active)
        {
            std::cout << "active=true\n";
            if (realMsg->gettype() == NetAvailablePeerMessage::type)
            {
                // Create new tunneling peer object#
                const NetAvailablePeerMessage* tunnelMsg = static_cast<const NetAvailablePeerMessage*>(realMsg.get());
                new TunnelingFuturellaPeer(this, tunnelMsg);
            }
            else if (realMsg->gettype() == NetTunneledMessage::type)
            {
                const NetTunneledMessage* tunnelMsg = static_cast<const NetTunneledMessage*>(realMsg.get());
                if (tunnelMsg->peerIdWhom == RemotePeersManager::getManager()->m_myPeerId)
                {
                    RawMessage::pointer innerMsg(new RawMessage);
                    RemoteMessagePeer::pointer tunneledPeer = RemotePeersManager::getManager()->peerFromId(tunnelMsg->peerIdFrom);
                    if (tunneledPeer == 0)
                    {
                        // It shouldn't happen, but it's still possible to create a new TunnelingFuturellaPeer object
                        //qDebug() << "Unexpected message from unknown source, id " << tunnelMsg->peerIdFrom << ". Creating tunnel...";
                        tunneledPeer = new TunnelingFuturellaPeer(this, tunnelMsg->peerIdFrom);
                    }
                    innerMsg->msgType = tunnelMsg->msgType;
                    innerMsg->msgBytes.assign(tunnelMsg->msgData.data(), tunnelMsg->msgData.data() + tunnelMsg->msgData.size());
                    tunneledPeer->netMessageReceived(innerMsg);
                    return;
                }
                // The message isn't ours,
                // send it further
                RemoteMessagePeer::pointer destinPeer = RemotePeersManager::getManager()->peerFromId(tunnelMsg->peerIdWhom);
                if (destinPeer)
                    destinPeer->send(realMsg);
                else
                {
                    // We must tell the sender that the receiver doesn't exist any more
                    NetPeerUnavailableMessage::pointer unavailableMsg{ new NetPeerUnavailableMessage };
                    unavailableMsg->peerId = tunnelMsg->peerIdWhom;
                    destinPeer = RemotePeersManager::getManager()->peerFromId(tunnelMsg->peerIdFrom);
                    if (destinPeer)
                        destinPeer->send(NetMessage::const_pointer(unavailableMsg));
                }
            }
            else if (realMsg->gettype() == NetPeerUnavailableMessage::type)
            {
                const NetPeerUnavailableMessage* tunnelMsg = static_cast<const NetPeerUnavailableMessage*>(realMsg.get());
                RemoteMessagePeer::pointer deletedPeer = RemotePeersManager::getManager()->peerFromId(tunnelMsg->peerId);
                if (deletedPeer && deletedPeer->isTunnel())
                    m_netService.post([=](){ delete deletedPeer; });
            }
            else
            {
                m_messageReceived(realMsg, this);
                broadcastLocally(realMsg);
            }
        }
        else
        {
            std::cout << "active=false\n";
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

void RemoteMessagePeer::netDisconnected(const std::string& err)
{
    if (!err.empty())
        m_errorSignal(err);
    if (m_status == MP_DISCONNECTED)
        return; //about to be deleted anyway

    m_status = MP_DISCONNECTED;
    m_statusChanged();
    m_netService.post([this](){
        delete this;
    });
}

void RemoteMessagePeer::netConnected()
{
    --m_halloStatus;
    m_connectLine->sendMessage(createHalloMsg()->toRaw());
}

void RemoteMessagePeer::halloProceed(const std::shared_ptr<const NetHalloMessage>& hallo)
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
        if (RemotePeersManager::getManager()->registerPeerId(m_peerIdentityKey, this) == false)
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
            send(m_activationWaitingQueue.front());
            m_activationWaitingQueue.pop_front();
        }
        // Send a message to other peers about the new peer
        NetAvailablePeerMessage* msg = new NetAvailablePeerMessage;
        msg->buddyName = m_buddyName;
        msg->peerId = m_peerIdentityKey;
        RemotePeersManager::getManager()->broadcast(NetMessage::const_pointer(msg), this);

        m_statusChanged();
        m_onActivated();
    }
}

NetMessage::pointer RemoteMessagePeer::createHalloMsg() const
{
    NetHalloMessage* myMsg = new NetHalloMessage;
    myMsg->buddyName = m_myName;
    myMsg->halloStat = ++m_halloStatus;
    ++m_halloStatus;
    myMsg->version = protokoll_version;
    myMsg->peerId = RemotePeersManager::getManager()->m_myPeerId;
    return NetMessage::pointer(myMsg);
}

RemoteMessagePeer::PeerStatus RemoteMessagePeer::getStatus() const
{
    return m_status;
}

std::string RemoteMessagePeer::getRemoteName() const
{
    return m_buddyName;
}

void RemoteMessagePeer::addDependablePeer(RemoteMessagePeer* child)
{
    /*m_tobeDestroyed.connect([child](){
        delete child;
    }, child);*/
}

bool RemoteMessagePeer::isTunnel() const
{
    return false;
}

RemotePeersManager::RemotePeersManager():
m_peerRegistred("RemotePeersManager::peerRegistered")
{
    m_myPeerId = rand();
}

RemotePeersManager* RemotePeersManager::getManager()
{
    if (!mSingleton)
        mSingleton = new RemotePeersManager();
    return mSingleton;
}

void RemotePeersManager::regFuturellaPeer(RemoteMessagePeer::pointer ptr)
{
    m_msgPeers.push_back(ptr);
    ptr->onActivation(std::bind(&RemotePeersManager::onPeerActivated, this, ptr));
    m_peerRegistred(ptr);
}

void RemotePeersManager::unregisterPeer(RemoteMessagePeer* peer)
{
    std::deque<RemoteMessagePeer::pointer>::iterator it = std::find(m_msgPeers.begin(),
        m_msgPeers.end(), peer);
    assert(it != m_msgPeers.end());
    m_msgPeers.erase(it);
}

bool RemotePeersManager::registerPeerId(uint32_t id, RemoteMessagePeer::pointer peer)
{
    return id == m_myPeerId ? false : m_registeredPeerIds.insert(std::make_pair(id, peer)).second;
}

void RemotePeersManager::unregisterPeerId(uint32_t id)
{
    std::map<uint32_t, RemoteMessagePeer::pointer>::iterator test = m_registeredPeerIds.find(id);
    if (test != m_registeredPeerIds.end())
        m_registeredPeerIds.erase(test);
}

RemoteMessagePeer::pointer RemotePeersManager::peerFromId(uint32_t id)const
{
    std::map<uint32_t, RemoteMessagePeer::pointer>::const_iterator peer = m_registeredPeerIds.find(id);
    if (peer != m_registeredPeerIds.end())
        return peer->second;
    return RemoteMessagePeer::pointer(0);
}

void RemotePeersManager::setMyName(std::string name)
{
    m_peerName = name;
}

std::string RemotePeersManager::getMyName() const
{
    return m_peerName;
}

bool RemotePeersManager::isAnyoneConnected() const
{
    return m_msgPeers.empty() == false;
}

void RemotePeersManager::sendChatMessage(const std::string &msg)
{
    NetChatMessage* myMsg = new NetChatMessage;
    myMsg->message = msg;
    myMsg->sentTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    broadcast(NetMessage::const_pointer(myMsg));
}

void RemotePeersManager::broadcast(NetMessage::const_pointer msg, RemoteMessagePeer::pointer exclude)
{
    for(RemoteMessagePeer::pointer peer: m_msgPeers)
    {
        if (peer != exclude)
            peer->send(msg);
    }
}

void RemotePeersManager::onPeerActivated(const RemoteMessagePeer::pointer& sendPeer)
{
    // Send the peer all info about other peers available
    for(std::map<uint32_t, RemoteMessagePeer::pointer>::const_iterator it = m_registeredPeerIds.begin();
        it != m_registeredPeerIds.end();
        ++it)
    {
        if (it->second == sendPeer) continue;
        NetAvailablePeerMessage* msg = new NetAvailablePeerMessage;
        msg->buddyName = it->second->getRemoteName();
        msg->peerId = it->first;
        sendPeer->send(NetMessage::const_pointer(msg));
    }
}

BEGIN_NETTORAWMESSAGE_QCONVERT(Chat)
outStr << message << sentTime;
END_NETTORAWMESSAGE_QCONVERT()

BEGIN_RAWTONETMESSAGE_QCONVERT(Chat)
inStr >> temp->message >> temp->sentTime;
END_RAWTONETMESSAGE_QCONVERT()

BEGIN_NETTORAWMESSAGE_QCONVERT(Hallo)
outStr << buddyName << halloStat << peerId << version;
END_NETTORAWMESSAGE_QCONVERT()

BEGIN_RAWTONETMESSAGE_QCONVERT(Hallo)
inStr >> temp->buddyName >> temp->halloStat >> temp->peerId >> temp->version;
END_RAWTONETMESSAGE_QCONVERT()

BEGIN_NETTORAWMESSAGE_QCONVERT(AvailablePeer)
outStr << buddyName << peerId;
END_NETTORAWMESSAGE_QCONVERT()

BEGIN_RAWTONETMESSAGE_QCONVERT(AvailablePeer)
inStr >> temp->buddyName >> temp->peerId;
END_RAWTONETMESSAGE_QCONVERT()

BEGIN_NETTORAWMESSAGE_QCONVERT(Tunneled)
outStr << peerIdWhom << peerIdFrom << msgType << msgData;
END_NETTORAWMESSAGE_QCONVERT()

BEGIN_RAWTONETMESSAGE_QCONVERT(Tunneled)
inStr >> temp->peerIdWhom >> temp->peerIdFrom >> temp->msgType >> temp->msgData;
END_RAWTONETMESSAGE_QCONVERT()

BEGIN_NETTORAWMESSAGE_QCONVERT(PeerUnavailable)
outStr << peerId;
END_NETTORAWMESSAGE_QCONVERT()

BEGIN_RAWTONETMESSAGE_QCONVERT(PeerUnavailable)
inStr >> temp->peerId;
END_RAWTONETMESSAGE_QCONVERT()

REGISTER_NETMESSAGE(Chat)
REGISTER_NETMESSAGE(Hallo)
REGISTER_NETMESSAGE(AvailablePeer)
REGISTER_NETMESSAGE(Tunneled)
REGISTER_NETMESSAGE(PeerUnavailable)
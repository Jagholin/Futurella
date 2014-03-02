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

typedef GenericNetMessage<1000, std::string, uint32_t, uint32_t, uint16_t, uint16_t> NetHalloMessage; // varNames: "buddyName", "halloStat", "peerId", "udpPort", "version"
typedef GenericNetMessage<1001, std::string, uint32_t> NetAvailablePeerMessage; // varNames: "buddyName", "peerId"
typedef GenericNetMessage<1002, uint32_t, uint32_t, uint32_t, std::string> NetTunneledMessage; // varNames: "peerIdWhom", "peerIdFrom", "msgType", "msgData"
typedef GenericNetMessage<1003, uint32_t> NetPeerUnavailableMessage; // varNames: "peerId"

template<> MessageMetaData 
NetChatMessage::m_metaData = MessageMetaData::createMetaData<NetChatMessage>("message\ntime", NetMessage::MESSAGE_PREFERS_UDP);

template<> MessageMetaData
NetHalloMessage::m_metaData = MessageMetaData::createMetaData<NetHalloMessage>("buddyName\nhalloStat\npeerId\nudpPort\nversion");

template<> MessageMetaData
NetAvailablePeerMessage::m_metaData = MessageMetaData::createMetaData<NetAvailablePeerMessage>("buddyName\npeerId");

template<> MessageMetaData
NetTunneledMessage::m_metaData = MessageMetaData::createMetaData<NetTunneledMessage>("peerIdWhom\npeerIdFrom\nmsgType\nmsgData");

template<> MessageMetaData
NetPeerUnavailableMessage::m_metaData = MessageMetaData::createMetaData<NetPeerUnavailableMessage>("peerId");

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
    m_peerIdentityKey = msg->get<uint32_t>("peerId");
    m_buddyName = msg->get<std::string>("buddyName");
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
    message->get<uint32_t> ("peerIdWhom") = m_peerIdentityKey;
    message->get<uint32_t> ("peerIdFrom") = RemotePeersManager::getManager()->m_myPeerId;
    message->get<uint32_t> ("msgType") = msg->gettype();
    RawMessage::const_pointer toSend = msg->toRaw();
    if (!toSend->msgBytes.empty())
        message->get<std::string> ("msgData") = toSend->msgBytes;
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
        msg->get<uint32_t>("peerId") = m_peerIdentityKey;
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
    //std::cout << "Message send: type=" << msg->gettype() << " ";
    // We can't send any messages till we are ready with greetings procedure
    if (m_status == MP_DISCONNECTED)
    {
        std::cout << "DISCONNECTED\n";
        return false;
    }
    if (m_active)
    {
        //std::cout << "active=true\n";
        if (msg->prefersUdp())  // TODO: sophisticated udp/tcp choice algorithm?
            m_connectLine->sendAsUDP(msg->toRaw());
        else
            m_connectLine->sendMessage(msg->toRaw());
    }
    else
    {
        //std::cout << "active=false\n";
        m_activationWaitingQueue.push_back(msg);
    }
    return m_active;
}

void RemoteMessagePeer::netMessageReceived(RawMessage::pointer msg)
{
    try
    {
        //std::cout << "Message receive: type=" << msg->msgType << " ";
        NetMessage::const_pointer realMsg = MsgFactory::create(*msg);
        // If we are not ready, only greetings messages are allowed
        if (m_active)
        {
            //std::cout << "active=true\n";
            if (realMsg->gettype() == NetAvailablePeerMessage::type)
            {
                // Create new tunneling peer object#
                const NetAvailablePeerMessage* tunnelMsg = static_cast<const NetAvailablePeerMessage*>(realMsg.get());
                new TunnelingFuturellaPeer(this, tunnelMsg);
            }
            else if (realMsg->gettype() == NetTunneledMessage::type)
            {
                const NetTunneledMessage* tunnelMsg = static_cast<const NetTunneledMessage*>(realMsg.get());
                if (tunnelMsg->get<uint32_t>("peerIdWhom") == RemotePeersManager::getManager()->m_myPeerId)
                {
                    RawMessage::pointer innerMsg(new RawMessage);
                    RemoteMessagePeer::pointer tunneledPeer = RemotePeersManager::getManager()->peerFromId(tunnelMsg->get<uint32_t>("peerIdFrom"));
                    if (tunneledPeer == 0)
                    {
                        // It shouldn't happen, but it's still possible to create a new TunnelingFuturellaPeer object
                        //qDebug() << "Unexpected message from unknown source, id " << tunnelMsg->peerIdFrom << ". Creating tunnel...";
                        tunneledPeer = new TunnelingFuturellaPeer(this, tunnelMsg->get<uint32_t>("peerIdFrom"));
                    }
                    innerMsg->msgType = tunnelMsg->get<uint32_t>("msgType");
                    innerMsg->msgBytes.assign(tunnelMsg->get<std::string>("msgData").data(), tunnelMsg->get<std::string>("msgData").data() 
                        + tunnelMsg->get<std::string>("msgData").size());
                    tunneledPeer->netMessageReceived(innerMsg);
                    return;
                }
                // The message isn't ours,
                // send it further
                RemoteMessagePeer::pointer destinPeer = RemotePeersManager::getManager()->peerFromId(tunnelMsg->get<uint32_t>("peerIdWhom"));
                if (destinPeer)
                    destinPeer->send(realMsg);
                else
                {
                    // We must tell the sender that the receiver doesn't exist any more
                    NetPeerUnavailableMessage::pointer unavailableMsg{ new NetPeerUnavailableMessage };
                    unavailableMsg->get<uint32_t>("peerId") = tunnelMsg->get<uint32_t>("peerIdWhom");
                    destinPeer = RemotePeersManager::getManager()->peerFromId(tunnelMsg->get<uint32_t>("peerIdFrom"));
                    if (destinPeer)
                        destinPeer->send(NetMessage::const_pointer(unavailableMsg));
                }
            }
            else if (realMsg->gettype() == NetPeerUnavailableMessage::type)
            {
                const NetPeerUnavailableMessage* tunnelMsg = static_cast<const NetPeerUnavailableMessage*>(realMsg.get());
                RemoteMessagePeer::pointer deletedPeer = RemotePeersManager::getManager()->peerFromId(tunnelMsg->get<uint32_t>("peerId"));
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
            //std::cout << "active=false\n";
            //NetHalloMessage::const_pointer halloMsg = std::dynamic_pointer_cast<const NetHalloMessage>(realMsg);
            //if (halloMsg)
                halloProceed(realMsg);
            //else
            //    throw std::bad_cast();
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

void RemoteMessagePeer::halloProceed(const NetMessage::const_pointer& halloMsg)
{
    NetHalloMessage::const_pointer hallo = halloMsg->as_safe<NetHalloMessage>();
    if (hallo->get<uint16_t>("version") != protokoll_version || hallo->get<uint32_t>("halloStat") != m_halloStatus)
    {
        // Wrong hallo packet, break it down
        m_connectLine->close();
        
        m_errorSignal("Wrong or unsupported packet received. Terminating the connection...");
        return;
    }
    if (m_peerIdentityKey == 0)
    {
        m_peerIdentityKey = hallo->get<uint32_t>("peerId");
        if (RemotePeersManager::getManager()->registerPeerId(m_peerIdentityKey, this) == false)
        {
            m_peerIdentityKey = 0;
            m_connectLine->close();
            
            m_errorSignal("Already registered peer connected again. Disconnecting...");
            return;
        }
    }
    m_buddyName = hallo->get<std::string>("buddyName");
    if (m_halloStatus != 3)
        m_connectLine->sendMessage(createHalloMsg()->toRaw());
    if (m_halloStatus >= 2)
    {
        m_active = true;
        if (m_halloStatus == 4)
        {
            m_status = MP_ACTIVE;
        }
        else
        {
            m_status = MP_SERVER;
        }
        m_connectLine->setupUdpSocket(hallo->get<uint16_t>("udpPort"));
        // sending all messages from the waiting queue
        while(!m_activationWaitingQueue.empty())
        {
            send(m_activationWaitingQueue.front());
            m_activationWaitingQueue.pop_front();
        }
        // Send a message to other peers about the new peer
        NetAvailablePeerMessage::pointer msg{ new NetAvailablePeerMessage };
        msg->get<std::string>("buddyName") = m_buddyName;
        msg->get<uint32_t>("peerId") = m_peerIdentityKey;
        RemotePeersManager::getManager()->broadcast(NetMessage::const_pointer(msg), this);

        m_statusChanged();
        m_onActivated();
    }
}

NetMessage::pointer RemoteMessagePeer::createHalloMsg() const
{
    NetHalloMessage::pointer myMsg{ new NetHalloMessage };
    myMsg->get<std::string>("buddyName") = m_myName;
    myMsg->get<uint32_t>("halloStat") = ++m_halloStatus;
    ++m_halloStatus;
    myMsg->get<uint16_t>("udpPort") = RemotePeersManager::getManager()->m_myUdpPort;
    myMsg->get<uint16_t>("version") = protokoll_version;
    myMsg->get<uint32_t>("peerId") = RemotePeersManager::getManager()->m_myPeerId;
    return myMsg;
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
    m_myUdpPort = 0;
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
    std::get<0>(myMsg->m_values) = msg;
    std::get<1>(myMsg->m_values) = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
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
        msg->get<std::string>("buddyName") = it->second->getRemoteName();
        msg->get<uint32_t>("peerId") = it->first;
        sendPeer->send(NetMessage::const_pointer(msg));
    }
}

uint32_t RemotePeersManager::getMyId() const
{
    return m_myPeerId;
}

void RemotePeersManager::setMyUdpPort(uint16_t portNum)
{
    m_myUdpPort = portNum;
}

uint32_t RemotePeersManager::getPeersId(MessagePeer* peer) const
{
    for (auto regRemotePeers : m_registeredPeerIds)
    {
        if (regRemotePeers.second == peer)
            return regRemotePeers.first;
    }
    return getMyId();
}

REGISTER_NETMESSAGE(Chat)
REGISTER_NETMESSAGE(Hallo)
REGISTER_NETMESSAGE(AvailablePeer)
REGISTER_NETMESSAGE(Tunneled)
REGISTER_NETMESSAGE(PeerUnavailable)
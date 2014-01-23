/*
Copyright (C) 2009 - 2014 Belskiy Pavel, github.com/Jagholin
*/
#pragma once

#include "messages.h"
#include <map>
#include <ctime>
#include <functional>
#include <atomic>
#include <boost/asio.hpp>

#include <osg/Referenced>
#include "sigslot.h"
class NetHalloMessage;
class NetConnection;

// Remote Message peer is an endpoint in the local network graph, that represents a service
// which is installed remotely.
// This can be a remote client(from POV of a local server), a remote server(that a local client is
// connected to), or even an equal-rights partner in a Peer2Peer network.
class RemoteMessagePeer: public MessagePeer
{
public:
    typedef std::function<void(NetMessage::const_pointer, RemoteMessagePeer*)> msgFunc;
    typedef RemoteMessagePeer* pointer;
    typedef RemoteMessagePeer* weak_pointer;

    enum { protokoll_version = 1002 };
    enum PeerStatus { MP_CONNECTING = 0, MP_ACTIVE, MP_SERVER, MP_DISCONNECTED };

protected:
    friend class TunnelingFuturellaPeer;

    NetConnection* m_connectLine;                                       // << NetConnection object used to communicate remotely
    std::deque<NetMessage::const_pointer> m_activationWaitingQueue;     // << messages waiting for the handshaking to finish before they can be sent
    std::deque<RemoteMessagePeer::weak_pointer> m_dependencyList;       // << list of all tunnels that are using this peer to communicate through
    std::atomic<bool> m_active;                                         // << true if connection is active and the handshake is successfully finished
    mutable unsigned int m_halloStatus;                                 // << handshake phase
    std::string m_buddyName, m_myName;                                  // << name of the remote counterpart and the self(from RemotePeersManager) 
    uint32_t m_peerIdentityKey;                                         // << id to identify the source of communication over the network.
    boost::asio::io_service& m_netService;                              // << network thread service.
    PeerStatus m_status;                                                // << connection status

    addstd::signal <void()> m_tobeDestroyed;                            // << signal fired in the event of this object's destruction
    addstd::signal <void(std::string)> m_errorSignal;                   // << signal fired in the event of network or some other error
    addstd::signal <void()> m_onActivated;                              // << signal fired when the RemoteMessagePeer is ready to work
    addstd::signal <void()> m_statusChanged;                            // << signal fired in the event of this Peer's status change
    addstd::signal <void(NetMessage::const_pointer, RemoteMessagePeer*)>
        m_messageReceived;                                              // << signals about incoming message packets

    virtual bool takeMessage(const NetMessage::const_pointer&, MessagePeer*);
    RemoteMessagePeer(boost::asio::io_service& networkThread);

    // adds a new tunnel to the list
    void addDependablePeer(RemoteMessagePeer* child);

public:
    // Creates new MessagePeer using an existing network connection
    RemoteMessagePeer(NetConnection*, bool serverSide, boost::asio::io_service& networkThread);
    virtual ~RemoteMessagePeer();

    //virtual bool sendMessage(const NetMessage::const_pointer&);
    PeerStatus getStatus()const;
    std::string getRemoteName()const;
    virtual bool isTunnel()const;

    // Functions to connect signals with signal handlers.
    template<typename T=int> void onPeerDestruction(const std::function<void()> &callBack, T&& closure = int(-1)) { m_tobeDestroyed.connect(callBack, std::forward<T>(closure)); }
    template<typename T=int> void onError(const std::function<void(std::string)> &callBack, T&& closure = int(-1)) { m_errorSignal.connect(callBack, std::forward<T>(closure)); }
    template<typename T=int> void onActivation(const std::function<void()> &callBack, T&& closure = int(-1)) { m_onActivated.connect(callBack, std::forward<T>(closure)); }
    template<typename T=int> void onStatusChange(const std::function<void()> &callBack, T&& closure = int(-1)) { m_statusChanged.connect(callBack, std::forward<T>(closure)); }
    template<typename T=int> void onMessage(const msgFunc &callBack, T&& closure = int(-1)) { m_messageReceived.connect(callBack, std::forward<T>(closure)); }

protected:
    virtual void netMessageReceived(RawMessage::pointer);
    virtual void netDisconnected(const std::string&);
    virtual void netConnected();

    void halloProceed(const std::shared_ptr<const NetHalloMessage>& hallo);
    NetMessage::pointer createHalloMsg()const;
};

// This class has a list of all existing RemoteMessagePeers
// and lets the user to react when new remote peers are created.
class RemotePeersManager
{
protected:
    static RemotePeersManager* mSingleton;
    std::deque<RemoteMessagePeer::pointer> m_msgPeers;
    std::map<uint32_t, RemoteMessagePeer::pointer> m_registeredPeerIds;
    uint32_t m_myPeerId;
    std::string m_peerName;

    RemotePeersManager();
    void unregisterPeer(RemoteMessagePeer*);
    void regFuturellaPeer(RemoteMessagePeer::pointer);
    bool registerPeerId(uint32_t id, RemoteMessagePeer::pointer);
    void unregisterPeerId(uint32_t id);

    RemoteMessagePeer::pointer peerFromId(uint32_t id)const;
public:
    static RemotePeersManager* getManager();

    bool isAnyoneConnected()const;
    void broadcast(NetMessage::const_pointer, RemoteMessagePeer::pointer exclude = 0);

    void setMyName(std::string name);
    std::string getMyName()const;

    template<typename T=int> void onNewPeerRegistration(const std::function<void(RemoteMessagePeer::pointer)> &callBack, T&& closure = int(-1))
    {
        m_peerRegistred.connect(callBack, std::forward<T>(closure));
    }

    friend class RemoteMessagePeer;
    friend class TunnelingFuturellaPeer;
protected:
    addstd::signal<void(RemoteMessagePeer::pointer)> m_peerRegistred;
public:
    void sendChatMessage(const std::string&);
protected:
    void onPeerActivated(const RemoteMessagePeer::pointer& sender);
};

BEGIN_DECLNETMESSAGE(Chat, 100, true)
std::string message;
std::time_t sentTime;
END_DECLNETMESSAGE()

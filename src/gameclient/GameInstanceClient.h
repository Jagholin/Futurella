#pragma once

#include "../gamecommon/GameMessagePeer.h"

class GameInstanceClient : public GameMessagePeer
{
    // TODO
public:
    GameInstanceClient();
    //...

    // Connect clientOrphaned signal to the given slot.
    // You can use this function to destroy clients that lost their connection to the server.
    template<typename T=int> void onClientOrphaned(const std::function<void()>& slot, T&& closure = int(-1)) { m_clientOrphaned.connect(slot, std::forward<T>(closure)); }

    virtual void connectLocallyTo(MessagePeer* buddy, bool recursive = true);
    virtual void disconnectLocallyFrom(MessagePeer* buddy, bool recursive = true);

    virtual bool unknownObjectIdMessage(const GameMessage::const_pointer& msg, MessagePeer* sender);

protected:
    addstd::signal<void()> m_clientOrphaned;

    bool m_connected;
    bool m_orphaned;
};

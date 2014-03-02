#pragma once

#include "../networking/messages.h"
#include <OpenThreads/ScopedLock>
#include <OpenThreads/Mutex>

struct GameMessage : public NetMessage
{
    virtual uint16_t objectId() const = 0;
    virtual void objectId(uint16_t) = 0;

    typedef std::shared_ptr<GameMessage> pointer;
    typedef std::shared_ptr<const GameMessage> const_pointer;
};

template<int MessageTypeID, typename... Types>
struct GenericGameMessage : public GenericNetMessage_Base<GameMessage, MessageTypeID, Types..., uint16_t>
{
    typedef std::shared_ptr<GenericGameMessage<MessageTypeID, Types...>> pointer;
    typedef std::shared_ptr<const GenericGameMessage<MessageTypeID, Types...>> const_pointer;
    typedef GenericNetMessage_Base<GameMessage, MessageTypeID, Types..., uint16_t> base;

    static void postMetaInit(MessageMetaData& md)
    {
        md.declVariable<GenericGameMessage<MessageTypeID, Types...>, uint16_t, sizeof...(Types)>
            ("objectId");
    }

    uint16_t objectId() const override
    {
        return get<uint16_t>("objectId");
    }

    void objectId(uint16_t id) override
    {
        get<uint16_t>("objectId") = id;
    }

    bool isGameMessage() const override
    {
        return true;
    }
    
    static unsigned int mtype;
};

#define REGISTER_GAMEMESSAGE(name) REGISTER_MESSAGE_BASE(name, Game)

class GameObject;

class GameMessagePeer : public LocalMessagePeer
{
protected:
    GameMessagePeer();
    virtual ~GameMessagePeer();
    virtual bool takeMessage(const NetMessage::const_pointer&, MessagePeer*);

public:

    // Retrieves owner_id identification of this object.
    virtual uint16_t getOwnerId() const;

    void registerGameObject(uint16_t objId, GameObject* obj);
    uint16_t registerGameObject(GameObject*);
    void unregisterGameObject(uint16_t objId);
    virtual bool send(const NetMessage::const_pointer& msg, MessagePeer* sender = nullptr);

    virtual boost::asio::io_service* eventService() = 0;

    friend class GameObject;
protected:
    // Sends a message to its partner.
    // used by GameObject only.
    void messageToPartner(const GameMessage::const_pointer& msg);
    virtual bool unknownObjectIdMessage(const GameMessage::const_pointer& msg, MessagePeer* sender) = 0;

protected:
    std::map<uint16_t, GameObject*> m_messageListeners;
    uint16_t m_lastObjectId;
    std::deque<MessagePeer::workPair> m_highPriorityQueue;

    OpenThreads::Mutex m_messageQueueMutex;
};

#pragma once

#include "../networking/messages.h"

struct GameMessage : public NetMessage
{
    uint16_t objectId;

    typedef std::shared_ptr<GameMessage> pointer;
    typedef std::shared_ptr<const GameMessage> const_pointer;
};

#define BEGIN_DECLGAMEMESSAGE(name, number, udppref) DECLMESSAGE_BASE(name, number, udppref, true, Game, GameMessage)

#define END_DECLGAMEMESSAGE() };

#define REGISTER_GAMEMESSAGE(name) REGISTER_MESSAGE_BASE(name, Game)

#define BEGIN_GAMETORAWMESSAGE_QCONVERT(name) BEGIN_TORAWMESSAGE_QCONVERTBASE(name, Game) \
    out << objectId;

#define END_GAMETORAWMESSAGE_QCONVERT() } result->msgBytes = out.str(); \
    return result; }

#define BEGIN_RAWTOGAMEMESSAGE_QCONVERT(name) BEGIN_TONETMESSAGE_QCONVERTBASE(name, Game) \
    in >> objectId;

#define END_RAWTOGAMEMESSAGE_QCONVERT() }

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
};

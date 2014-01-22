#pragma once

#include "../networking/messages.h"

struct GameMessage : public NetMessage, public std::enable_shared_from_this<GameMessage>
{
    uint16_t objectId;

    typedef std::shared_ptr<GameMessage> pointer;
    typedef std::shared_ptr<const GameMessage> const_pointer;
};

#define BEGIN_DECLGAMEMESSAGE(name, number, udppref) DECLMESSAGE_BASE(name, number, udppref, true, Game, GameMessage)

#define END_DECLGAMEMESSAGE() };

#define REGISTER_GAMEMESSAGE(name) REGISTER_MESSAGE_BASE(name, Game)

#define BEGIN_GAMETORAWMESSAGE_QCONVERT(name) BEGIN_TORAWMESSAGE_QCONVERTBASE(name, Game)

#define END_GAMETORAWMESSAGE_QCONVERT() } result->msgBytes = outStr.str(); \
    return result; }

#define BEGIN_RAWTOGAMEMESSAGE_QCONVERT(name) BEGIN_TONETMESSAGE_QCONVERTBASE(name, Game)

#define END_RAWTOGAMEMESSAGE_QCONVERT() return NetMessage::pointer(temp);}

class GameObject;

class GameMessagePeer : public MessagePeer
{
protected:
    GameMessagePeer();
    virtual ~GameMessagePeer();
    virtual bool takeMessage(const NetMessage::const_pointer&, MessagePeer*);

    // this function is to be implemented in all inherited classes
    //virtual bool takeMessage(const GameMessage::const_pointer&, MessagePeer*) = 0;

public:
    // ...

    // Retrieves owner_id identification of this object.
    virtual uint16_t getOwnerId() const;

    void registerGameObject(uint16_t objId, GameObject* obj);
    uint16_t registerGameObject(GameObject*);
    void unregisterGameObject(uint16_t objId);
protected:
    std::map<uint16_t, GameObject*> m_messageListeners;
    uint16_t m_lastObjectId;
};

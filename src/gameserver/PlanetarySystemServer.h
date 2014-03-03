#pragma once
#include "../LargeScaleCoords.h"
#include "../gamecommon/GameObject.h"

typedef GenericGameMessage<3300, uint32_t> GameCreatePlanetarySystemMessage; // varNames: "ownerId"

class PlanetarySystemServer : public GameObject
{
public:
    typedef std::shared_ptr<PlanetarySystemServer> pointer;

    PlanetarySystemServer(uint32_t ownerId, GameMessagePeer* ctx);
    virtual ~PlanetarySystemServer();

    virtual bool takeMessage(const GameMessage::const_pointer& msg, MessagePeer* sender) override;

    GameMessage::pointer creationMessage() const;
protected:

    // TODO: add suns, planets:)
};

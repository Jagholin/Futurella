#pragma once
#include "../gamecommon/GameObject.h"
#include "../AsteroidField.h"
#include <Magnum/Magnum.h>

using namespace Magnum;

typedef GenericGameMessage<5001, std::vector<Vector3>, std::vector<float>, uint32_t, Vector3i> GameAsteroidFieldDataMessage; // varNames: "position", "radius", "ownerId", "chunkCoord"

class GameInstanceServer;
class PhysicsEngine;

class AsteroidFieldChunkServer : public GameObject
{
public:
    typedef std::shared_ptr<AsteroidFieldChunkServer> pointer;

    AsteroidFieldChunkServer(int numOfAsteroids, float fieldSideLength, float astMinSize, float astMaxSize, uint32_t ownerId, Vector3i chunkCoord, GameInstanceServer* ctx, PhysicsEngine* engine);
    virtual ~AsteroidFieldChunkServer();

    GameMessage::pointer creationMessage() const;

    virtual bool takeMessage(const GameMessage::const_pointer& msg, MessagePeer* sender);

    AsteroidField* getAsteroidField();

protected:

    Vector3i m_chunkCoord;
    std::shared_ptr<AsteroidField> m_asteroids;
    std::vector<unsigned int> m_physicsIds;
    PhysicsEngine* m_physicsEngine;
};

#pragma once
#include "../gamecommon/GameObject.h"
#include "../AsteroidField.h"
#include <osg/Vec3i>

typedef GenericGameMessage<5001, std::vector<osg::Vec3f>, std::vector<float>, uint32_t, osg::Vec3i> GameAsteroidFieldDataMessage; // varNames: "position", "radius", "ownerId", "chunkCoord"

class GameInstanceServer;
class PhysicsEngine;

class AsteroidFieldChunkServer : public GameObject
{
public:
    typedef std::shared_ptr<AsteroidFieldChunkServer> pointer;

    AsteroidFieldChunkServer(int numOfAsteroids, float fieldSideLength, float astMinSize, float astMaxSize, uint32_t ownerId, osg::Vec3i chunkCoord, GameInstanceServer* ctx, PhysicsEngine* engine);
    virtual ~AsteroidFieldChunkServer();

    GameMessage::pointer creationMessage() const;

    virtual bool takeMessage(const GameMessage::const_pointer& msg, MessagePeer* sender);

    float getCubeSideLength();

protected:

    float m_asteroidFieldSpaceCubeSideLength;
    osg::Vec3i m_chunkCoord;
    std::shared_ptr<AsteroidField> m_asteroids;
    std::vector<unsigned int> m_physicsIds;
    PhysicsEngine* m_physicsEngine;
};

#pragma once
#include "../gamecommon/GameObject.h"
#include "../AsteroidField.h"
#include <osg/Vec3i>

BEGIN_DECLGAMEMESSAGE(AsteroidFieldData, 5001, false)
std::vector<osg::Vec3f> position;
std::vector<float> radius;
uint32_t ownerId;
osg::Vec3i chunkCoord;
END_DECLGAMEMESSAGE()

class GameInstanceServer;
class PhysicsEngine;

class AsteroidFieldChunkServer : public GameObject
{
public:
    typedef std::shared_ptr<AsteroidFieldChunkServer> pointer;

    AsteroidFieldChunkServer(int numOfAsteroids, float density, uint32_t ownerId, osg::Vec3i chunkCoord, GameInstanceServer* ctx, PhysicsEngine* engine);
    virtual ~AsteroidFieldChunkServer();

    GameMessage::pointer creationMessage() const;

    virtual bool takeMessage(const GameMessage::const_pointer& msg, MessagePeer* sender);

    float getCubeSideLength();

protected:

    const float m_astMinSize = 0.2f, m_astMaxSize = 1.0f;
    float m_asteroidFieldSpaceCubeSideLength;
    osg::Vec3i m_chunkCoord;
    std::shared_ptr<AsteroidField> m_asteroids;
    std::vector<unsigned int> m_physicsIds;
    PhysicsEngine* m_physicsEngine;
};

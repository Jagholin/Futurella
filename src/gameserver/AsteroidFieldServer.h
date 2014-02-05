#pragma once
#include "../gamecommon/GameObject.h"
#include "../AsteroidField.h"

BEGIN_DECLGAMEMESSAGE(AsteroidFieldData, 5001, false)
std::vector<osg::Vec3f> position;
std::vector<float> radius;
uint32_t ownerId;
END_DECLGAMEMESSAGE()

class GameInstanceServer;
class PhysicsEngine;

class AsteroidFieldServer : public GameObject
{
public:
    typedef std::shared_ptr<AsteroidFieldServer> pointer;

    AsteroidFieldServer(int numOfAsteroids, float turbulence, float density, uint32_t ownerId, GameInstanceServer* ctx, PhysicsEngine* engine);

    GameMessage::pointer creationMessage() const;

    virtual bool takeMessage(const GameMessage::const_pointer& msg, MessagePeer* sender);

    float getCubeSideLength();

protected:

    const float m_astMinSize = 0.2f, m_astMaxSize = 1.0f;
    float m_asteroidFieldSpaceCubeSideLength;
    std::shared_ptr<AsteroidField> m_asteroids;
};

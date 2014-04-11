#include "AsteroidFieldChunkServer.h"
#include "GameInstanceServer.h"
#include "../Asteroid.h"
#include "../gamecommon/PhysicsEngine.h"

#include <random>
#include <memory>

template <> MessageMetaData
GameAsteroidFieldDataMessage::base::m_metaData = MessageMetaData::createMetaData<GameAsteroidFieldDataMessage>("position\nradius\nownerId\nchunkCoord");

REGISTER_GAMEMESSAGE(AsteroidFieldData)

AsteroidFieldChunkServer::AsteroidFieldChunkServer(int asteroidNumber, float chunkSideLength, float astMinSize, float astMaxSize, uint32_t ownerId, const Vector3i& chunk, GameInstanceServer* ctx, PhysicsEngine* engine) :
GameObject(ownerId, ctx)
{
    m_chunkCoord = chunk;
    m_physicsEngine = engine;
    std::random_device randDevice;

    //Generate Asteroid Field
    //step 1: scatter asteroids. save to AsteroidField
    m_asteroids = std::make_shared<AsteroidField>();
    float astSizeDif = astMaxSize - astMinSize;
    float asteroidCubeMaxSidelength = astMaxSize * 2;
    float radius;
    Vector3 pos;
    for (int i = 0; i < asteroidNumber; i++)
    {
        pos = {
            randDevice()*1.0f / randDevice.max() * chunkSideLength,
            randDevice()*1.0f / randDevice.max() * chunkSideLength,
            randDevice()*1.0f / randDevice.max() * chunkSideLength };
        radius = ( ( randDevice() * 1.0f / randDevice.max() ) * ( randDevice() * 1.0f / randDevice.max() ) * astSizeDif + astMinSize ) * 0.5f;
        m_asteroids->addAsteroid(pos, radius);
    }

    //step 2: move asteroids to avoid overlappings (heuristic). save to Level::asteroidField
    int accuracy = 10; //how often d'you wanna apply heuristic?
    for (int iterations = 0; iterations < accuracy; ++iterations)
    {
        std::shared_ptr<AsteroidField> previousScattering = m_asteroids;
        m_asteroids = std::make_shared<AsteroidField>();
        for (int i = 0; i < asteroidNumber; ++i)
        {
            float favoredDistance = (asteroidCubeMaxSidelength - astMaxSize) / 2 + previousScattering->getAsteroid(i)->getRadius();
            float boundingBoxRadius = favoredDistance + astMaxSize / 2;
            std::list<Asteroid*> candidates = previousScattering->getAsteroidsInBlock(previousScattering->getAsteroid(i)->getPosition(), Vector3(boundingBoxRadius));
            Vector3 shift(0, 0, 0);
            int numberOfCloseAsteroids = 0;
            for (std::list<Asteroid*>::iterator it = candidates.begin(); it != candidates.end(); it++) {
                if (*it != previousScattering->getAsteroid(i)) {
                    Vector3 d = previousScattering->getAsteroid(i)->getPosition() - (*it)->getPosition();
                    float scale = -((d.length() - (*it)->getRadius()) - favoredDistance);
                    if (scale > 0) {
                        shift += shift + (d.normalized() * scale *0.5f); //push away from other close asteroids
                        ++numberOfCloseAsteroids;
                    }
                }
            }
            m_asteroids->addAsteroid(previousScattering->getAsteroid(i)->getPosition() + shift, previousScattering->getAsteroid(i)->getRadius());
        }
    }

    // Add asteroids as collision bodies to the engine
    for (unsigned int i = 0; i < m_asteroids->getLength(); ++i)
    {
        Asteroid* currentAst = m_asteroids->getAsteroid(i);
        unsigned int id = engine->addCollisionSphere(currentAst->getPosition() + GameInstanceServer::chunkToPosition(m_chunkCoord), currentAst->getRadius());
        m_physicsIds.push_back(id);
    }
}

AsteroidFieldChunkServer::~AsteroidFieldChunkServer()
{
    for (unsigned int id : m_physicsIds)
        m_physicsEngine->removeCollisionSphere(id);
}

GameMessage::pointer AsteroidFieldChunkServer::creationMessage() const
{
    GameAsteroidFieldDataMessage::pointer msg{ new GameAsteroidFieldDataMessage };
    auto &posVector = msg->get<std::vector<Vector3>>("position");
    auto &radVector = msg->get<std::vector<float>>("radius");
    for (unsigned int i = 0; i < m_asteroids->getLength(); ++i)
    {
        posVector.push_back(m_asteroids->getAsteroid(i)->getPosition());
        radVector.push_back(m_asteroids->getAsteroid(i)->getRadius());
    }
    msg->objectId(m_myObjectId);
    msg->get<uint32_t>("ownerId") = m_myOwnerId;
    msg->get<Vector3i>("chunkCoord") = m_chunkCoord;
    return msg;
}

bool AsteroidFieldChunkServer::takeMessage(const GameMessage::const_pointer& msg, MessagePeer* sender)
{
    return false;
}


AsteroidField* AsteroidFieldChunkServer::getAsteroidField()
{
    return m_asteroids.get();
}
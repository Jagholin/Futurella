#include "AsteroidFieldChunkServer.h"
#include "GameInstanceServer.h"
#include "../Asteroid.h"

#include <random>
#include <memory>

BEGIN_RAWTOGAMEMESSAGE_QCONVERT(AsteroidFieldData)
uint16_t size;
in >> ownerId >> size;
for (unsigned int i = 0; i < size; ++i)
{
    osg::Vec3f pos;
    float rad;
    in >> pos >> rad;
    position.push_back(pos);
    radius.push_back(rad);
}
in >> chunkCoord;
END_RAWTOGAMEMESSAGE_QCONVERT()
BEGIN_GAMETORAWMESSAGE_QCONVERT(AsteroidFieldData)
uint16_t size = position.size();
out << ownerId << size;
for (int i = 0; i < position.size(); ++i)
{
    osg::Vec3f pos = position.at(i);
    float rad = radius.at(i);
    out << position.at(i) << radius.at(i);
}
out << chunkCoord;
END_GAMETORAWMESSAGE_QCONVERT()

REGISTER_GAMEMESSAGE(AsteroidFieldData)

AsteroidFieldChunkServer::AsteroidFieldChunkServer(int asteroidNumber, float chunkSideLength, uint32_t ownerId, osg::Vec3i chunk, GameInstanceServer* ctx):
GameObject(ownerId, ctx)
{
    m_chunkCoord = chunk;
    std::random_device randDevice;

    //Generate Asteroid Field
    //step 1: scatter asteroids. save to AsteroidField
    std::shared_ptr<AsteroidField> scatteredAsteroids = std::make_shared<AsteroidField>();
    float astSizeDif = m_astMaxSize - m_astMinSize;
    float asteroidSpaceCubeSidelength = m_astMaxSize * 2;
    float asteroidFieldSpaceCubeSideLength = chunkSideLength;
    float radius;
    osg::Vec3f pos;
    for (int i = 0; i < asteroidNumber; i++)
    {
        pos.set(
            randDevice()*asteroidFieldSpaceCubeSideLength / randDevice.max(),
            randDevice()*asteroidFieldSpaceCubeSideLength / randDevice.max(),
            randDevice()*asteroidFieldSpaceCubeSideLength / randDevice.max());
        radius = (randDevice()*astSizeDif / randDevice.max() + m_astMinSize)*0.5f;
        scatteredAsteroids->addAsteroid(pos, radius);
    }

    //step 2: move asteroids to avoid overlappings (heuristic). save to Level::asteroidField
    m_asteroids = std::make_shared<AsteroidField>();
    for (int i = 0; i < asteroidNumber; i++)
    {
        float favoredDistance = (asteroidSpaceCubeSidelength - m_astMaxSize) / 2 + scatteredAsteroids->getAsteroid(i)->getRadius();
        float boundingBoxRadius = favoredDistance + m_astMaxSize / 2;
        std::list<Asteroid*> candidates = scatteredAsteroids->getAsteroidsInBlock(scatteredAsteroids->getAsteroid(i)->getPosition(), osg::Vec3f(boundingBoxRadius, boundingBoxRadius, boundingBoxRadius));
        osg::Vec3f shift(0, 0, 0);
        int numberOfCloseAsteroids = 0;
        for (std::list<Asteroid*>::iterator it = candidates.begin(); it != candidates.end(); it++) {
            if (*it != scatteredAsteroids->getAsteroid(i)) {
                osg::Vec3f d = scatteredAsteroids->getAsteroid(i)->getPosition() - (*it)->getPosition();
                float scale = -((d.length() - (*it)->getRadius()) - favoredDistance);
                if (scale > 0) {
                    d.normalize();
                    shift += shift + (d * scale *0.5f); //push away from other close asteroids
                    numberOfCloseAsteroids++;
                }
            }
        }
        m_asteroids->addAsteroid(scatteredAsteroids->getAsteroid(i)->getPosition() + shift, scatteredAsteroids->getAsteroid(i)->getRadius());

    }
}

GameMessage::pointer AsteroidFieldChunkServer::creationMessage() const
{
    GameAsteroidFieldDataMessage::pointer msg{ new GameAsteroidFieldDataMessage };
    for (unsigned int i = 0; i < m_asteroids->getLength(); ++i)
    {
        msg->position.push_back(m_asteroids->getAsteroid(i)->getPosition());
        msg->radius.push_back(m_asteroids->getAsteroid(i)->getRadius());
    }
    msg->ownerId = m_myOwnerId;
    msg->chunkCoord = m_chunkCoord;
    msg->objectId = m_myObjectId;
    return msg;
}

bool AsteroidFieldChunkServer::takeMessage(const GameMessage::const_pointer& msg, MessagePeer* sender)
{
    return false;
}
